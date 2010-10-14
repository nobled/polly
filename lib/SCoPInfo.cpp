//===--------- SCoPInfo.cpp  - Create SCoPs from LLVM IR ---------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Create a polyhedral description of the region.
//
//===----------------------------------------------------------------------===//

#include "polly/TempSCoPInfo.h"

#include "polly/SCoPInfo.h"
#include "polly/LinkAllPasses.h"
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

void MemoryAccess::print(raw_ostream &OS) const {
  OS << (isRead() ? "Reads" : "Writes") << " ";
  WriteAsOperand(OS, getBaseAddr(), false);
  OS << " at:\n";
  isl_map_print(getAccessFunction(), stderr, 20, ISL_FORMAT_ISL);
  DEBUG(OS << "\n");
  DEBUG(isl_map_dump(getAccessFunction(), stderr, 20));
}

void MemoryAccess::dump() const {
  print(errs());
}

//===----------------------------------------------------------------------===//
void SCoPStmt::buildScattering(SmallVectorImpl<unsigned> &Scatter,
                               unsigned CurLoopDepth) {
  unsigned ScatDim = Parent.getMaxLoopDepth() * 2 + 1;
  polly_dim *dim = isl_dim_alloc(Parent.getCtx(), Parent.getNumParams(),
                                 CurLoopDepth, ScatDim);
  polly_basic_map *bmap = isl_basic_map_universe(isl_dim_copy(dim));
  isl_int v;
  isl_int_init(v);

  // Loop dimensions.
  for (unsigned i = 0; i < CurLoopDepth; ++i) {
    polly_constraint *c = isl_equality_alloc(isl_dim_copy(dim));
    isl_int_set_si(v, 1);
    isl_constraint_set_coefficient(c, isl_dim_out, 2 * i + 1, v);
    isl_int_set_si(v, -1);
    isl_constraint_set_coefficient(c, isl_dim_in, i, v);

    bmap = isl_basic_map_add_constraint(bmap, c);
  }

  // Constant dimensions
  for (unsigned i = 0; i < CurLoopDepth + 1; ++i) {
    polly_constraint *c = isl_equality_alloc(isl_dim_copy(dim));
    isl_int_set_si(v, -1);
    isl_constraint_set_coefficient(c, isl_dim_out, 2 * i, v);
    isl_int_set_si(v, Scatter[i]);
    isl_constraint_set_constant(c, v);

    bmap = isl_basic_map_add_constraint(bmap, c);
  }

  // Fill scattering dimensions.
  for (unsigned i = 2 * CurLoopDepth + 1; i < ScatDim ; ++i) {
    polly_constraint *c = isl_equality_alloc(isl_dim_copy(dim));
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

  if (!AccFuncs)
    return;

  // At the moment, getelementptr translates multiple dimensions to
  // one dimension.
  polly_dim *dim = isl_dim_alloc(Parent.getCtx(), Parent.getNumParams(),
                                 NestLoops.size(), 1);
  for (AccFuncSetType::const_iterator I = AccFuncs->begin(),
       E = AccFuncs->end(); I != E; ++I) {
    const SCEVAffFunc &AffFunc = *I;
    polly_basic_map *bmap = isl_basic_map_universe(isl_dim_copy(dim));
    polly_constraint *c;

    c = AffFunc.toAccessFunction(dim, NestLoops, Parent.getParams(), SE);
    bmap = isl_basic_map_add_constraint(bmap, c);
    polly_map *map = isl_map_from_basic_map(bmap);

    MemoryAccess::AccessType AccessType;
    if (AffFunc.isRead())
      AccessType = MemoryAccess::Read;
    else
      AccessType = MemoryAccess::Write;

    MemoryAccess *access = new MemoryAccess(AffFunc.getBaseAddr(), AccessType, map);
    MemAccs.push_back(access);
  }
}
void SCoPStmt::buildIterationDomainFromLoops(TempSCoP &tempSCoP,
                                             IndVarVec &IndVars) {
  polly_dim *dim = isl_dim_set_alloc(Parent.getCtx(), Parent.getNumParams(),
                                     IndVars.size());
  polly_basic_set *bset = isl_basic_set_universe(dim);

  isl_int v;
  isl_int_init(v);

  // Loop bounds.
  for (int i = 0, e = IndVars.size(); i != e; ++i) {
    const SCEVAffFunc &bound = tempSCoP.getLoopBound(IndVars[i]->getLoop());
    polly_constraint *c = isl_inequality_alloc(isl_dim_copy(dim));

    // Lower bound: IV >= 0.
    isl_int_set_si(v, 1);
    isl_constraint_set_coefficient(c, isl_dim_set, i, v);
    bset = isl_basic_set_add_constraint(bset, c);

    // Upper bound: IV <= NumberOfIterations.
    c = bound.toConditionConstrain(Parent.getCtx(), dim, IndVars,
                                   Parent.getParams());
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
  polly_dim *dim = isl_set_get_dim(Domain);

  // Build BB condition constrains, by traveling up the region tree.
  // NOTE: This is only a temporary hack.
  const Region *TopR = tempSCoP.getMaxRegion().getParent(),
               *CurR = &CurRegion;
  const BasicBlock *CurEntry = BB;
  do {
    assert(CurR && "We exceed the top region?");
    // Skip when multiple regions share the same entry.
    if (CurEntry != CurR->getEntry())
      if (const BBCond *Cnd = tempSCoP.getBBCond(CurEntry))
        for (BBCond::const_iterator I = Cnd->begin(), E = Cnd->end();
             I != E; ++I) {
          polly_set *c = (*I).toConditionSet(Parent.getCtx(), dim,
                                             IndVars, Parent.getParams());
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
  OS << "\tStatement " << BB->getNameStr() << ":\n";

  OS << "\t\tDomain:\n";
  if (Domain) {
    isl_set_print(Domain, stderr, 20, ISL_FORMAT_ISL);
    DEBUG(OS << "\n");
    DEBUG(isl_set_dump(Domain, stderr, 20));
  } else
    OS << "\t\t\tn/a\n";

  OS << "\n";

  OS << "\t\t Scattering:\n";
  if (Scattering) {
    isl_map_print(Scattering, stderr, 20, ISL_FORMAT_ISL);
    DEBUG(OS << "\n");
    DEBUG(isl_map_dump(Scattering, stderr, 20));
  } else
    OS << "\t\t\tn/a\n";

  OS << "\n";

  for (MemoryAccessVec::const_iterator I = MemAccs.begin(), E = MemAccs.end();
      I != E; ++I) {
    (*I)->print(OS);
    OS << "\n";
  }
}

void SCoPStmt::dump() const { print(dbgs()); }

//===----------------------------------------------------------------------===//
/// SCoP class implement
SCoP::SCoP(TempSCoP &tempSCoP, LoopInfo &LI, ScalarEvolution &SE)
           : R(tempSCoP.getMaxRegion()),
           MaxLoopDepth(tempSCoP.getMaxLoopDepth()) {
  ctx = isl_ctx_alloc();

  ParamSetType &Params = tempSCoP.getParamSet();
  Parameters.insert(Parameters.begin(), Params.begin(), Params.end());

  polly_dim *dim = isl_dim_set_alloc(ctx, getNumParams(), 0);

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
  OS << "\tContext:\n";
  if (Context) {
    isl_set_print(Context, stderr, 12, ISL_FORMAT_ISL);
    DEBUG(isl_set_dump(Context, stderr, 12));
  }
  else
    OS << "\t\tn/a\n";

  OS << "\n";
}

void SCoP::printStatements(raw_ostream &OS) const {
  OS << "Statements {\n";

  for (const_iterator SI = begin(), SE = end();SI != SE; ++SI)
    OS << (**SI) << "\n";

  OS << "}\n";
}


void SCoP::print(raw_ostream &OS) const {
  // Parameters.
  OS << "SCoP: " << R.getNameStr() << "\tParameters: (";
  for (const_param_iterator PI = param_begin(), PE = param_end();
        PI != PE; ++PI)
    OS << **PI << ", ";

  OS << "), Max Loop Depth: "<< MaxLoopDepth <<"\n";

  printContext(OS);
  printStatements(OS);

}

void SCoP::dump() const { print(dbgs()); }

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

  // TODO: Scattering function for non-linear CFG.
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

      // Build the statement.
      SCoPStmt *stmt = new SCoPStmt(*this, tempSCoP, CurRegion, *BB,
                                    NestLoops, Scatter, SE);

      // Add the new statement to statements list.
      Stmts.push_back(stmt);

      // Increasing the Scattering function is OK for the moment, because
      // we are using a depth first iterator and the program is linear.
      ++Scatter[loopDepth];
    }

  if (L) {
    // Clear the scatter function when leaving the loop.
    Scatter[loopDepth] = 0;
    NestLoops.pop_back();
    // To next loop.
    ++Scatter[loopDepth-1];
    // TODO: scattering function for non-linear CFG
  }

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

  // Only build the SCoP, if the temporary SCoP information is available.
  if (TempSCoP *tempSCoP = getAnalysis<TempSCoPInfo>().getTempSCoP()) {
    // Statistics
    ++SCoPFound;
    if (tempSCoP->getMaxLoopDepth() > 0) ++RichSCoPFound;

    scop = new SCoP(*tempSCoP, *LI, *SE);
  } else
    scop = 0;

  return false;
}

char SCoPInfo::ID = 0;


static RegisterPass<SCoPInfo>
X("polly-scops", "Polly - Create polyhedral description of SCoPs");

Pass *polly::createSCoPInfoPass() {
  return new SCoPInfo();
}
