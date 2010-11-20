//===--------- SCoPInfo.cpp  - Create SCoPs from LLVM IR ------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Create a polyhedral description for a static control flow region.
//
// The pass creates a polyhedral description of the SCoPs detected by the SCoP
// detection derived from their LLVM-IR code.
//
// This represantation is shared among several tools in the polyhedral
// community, which are e.g. CLooG, Pluto, Loopo, Graphite.
//
//===----------------------------------------------------------------------===//

#include "polly/SCoPInfo.h"

#include "polly/TempSCoPInfo.h"
#include "polly/LinkAllPasses.h"
#include "polly/Support/GmpConv.h"
#include "polly/Support/SCoPHelper.h"

#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/ScalarEvolutionExpressions.h"
#include "llvm/Analysis/RegionIterator.h"

#define DEBUG_TYPE "polly-scops"
#include "llvm/Support/Debug.h"

#include "isl/constraint.h"

using namespace llvm;
using namespace polly;

STATISTIC(SCoPFound,  "Number of valid SCoPs");
STATISTIC(RichSCoPFound,   "Number of SCoPs containing a loop");

//===----------------------------------------------------------------------===//
static void setCoefficient(const SCEV *Coeff, mpz_t v, bool negative,
                           bool isSigned = true) {
  if (Coeff) {
    const SCEVConstant *C = dyn_cast<SCEVConstant>(Coeff);
    const APInt &CI = C->getValue()->getValue();
    MPZ_from_APInt(v, negative ? (-CI) : CI, isSigned);
  } else
    isl_int_set_si(v, 0);
}

static isl_map *getValueOf(const SCEVAffFunc &AffFunc,
                           const SCoPStmt *Statement, isl_dim *dim) {

  const SmallVectorImpl<const SCEV*> &Params =
    Statement->getParent()->getParams();
  unsigned num_in = Statement->getNumIterators(), num_param = Params.size();

  const char *dimname = isl_dim_get_tuple_name(dim, isl_dim_set);
  dim = isl_dim_alloc(isl_dim_get_ctx(dim), num_param,
                      isl_dim_size(dim, isl_dim_set), 1);
  dim = isl_dim_set_tuple_name(dim, isl_dim_in, dimname);

  assert((AffFunc.getType() == SCEVAffFunc::Eq
          || AffFunc.getType() == SCEVAffFunc::ReadMem
          || AffFunc.getType() == SCEVAffFunc::WriteMem)
          && "AffFunc is not an equality");

  isl_constraint *c = isl_equality_alloc(isl_dim_copy(dim));

  isl_int v;
  isl_int_init(v);

  // Set single output dimension.
  isl_int_set_si(v, -1);
  isl_constraint_set_coefficient(c, isl_dim_out, 0, v);

  // Set the coefficient for induction variables.
  for (unsigned i = 0, e = num_in; i != e; ++i) {
    setCoefficient(AffFunc.getCoeff(Statement->getSCEVForDimension(i)), v,
                   false, AffFunc.isSigned());
    isl_constraint_set_coefficient(c, isl_dim_in, i, v);
  }

  // Set the coefficient of parameters
  for (unsigned i = 0, e = num_param; i != e; ++i) {
    setCoefficient(AffFunc.getCoeff(Params[i]), v, false, AffFunc.isSigned());
    isl_constraint_set_coefficient(c, isl_dim_param, i, v);
  }

  // Set the constant.
  setCoefficient(AffFunc.getTransComp(), v, false, AffFunc.isSigned());
  isl_constraint_set_constant(c, v);
  isl_int_clear(v);

  isl_basic_map *BasicMap = isl_basic_map_universe(isl_dim_copy(dim));
  BasicMap = isl_basic_map_add_constraint(BasicMap, c);
  return isl_map_from_basic_map(BasicMap);
}
//===----------------------------------------------------------------------===//

MemoryAccess::~MemoryAccess() {
  isl_map_free(getAccessFunction());
}

