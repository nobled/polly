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

#include "isl_constraint.h"

using namespace llvm;
using namespace polly;

STATISTIC(SCoPFound,  "Number of valid SCoPs");
STATISTIC(RichSCoPFound,   "Number of SCoPs containing a loop");

//===----------------------------------------------------------------------===//
MemoryAccess::~MemoryAccess() {
  isl_map_free(getAccessFunction());
}

MemoryAccess::MemoryAccess(const SCEVAffFunc &AffFunc,
                           SmallVectorImpl<Loop*> &NestLoops,
                           SCoP &S, ScalarEvolution &SE,
                           const char *IteratorName) {
  isl_dim *dim = isl_dim_alloc(S.getCtx(), S.getNumParams(),
                               NestLoops.size(), 1);
  BaseAddr = AffFunc.getBaseAddr();

  std::string name;
  raw_string_ostream OS(name);
  WriteAsOperand(OS, getBaseAddr(), false);
  name = OS.str();

  dim = isl_dim_set_tuple_name(dim, isl_dim_out, name.c_str());
  dim = isl_dim_set_tuple_name(dim, isl_dim_in, IteratorName);

  isl_basic_map *bmap = isl_basic_map_universe(dim);
  isl_constraint *c;

  c = toAccessFunction(AffFunc, dim, NestLoops, S.getParams(), SE);
  bmap = isl_basic_map_add_constraint(bmap, c);
  AccessRelation = isl_map_from_basic_map(bmap);

  Type = AffFunc.isRead() ? Read : Write;

}

static void setCoefficient(const SCEV *Coeff, mpz_t v, bool negative,
                           bool isSigned = true) {
  if (Coeff) {
    const SCEVConstant *C = dyn_cast<SCEVConstant>(Coeff);
    const APInt &CI = C->getValue()->getValue();
    MPZ_from_APInt(v, negative ? (-CI) : CI, isSigned);
  } else
    isl_int_set_si(v, 0);
}