isl_basic_map *MemoryAccess::createBasicAccessMap(SCoPStmt *Statement) {
  isl_dim *dim = isl_dim_alloc(Statement->getIslContext(),
                               Statement->getNumParams(),
                               Statement->getNumIterators(), 1);
  std::string name;
  raw_string_ostream OS(name);
  WriteAsOperand(OS, getBaseAddr(), false);
  name = OS.str();

  dim = isl_dim_set_tuple_name(dim, isl_dim_out, name.c_str());
  dim = isl_dim_set_tuple_name(dim, isl_dim_in, Statement->getBaseName());

  return isl_basic_map_universe(dim);
}

MemoryAccess::MemoryAccess(const SCEVAffFunc &AffFunc, SCoPStmt *Statement) {
  BaseAddr = AffFunc.getBaseAddr();
  Type = AffFunc.isRead() ? Read : Write;

  isl_dim *dim = isl_dim_set_alloc(Statement->getIslContext(),
                                   Statement->getNumParams(),
                                   Statement->getNumIterators());
  std::string name;
  raw_string_ostream OS(name);
  WriteAsOperand(OS, getBaseAddr(), false);
  name = OS.str();

  dim = isl_dim_set_tuple_name(dim, isl_dim_set, Statement->getBaseName());

  AccessRelation = getValueOf(AffFunc, Statement, dim);
  AccessRelation = isl_map_set_tuple_name(AccessRelation, isl_dim_out,
                                          name.c_str());
}

MemoryAccess::MemoryAccess(const Value *BaseAddress, SCoPStmt *Statement) {
  BaseAddr = BaseAddress;
  Type = Read;

  isl_basic_map *BasicAccessMap = createBasicAccessMap(Statement);
  AccessRelation = isl_map_from_basic_map(BasicAccessMap);
}

void MemoryAccess::print(raw_ostream &OS) const {
  OS.indent(12) << (isRead() ? "Reads" : "Writes") << " ";
  WriteAsOperand(OS, getBaseAddr(), false);
  OS << " at:\n";
  isl_printer *p = isl_printer_to_str(isl_map_get_ctx(getAccessFunction()));
  isl_printer_print_map(p, getAccessFunction());
  OS.indent(16) << isl_printer_get_str(p) << "\n";
  isl_printer_free(p);
}

void MemoryAccess::dump() const {
  print(errs());
}

//===----------------------------------------------------------------------===//
void SCoPStmt::buildScattering(SmallVectorImpl<unsigned> &Scatter) {
  unsigned NumberOfIterators = getNumIterators();
  unsigned ScatDim = Parent.getMaxLoopDepth() * 2 + 1;
  isl_dim *dim = isl_dim_alloc(Parent.getCtx(), Parent.getNumParams(),
                               NumberOfIterators, ScatDim);
  dim = isl_dim_set_tuple_name(dim, isl_dim_out, "scattering");
  dim = isl_dim_set_tuple_name(dim, isl_dim_in, getBaseName());
  isl_basic_map *bmap = isl_basic_map_universe(isl_dim_copy(dim));
  isl_int v;
  isl_int_init(v);

  // Loop dimensions.
  for (unsigned i = 0; i < NumberOfIterators; ++i) {
    isl_constraint *c = isl_equality_alloc(isl_dim_copy(dim));
    isl_int_set_si(v, 1);
    isl_constraint_set_coefficient(c, isl_dim_out, 2 * i + 1, v);
    isl_int_set_si(v, -1);
    isl_constraint_set_coefficient(c, isl_dim_in, i, v);

    bmap = isl_basic_map_add_constraint(bmap, c);
  }

  // Constant dimensions
  for (unsigned i = 0; i < NumberOfIterators + 1; ++i) {
    isl_constraint *c = isl_equality_alloc(isl_dim_copy(dim));
    isl_int_set_si(v, -1);
    isl_constraint_set_coefficient(c, isl_dim_out, 2 * i, v);
    isl_int_set_si(v, Scatter[i]);
    isl_constraint_set_constant(c, v);

    bmap = isl_basic_map_add_constraint(bmap, c);
  }

  // Fill scattering dimensions.
  for (unsigned i = 2 * NumberOfIterators + 1; i < ScatDim ; ++i) {
    isl_constraint *c = isl_equality_alloc(isl_dim_copy(dim));
    isl_int_set_si(v, 1);
    isl_constraint_set_coefficient(c, isl_dim_out, i, v);
    isl_int_set_si(v, 0);
    isl_constraint_set_constant(c, v);

    bmap = isl_basic_map_add_constraint(bmap, c);
  }

  isl_int_clear(v);
  isl_dim_free(dim);
  Scattering = isl_map_from_basic_map(bmap);
}

void SCoPStmt::buildAccesses(TempSCoP &tempSCoP, const Region &CurRegion) {
  const AccFuncSetType *AccFuncs = tempSCoP.getAccessFunctions(BB);

  for (AccFuncSetType::const_iterator I = AccFuncs->begin(),
       E = AccFuncs->end(); I != E; ++I)
    MemAccs.push_back(new MemoryAccess(*I, this));
}

static isl_map *MapValueToLHS(isl_ctx *Context, unsigned ParameterNumber) {
  std::string MapString;
  isl_map *Map;

  MapString = "{[i0] -> [i0, o1]}";
  Map = isl_map_read_from_str(Context, MapString.c_str(), -1);
  return isl_map_add_dims(Map, isl_dim_param, ParameterNumber);
}

static isl_map *MapValueToRHS(isl_ctx *Context, unsigned ParameterNumber) {
  std::string MapString;
  isl_map *Map;

  MapString = "{[i0] -> [o0, i0]}";
  Map = isl_map_read_from_str(Context, MapString.c_str(), -1);
  return isl_map_add_dims(Map, isl_dim_param, ParameterNumber);
}

static isl_set *getComparison(isl_ctx *Context, const ICmpInst::Predicate Pred,
                              unsigned ParameterNumber) {
  std::string SetString;

  switch (Pred) {
  case ICmpInst::ICMP_EQ:
    SetString = "{[i0, i1] : i0 = i1}";
    break;
  case ICmpInst::ICMP_NE:
    SetString = "{[i0, i1] : i0 + 1 <= i1; [i0, i1] : i0 - 1 >= i1}";
    break;
  case ICmpInst::ICMP_SLT:
    SetString = "{[i0, i1] : i0 + 1 <= i1}";
    break;
  case ICmpInst::ICMP_ULT:
    SetString = "{[i0, i1] : i0 + 1 <= i1}";
    break;
  case ICmpInst::ICMP_SGT:
    SetString = "{[i0, i1] : i0 >= i1 + 1}";
    break;
  case ICmpInst::ICMP_UGT:
    SetString = "{[i0, i1] : i0 >= i1 + 1}";
    break;
  case ICmpInst::ICMP_SLE:
    SetString = "{[i0, i1] : i0 <= i1}";
    break;
  case ICmpInst::ICMP_ULE:
    SetString = "{[i0, i1] : i0 <= i1}";
    break;
  case ICmpInst::ICMP_SGE:
    SetString = "{[i0, i1] : i0 >= i1}";
    break;
  case ICmpInst::ICMP_UGE:
    SetString = "{[i0, i1] : i0 >= i1}";
    break;
  default:
    llvm_unreachable("Non integer predicate not supported");
  }

  isl_set *Set = isl_set_read_from_str(Context, SetString.c_str(), -1);
  return isl_set_add_dims(Set, isl_dim_param, ParameterNumber);
}