isl_constraint *MemoryAccess::toAccessFunction(const SCEVAffFunc &AffFunc,
  isl_dim* dim, const SmallVectorImpl<Loop*> &NestLoops,
  const SmallVectorImpl<const SCEV*> &Params, ScalarEvolution &SE) const {

  isl_constraint *c = isl_equality_alloc(isl_dim_copy(dim));
  isl_int v;
  isl_int_init(v);

  isl_int_set_si(v, 1);
  isl_constraint_set_coefficient(c, isl_dim_out, 0, v);

  // Do not touch the current iterator.
  for (unsigned i = 0, e = NestLoops.size(); i != e; ++i) {
    Loop *L = NestLoops[i];
    Value *IndVar = L->getCanonicalInductionVariable();
    setCoefficient(AffFunc.getCoeff(SE.getSCEV(IndVar)), v, true);
    isl_constraint_set_coefficient(c, isl_dim_in, i, v);
  }

  // Setup the coefficient of parameters
  for (unsigned i = 0, e = Params.size(); i != e; ++i) {
    setCoefficient(AffFunc.getCoeff(Params[i]), v, true);
    isl_constraint_set_coefficient(c, isl_dim_param, i, v);
  }

  // Set the const.
  setCoefficient(AffFunc.getTransComp(), v, true);
  isl_constraint_set_constant(c, v);
  isl_int_clear(v);

  return c;
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
void SCoPStmt::buildScattering(SmallVectorImpl<unsigned> &Scatter,
                               unsigned CurLoopDepth) {
  unsigned ScatDim = Parent.getMaxLoopDepth() * 2 + 1;
  isl_dim *dim = isl_dim_alloc(Parent.getCtx(), Parent.getNumParams(),
                                 CurLoopDepth, ScatDim);
  dim = isl_dim_set_tuple_name(dim, isl_dim_out, "scattering");
  dim = isl_dim_set_tuple_name(dim, isl_dim_in, getBaseName());
  isl_basic_map *bmap = isl_basic_map_universe(isl_dim_copy(dim));
  isl_int v;
  isl_int_init(v);

  // Loop dimensions.
  for (unsigned i = 0; i < CurLoopDepth; ++i) {
    isl_constraint *c = isl_equality_alloc(isl_dim_copy(dim));
    isl_int_set_si(v, 1);
    isl_constraint_set_coefficient(c, isl_dim_out, 2 * i + 1, v);
    isl_int_set_si(v, -1);
    isl_constraint_set_coefficient(c, isl_dim_in, i, v);

    bmap = isl_basic_map_add_constraint(bmap, c);
  }

  // Constant dimensions
  for (unsigned i = 0; i < CurLoopDepth + 1; ++i) {
    isl_constraint *c = isl_equality_alloc(isl_dim_copy(dim));
    isl_int_set_si(v, -1);
    isl_constraint_set_coefficient(c, isl_dim_out, 2 * i, v);
    isl_int_set_si(v, Scatter[i]);
    isl_constraint_set_constant(c, v);

    bmap = isl_basic_map_add_constraint(bmap, c);
  }

  // Fill scattering dimensions.
  for (unsigned i = 2 * CurLoopDepth + 1; i < ScatDim ; ++i) {
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

void SCoPStmt::buildAccesses(TempSCoP &tempSCoP, const Region &CurRegion,
                             ScalarEvolution &SE, SmallVectorImpl<Loop*>
                             &NestLoops) {
  const AccFuncSetType *AccFuncs = tempSCoP.getAccessFunctions(BB);

  for (AccFuncSetType::const_iterator I = AccFuncs->begin(),
       E = AccFuncs->end(); I != E; ++I)
    MemAccs.push_back(new MemoryAccess(*I, NestLoops, Parent, SE,
                                       getBaseName()));
}

isl_constraint *SCoPStmt::toConditionConstrain(const SCEVAffFunc &AffFunc,
  isl_dim *dim,
  const SmallVectorImpl<const SCEVAddRecExpr*> &IndVars,
  const SmallVectorImpl<const SCEV*> &Params) const {

  unsigned num_in = IndVars.size(), num_param = Params.size();

  isl_constraint *c = 0;
  if (AffFunc.getType() == SCEVAffFunc::GE)
    c = isl_inequality_alloc(isl_dim_copy(dim));
  else // "!= 0" and "== 0".
    c = isl_equality_alloc(isl_dim_copy(dim));

  isl_int v;
  isl_int_init(v);

  // Set the coefficient for induction variables.
  for (unsigned i = 0, e = num_in; i != e; ++i) {
    setCoefficient(AffFunc.getCoeff(IndVars[i]), v, false, AffFunc.isSigned());
    isl_constraint_set_coefficient(c, isl_dim_set, i, v);
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

  return c;
}

isl_set *SCoPStmt::toConditionSet(const SCEVAffFunc &AffFunc,
  isl_dim *dim, const SmallVectorImpl<const SCEVAddRecExpr*> &IndVars,
  const SmallVectorImpl<const SCEV*> &Params) const {

  isl_basic_set *bset = isl_basic_set_universe(isl_dim_copy(dim));
  isl_constraint *c = toConditionConstrain(AffFunc, dim, IndVars, Params);
  bset = isl_basic_set_add_constraint(bset, c);
  isl_set *ret = isl_set_from_basic_set(bset);

  if (AffFunc.getType() == SCEVAffFunc::Ne) {
    // Invert the equal condition to get the not equal condition.
    ret = isl_set_complement(ret);
    DEBUG(dbgs() << "Ne:\n");
    DEBUG(isl_set_print(ret, stderr, 8, ISL_FORMAT_ISL));
  }

  return ret;
}


void SCoPStmt::buildIterationDomainFromLoops(TempSCoP &tempSCoP,
                                             IndVarVec &IndVars) {
  isl_dim *dim = isl_dim_set_alloc(Parent.getCtx(), Parent.getNumParams(),
                                     IndVars.size());
  dim = isl_dim_set_tuple_name(dim, isl_dim_set, getBaseName());
  isl_basic_set *bset = isl_basic_set_universe(dim);

  isl_int v;
  isl_int_init(v);

  // Loop bounds.
  for (int i = 0, e = IndVars.size(); i != e; ++i) {
    const SCEVAffFunc &bound = tempSCoP.getLoopBound(IndVars[i]->getLoop());
    isl_constraint *c = isl_inequality_alloc(isl_dim_copy(dim));

    // Lower bound: IV >= 0.
    isl_int_set_si(v, 1);
    isl_constraint_set_coefficient(c, isl_dim_set, i, v);
    bset = isl_basic_set_add_constraint(bset, c);

    // Upper bound: IV <= NumberOfIterations.
    c = toConditionConstrain(bound, dim, IndVars, Parent.getParams());
    isl_int_set_si(v, -1);
    isl_constraint_set_coefficient(c, isl_dim_set, i, v);
    bset = isl_basic_set_add_constraint(bset, c);
  }

  isl_int_clear(v);
  Domain = isl_set_from_basic_set(bset);
}

void SCoPStmt::addConditionsToDomain(TempSCoP &tempSCoP,
                                     const Region &CurRegion,
                                     IndVarVec &IndVars) {
  isl_dim *dim = isl_set_get_dim(Domain);
  const Region *TopR = tempSCoP.getMaxRegion().getParent(),
               *CurR = &CurRegion;
  const BasicBlock *CurEntry = BB;

  // Build BB condition constrains, by traveling up the region tree.
  do {
    assert(CurR && "We exceed the top region?");
    // Skip when multiple regions share the same entry.
    if (CurEntry != CurR->getEntry())
      if (const BBCond *Cnd = tempSCoP.getBBCond(CurEntry))
        for (BBCond::const_iterator I = Cnd->begin(), E = Cnd->end();
             I != E; ++I) {
          isl_set *c = toConditionSet(*I,dim, IndVars, Parent.getParams());
          Domain = isl_set_intersect(Domain, c);
        }
    CurEntry = CurR->getEntry();
    CurR = CurR->getParent();
  } while (TopR != CurR);

  isl_dim_free(dim);
}

void SCoPStmt::buildIterationDomain(TempSCoP &tempSCoP,
                                const Region &CurRegion,
                                ScalarEvolution &SE)
{
  IndVarVec IndVars(IVS.size());

  // Setup the induction variables.
  for (unsigned i = 0, e = IVS.size(); i < e; ++i) {
    PHINode *PN = IVS[i];
    IndVars[i] = cast<SCEVAddRecExpr>(SE.getSCEV(PN));
  }

  buildIterationDomainFromLoops(tempSCoP, IndVars);
  addConditionsToDomain(tempSCoP, CurRegion, IndVars);
}

SCoPStmt::SCoPStmt(SCoP &parent, TempSCoP &tempSCoP,
                   const Region &CurRegion, BasicBlock &bb,
                   SmallVectorImpl<Loop*> &NestLoops,
                   SmallVectorImpl<unsigned> &Scatter,
                   ScalarEvolution &SE)
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

  buildIterationDomain(tempSCoP, CurRegion, SE);
  buildScattering(Scatter, NestLoops.size());
  buildAccesses(tempSCoP, CurRegion, SE, NestLoops);

}
unsigned SCoPStmt::getNumParams() {
  return isl_set_n_param(Domain);
}

unsigned SCoPStmt::getNumIterators() {
  return isl_set_n_dim(Domain);
}

unsigned SCoPStmt::getNumScattering() {
  return isl_map_dim(Scattering, isl_dim_out);
}

PHINode *SCoPStmt::getIVatLevel(unsigned L) {
  return IVS[L];
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
  OS << "\t" << BB->getNameStr() << "\n";
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
SCoP::SCoP(TempSCoP &tempSCoP, LoopInfo &LI, ScalarEvolution &SE)
           : R(tempSCoP.getMaxRegion()),
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
  buildSCoP(tempSCoP, getRegion(), NestLoops, Scatter, LI, SE);

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

bool SCoP::isTrivialBB(BasicBlock *BB, TempSCoP &tempSCoP) {
  if (tempSCoP.getAccessFunctions(BB))
    return false;

  return true;
}

void SCoP::buildSCoP(TempSCoP &tempSCoP,
                      const Region &CurRegion,
                      SmallVectorImpl<Loop*> &NestLoops,
                      SmallVectorImpl<unsigned> &Scatter,
                      LoopInfo &LI, ScalarEvolution &SE) {
  Loop *L = castToLoop(CurRegion, LI);

  if (L)
    NestLoops.push_back(L);

  unsigned loopDepth = NestLoops.size();
  assert(Scatter.size() > loopDepth && "Scatter not big enough!");

  for (Region::const_element_iterator I = CurRegion.element_begin(),
       E = CurRegion.element_end(); I != E; ++I)
    if (I->isSubRegion())
      buildSCoP(tempSCoP, *(I->getNodeAs<Region>()), NestLoops, Scatter,
                LI, SE);
    else {
      BasicBlock *BB = I->getNodeAs<BasicBlock>();

      if (isTrivialBB(BB, tempSCoP))
        continue;

      Stmts.push_back(new SCoPStmt(*this, tempSCoP, CurRegion, *BB, NestLoops,
                                   Scatter, SE));

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