static isl_set *compareValues(isl_map *LeftValue, isl_map *RightValue,
                              const ICmpInst::Predicate Predicate) {
  isl_ctx *Context = isl_map_get_ctx(LeftValue);
  unsigned NumberOfParameters = isl_map_n_param(LeftValue);

  isl_map *MapToLHS = MapValueToLHS(Context, NumberOfParameters);
  isl_map *MapToRHS = MapValueToRHS(Context, NumberOfParameters);

  isl_map *LeftValueAtLHS = isl_map_apply_range(LeftValue, MapToLHS);
  isl_map *RightValueAtRHS = isl_map_apply_range(RightValue, MapToRHS);

  isl_map *BothValues = isl_map_intersect(LeftValueAtLHS, RightValueAtRHS);
  isl_set *Comparison = getComparison(Context, Predicate, NumberOfParameters);

  isl_map *ComparedValues = isl_map_intersect_range(BothValues, Comparison);
  return isl_map_domain(ComparedValues);
}

isl_set *SCoPStmt::toConditionSet(const Comparison &Comp, isl_dim *dim) const {
  isl_map *LHSValue = getValueOf(*Comp.getLHS(), this, dim);
  isl_map *RHSValue = getValueOf(*Comp.getRHS(), this, dim);

  return compareValues(LHSValue, RHSValue, Comp.getPred());
}

isl_set *SCoPStmt::toUpperLoopBound(const SCEVAffFunc &UpperBound, isl_dim *dim,
				    unsigned BoundedDimension) const {
  // Set output dimension to bounded dimension.
  isl_dim *RHSDim = isl_dim_alloc(Parent.getCtx(), getNumParams(),
                                  getNumIterators(), 1);
  RHSDim = isl_dim_set_tuple_name(RHSDim, isl_dim_in, getBaseName());
  isl_constraint *c = isl_equality_alloc(isl_dim_copy(RHSDim));
  isl_int v;
  isl_int_init(v);
  isl_int_set_si(v, 1);
  isl_constraint_set_coefficient(c, isl_dim_in, BoundedDimension, v);
  isl_int_set_si(v, -1);
  isl_constraint_set_coefficient(c, isl_dim_out, 0, v);
  isl_int_clear(v);
  isl_basic_map *bmap = isl_basic_map_universe(RHSDim);
  bmap = isl_basic_map_add_constraint(bmap, c);

  isl_map *LHSValue = isl_map_from_basic_map(bmap);

  isl_map *RHSValue = getValueOf(UpperBound, this, dim);

  return compareValues(LHSValue, RHSValue, ICmpInst::ICMP_SLE);
}

void SCoPStmt::buildIterationDomainFromLoops(TempSCoP &tempSCoP) {
  isl_dim *dim = isl_dim_set_alloc(Parent.getCtx(), getNumParams(),
                                   getNumIterators());
  dim = isl_dim_set_tuple_name(dim, isl_dim_set, getBaseName());

  Domain = isl_set_universe(isl_dim_copy(dim));

  isl_int v;
  isl_int_init(v);

  for (int i = 0, e = getNumIterators(); i != e; ++i) {
    // Lower bound: IV >= 0.
    isl_basic_set *bset = isl_basic_set_universe(isl_dim_copy(dim));
    isl_constraint *c = isl_inequality_alloc(isl_dim_copy(dim));
    isl_int_set_si(v, 1);
    isl_constraint_set_coefficient(c, isl_dim_set, i, v);
    bset = isl_basic_set_add_constraint(bset, c);
    Domain = isl_set_intersect(Domain, isl_set_from_basic_set(bset));

    // Upper bound: IV <= NumberOfIterations.
    const Loop *L = getSCEVForDimension(i)->getLoop();
    const SCEVAffFunc &UpperBound = tempSCoP.getLoopBound(L);
    isl_set *UpperBoundSet = toUpperLoopBound(UpperBound, isl_dim_copy(dim), i);
    Domain = isl_set_intersect(Domain, UpperBoundSet);
  }

  isl_int_clear(v);
}

void SCoPStmt::addConditionsToDomain(TempSCoP &tempSCoP,
                                     const Region &CurRegion) {
  isl_dim *dim = isl_set_get_dim(Domain);
  const Region *TopR = tempSCoP.getMaxRegion().getParent(),
               *CurR = &CurRegion;
  const BasicBlock *CurEntry = BB;

  // Build BB condition constrains, by traveling up the region tree.
  do {
    assert(CurR && "We exceed the top region?");
    // Skip when multiple regions share the same entry.
    if (CurEntry != CurR->getEntry()) {
      if (const BBCond *Cnd = tempSCoP.getBBCond(CurEntry))
        for (BBCond::const_iterator I = Cnd->begin(), E = Cnd->end();
             I != E; ++I) {
          isl_set *c = toConditionSet(*I, dim);
          Domain = isl_set_intersect(Domain, c);
        }
    }
    CurEntry = CurR->getEntry();
    CurR = CurR->getParent();
  } while (TopR != CurR);

  isl_dim_free(dim);
}

void SCoPStmt::buildIterationDomain(TempSCoP &tempSCoP, const Region &CurRegion)
{
  buildIterationDomainFromLoops(tempSCoP);
  addConditionsToDomain(tempSCoP, CurRegion);
}

SCoPStmt::SCoPStmt(SCoP &parent, TempSCoP &tempSCoP,
                   const Region &CurRegion, BasicBlock &bb,
                   SmallVectorImpl<Loop*> &NestLoops,
                   SmallVectorImpl<unsigned> &Scatter)
  : Parent(parent), BB(&bb), IVS(NestLoops.size()) {
  // Setup the induction variables.
  for (unsigned i = 0, e = NestLoops.size(); i < e; ++i) {
    PHINode *PN = NestLoops[i]->getCanonicalInductionVariable();
    assert(PN && "Non canonical IV in SCoP!");
    IVS[i] = PN;
  }

  raw_string_ostream OS(BaseName);
  WriteAsOperand(OS, &bb, false);
  BaseName = OS.str();

  buildIterationDomain(tempSCoP, CurRegion);
  buildScattering(Scatter);
  buildAccesses(tempSCoP, CurRegion);
}

SCoPStmt::SCoPStmt(SCoP &parent, SmallVectorImpl<unsigned> &Scatter)
  : Parent(parent), BB(NULL), IVS(0) {

  BaseName = "FinalRead";

  // Build iteration domain.
  std::string IterationDomainString = "{[i0] : i0 = 0}";
  Domain = isl_set_read_from_str(Parent.getCtx(), IterationDomainString.c_str(),
                                 -1);
  Domain = isl_set_add_dims(Domain, isl_dim_param, Parent.getNumParams());
  Domain = isl_set_set_tuple_name(Domain, getBaseName());

  // Build scattering.
  unsigned ScatDim = Parent.getMaxLoopDepth() * 2 + 1;
  isl_dim *dim = isl_dim_alloc(Parent.getCtx(), Parent.getNumParams(), 1,
                               ScatDim);
  dim = isl_dim_set_tuple_name(dim, isl_dim_out, "scattering");
  dim = isl_dim_set_tuple_name(dim, isl_dim_in, getBaseName());
  isl_basic_map *bmap = isl_basic_map_universe(isl_dim_copy(dim));
  isl_int v;
  isl_int_init(v);

  isl_constraint *c = isl_equality_alloc(dim);
  isl_int_set_si(v, -1);
  isl_constraint_set_coefficient(c, isl_dim_out, 0, v);
  isl_int_set_si(v, Scatter[0]);
  isl_constraint_set_constant(c, v);

  bmap = isl_basic_map_add_constraint(bmap, c);
  isl_int_clear(v);
  Scattering = isl_map_from_basic_map(bmap);

  // Build memory accesses.
  std::set<const Value*> BaseAddressSet;

  for (SCoP::const_iterator SI = Parent.begin(), SE = Parent.end(); SI != SE;
       ++SI) {
    SCoPStmt *Stmt = *SI;

    for (MemoryAccessVec::const_iterator I = Stmt->memacc_begin(),
         E = Stmt->memacc_end(); I != E; ++I)
      BaseAddressSet.insert((*I)->getBaseAddr());
  }

  for (std::set<const Value*>::iterator BI = BaseAddressSet.begin(),
       BE = BaseAddressSet.end(); BI != BE; ++BI)
    MemAccs.push_back(new MemoryAccess(*BI, this));
}

unsigned SCoPStmt::getNumParams() const {
  return Parent.getNumParams();
}

unsigned SCoPStmt::getNumIterators() const {
  return IVS.size();
}

unsigned SCoPStmt::getNumScattering() const {
  return isl_map_dim(Scattering, isl_dim_out);
}

const PHINode *SCoPStmt::getInductionVariableForDimension(unsigned Dimension)
  const {
  return IVS[Dimension];
}

const SCEVAddRecExpr *SCoPStmt::getSCEVForDimension(unsigned Dimension)
  const {
  PHINode *PN = IVS[Dimension];
  return cast<SCEVAddRecExpr>(getParent()->getSE()->getSCEV(PN));
}

isl_ctx *SCoPStmt::getIslContext() {
  return Parent.getCtx();
}

SCoPStmt::~SCoPStmt() {
  while (!MemAccs.empty()) {
    delete MemAccs.back();
    MemAccs.pop_back();
  }

  isl_set_free(Domain);
  isl_map_free(Scattering);
}

void SCoPStmt::print(raw_ostream &OS) const {
  OS << "\t" << getBaseName() << "\n";
  isl_printer *p = isl_printer_to_str(Parent.getCtx());

  OS.indent(12) << "Domain:\n";

  if (Domain) {
    isl_printer_print_set(p, Domain);
    OS.indent(16) << isl_printer_get_str(p) << "\n";
  } else
    OS.indent(16) << "n/a\n";

  OS.indent(12) << "Scattering:\n";

  isl_printer_flush(p);

  if (Domain) {
    isl_printer_print_map(p, Scattering);
    OS.indent(16) << isl_printer_get_str(p) << "\n";
  } else
    OS.indent(16) << "n/a\n";

  for (MemoryAccessVec::const_iterator I = MemAccs.begin(), E = MemAccs.end();
      I != E; ++I)
    (*I)->print(OS);

  isl_printer_free(p);
}

void SCoPStmt::dump() const { print(dbgs()); }

//===----------------------------------------------------------------------===//
/// SCoP class implement
SCoP::SCoP(TempSCoP &tempSCoP, LoopInfo &LI, ScalarEvolution &ScalarEvolution)
           : SE(&ScalarEvolution), R(tempSCoP.getMaxRegion()),
           MaxLoopDepth(tempSCoP.getMaxLoopDepth()) {
  isl_ctx *ctx = isl_ctx_alloc();

  ParamSetType &Params = tempSCoP.getParamSet();
  Parameters.insert(Parameters.begin(), Params.begin(), Params.end());

  isl_dim *dim = isl_dim_set_alloc(ctx, getNumParams(), 0);

  // TODO: Insert relations between parameters.
  // TODO: Insert constraints on parameters.
  Context = isl_set_universe (dim);

  SmallVector<Loop*, 8> NestLoops;
  SmallVector<unsigned, 8> Scatter;

  Scatter.assign(MaxLoopDepth + 1, 0);

  // Build the iteration domain, access functions and scattering functions
  // traversing the region tree.
  buildSCoP(tempSCoP, getRegion(), NestLoops, Scatter, LI);
  Stmts.push_back(new SCoPStmt(*this, Scatter));

  assert(NestLoops.empty() && "NestLoops not empty at top level!");
}

SCoP::~SCoP() {
  isl_set_free(Context);

  // Free the statements;
  for (iterator I = begin(), E = end(); I != E; ++I)
    delete *I;

  // Do we need a singleton to manage this?
  //isl_ctx_free(ctx);
}

void SCoP::printContext(raw_ostream &OS) const {
  OS << "Context:\n";

  if (!Context) {
    OS.indent(4) << "n/a\n\n";
    return;
  }

  isl_printer *p = isl_printer_to_str(getCtx());
  isl_printer_print_set(p, Context);
  OS.indent(4) << isl_printer_get_str(p) << "\n";
  isl_printer_free(p);
}

void SCoP::printStatements(raw_ostream &OS) const {
  OS << "Statements {\n";

  for (const_iterator SI = begin(), SE = end();SI != SE; ++SI)
    OS.indent(4) << (**SI);

  OS.indent(4) << "}\n";
}


void SCoP::print(raw_ostream &OS) const {
  printContext(OS.indent(4));
  printStatements(OS.indent(4));
}

void SCoP::dump() const { print(dbgs()); }

isl_ctx *SCoP::getCtx() const { return isl_set_get_ctx(Context); }

ScalarEvolution *SCoP::getSE() const { return SE; }

bool SCoP::isTrivialBB(BasicBlock *BB, TempSCoP &tempSCoP) {
  if (tempSCoP.getAccessFunctions(BB))
    return false;

  return true;
}

void SCoP::buildSCoP(TempSCoP &tempSCoP,
                      const Region &CurRegion,
                      SmallVectorImpl<Loop*> &NestLoops,
                      SmallVectorImpl<unsigned> &Scatter,
                      LoopInfo &LI) {
  Loop *L = castToLoop(CurRegion, LI);

  if (L)
    NestLoops.push_back(L);

  unsigned loopDepth = NestLoops.size();
  assert(Scatter.size() > loopDepth && "Scatter not big enough!");

  for (Region::const_element_iterator I = CurRegion.element_begin(),
       E = CurRegion.element_end(); I != E; ++I)
    if (I->isSubRegion())
      buildSCoP(tempSCoP, *(I->getNodeAs<Region>()), NestLoops, Scatter,
                LI);
    else {
      BasicBlock *BB = I->getNodeAs<BasicBlock>();

      if (isTrivialBB(BB, tempSCoP))
        continue;

      Stmts.push_back(new SCoPStmt(*this, tempSCoP, CurRegion, *BB, NestLoops,
                                   Scatter));

      // Increasing the Scattering function is OK for the moment, because
      // we are using a depth first iterator and the program is well structured.
      ++Scatter[loopDepth];
    }

  if (!L)
    return;

  // Exiting a loop region.
  Scatter[loopDepth] = 0;
  NestLoops.pop_back();
  ++Scatter[loopDepth-1];
}

//===----------------------------------------------------------------------===//

void SCoPInfo::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<LoopInfo>();
  AU.addRequired<RegionInfo>();
  AU.addRequired<ScalarEvolution>();
  AU.addRequired<TempSCoPInfo>();
  AU.setPreservesAll();
}

bool SCoPInfo::runOnRegion(Region *R, RGPassManager &RGM) {
  LoopInfo *LI = &getAnalysis<LoopInfo>();
  ScalarEvolution *SE = &getAnalysis<ScalarEvolution>();

  TempSCoP *tempSCoP = getAnalysis<TempSCoPInfo>().getTempSCoP();

  // This region is no SCoP.
  if (!tempSCoP) {
    scop = 0;
    return false;
  }

  // Statistics.
  ++SCoPFound;
  if (tempSCoP->getMaxLoopDepth() > 0) ++RichSCoPFound;

  scop = new SCoP(*tempSCoP, *LI, *SE);

  return false;
}

char SCoPInfo::ID = 0;


static RegisterPass<SCoPInfo>
X("polly-scops", "Polly - Create polyhedral description of SCoPs");

Pass *polly::createSCoPInfoPass() {
  return new SCoPInfo();
}
