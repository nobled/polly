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
#include "polly/Support/GmpConv.h"

#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/RegionIterator.h"
#include "llvm/Analysis/ScalarEvolutionExpressions.h"
#include "llvm/Assembly/Writer.h"
#include "llvm/Support/CFG.h"
#include "llvm/Support/CommandLine.h"

#define DEBUG_TYPE "polly-scop-info"
#include "llvm/Support/Debug.h"

#include "isl_constraint.h"

using namespace llvm;
using namespace polly;

STATISTIC(ValidRegion,"Number of regions that a valid part of SCoP");

STATISTIC(SCoPFound,  "Number of valid SCoP");

STATISTIC(RichSCoPFound,   "Number of SCoP has loop inside");

//===----------------------------------------------------------------------===//
/// Help function to build isl objects

static __isl_give
polly_map *buildScattering(__isl_keep polly_ctx *ctx, unsigned ParamDim,
                           SmallVectorImpl<unsigned> &Scatter,
                           unsigned ScatDim, unsigned CurLoopDepth) {
  // FIXME: No parameter need?
  polly_dim *dim = isl_dim_alloc(ctx, ParamDim, CurLoopDepth, ScatDim);
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
  return isl_map_from_basic_map(bmap);
}

static __isl_give
polly_set *buildIterateDomain(SCoP &SCoP, TempSCoP &tempSCoP,
                              const Region &CurRegion, const BasicBlock &CurBB,
                              ScalarEvolution &SE,
                              SmallVectorImpl<const SCEVAddRecExpr*> &IndVars) {
  // Create the basic set;
  polly_dim *dim = isl_dim_set_alloc(SCoP.getCtx(), SCoP.getNumParams(),
                                    IndVars.size());
  polly_basic_set *bset = isl_basic_set_universe(isl_dim_copy(dim));
  polly_set *dom = isl_set_from_basic_set(bset);

  for (int i = 0, e = IndVars.size(); i != e; ++i) {
    const BBCond &bounds = tempSCoP.getLoopBound(IndVars[i]->getLoop());
    // Build the constrain of bounds
    polly_set *lb = bounds[0].toConditionConstrain(SCoP.getCtx(),
      dim, IndVars, SCoP.getParams());
    dom = isl_set_intersect(dom, lb);
    polly_set *ub = bounds[1].toConditionConstrain(SCoP.getCtx(),
      dim, IndVars, SCoP.getParams());

    dom = isl_set_intersect(dom, ub);
  }

  // Build the BB condition constrains, by travel up the region tree.
  // NOTE: These are only a temporary hack.
  const Region *TopR = tempSCoP.getMaxRegion().getParent(),
               *CurR = &CurRegion;
  const BasicBlock *CurEntry = &CurBB;
  do {
    assert(CurR && "We exceed the top region?");
    // Skip when multiple regions share the same entry
    if (CurEntry != CurR->getEntry())
      if (const BBCond *Cnd = tempSCoP.getBBCond(CurEntry))
        for (BBCond::const_iterator I = Cnd->begin(), E = Cnd->end(); I != E; ++I) {
          polly_set *c = (*I).toConditionConstrain(SCoP.getCtx(), dim,
            IndVars, SCoP.getParams());
          dom = isl_set_intersect(dom, c);
        }
    //
    CurEntry = CurR->getEntry();
    CurR = CurR->getParent();
  } while (TopR != CurR);

  isl_dim_free(dim);
  return dom;
}

//===----------------------------------------------------------------------===//
DataRef::~DataRef() {
  isl_map_free(getAccessFunction());
}

void DataRef::print(raw_ostream &OS) const {
   // Print BaseAddr
  OS << (isRead() ? "Reads" : "Writes") << " ";
  if (isScalar())
    OS << *getScalar() << "\n";
  else {
    WriteAsOperand(OS, getBaseAddr(), false);
    OS << " at:\n";
    isl_map_print(getAccessFunction(), stderr, 20, ISL_FORMAT_ISL);
    DEBUG(OS << "\n");
    DEBUG(isl_map_dump(getAccessFunction(), stderr, 20));
  }
}

void DataRef::dump() const {
  print(errs());
}
//===----------------------------------------------------------------------===//
SCoPStmt::SCoPStmt(SCoP &parent, TempSCoP &tempSCoP,
                   const Region &CurRegion, BasicBlock &bb,
                   SmallVectorImpl<Loop*> &NestLoops,
                   SmallVectorImpl<unsigned> &Scatter,
                   ScalarEvolution &SE)
  : Parent(parent), BB(&bb), IVS(NestLoops.size()) {
  SmallVector<const SCEVAddRecExpr*, 8> SIVS(NestLoops.size());
  // Setup the induction variables
  for (unsigned i = 0, e = NestLoops.size(); i < e; ++i) {
    PHINode *PN = NestLoops[i]->getCanonicalInductionVariable();
    assert(PN && "SCoPDetect never allow loop without CanIV in SCoP!");
    IVS[i] = PN;
    SIVS[i] = cast<SCEVAddRecExpr>(SE.getSCEV(PN));
  }

  polly_ctx *ctx = parent.getCtx();

  // Build the iterate domain
  Domain = buildIterateDomain(parent, tempSCoP, CurRegion, *BB,
                                             SE, SIVS);

  DEBUG(std::cerr << "\n\nIterate domain of BB: " << bb.getNameStr() << " is:\n");
  DEBUG(isl_set_print(Domain, stderr, 20, ISL_FORMAT_ISL));
  DEBUG(std::cerr << std::endl);

  // Build the scattering function
  DEBUG(dbgs() << "For BB: " << bb.getName() << " ScatDim: " << Scatter.size()
    << " LoopDepth: " << NestLoops.size() << " { ");
  DEBUG(
    for (unsigned i = 0, e = Scatter.size(); i != e; ++i)
      dbgs() << Scatter[i] << ", ";
  dbgs() << "}\n";
  );
  Scattering = buildScattering(ctx, parent.getNumParams(),
                                          Scatter,
                                          parent.getMaxLoopDepth() * 2 + 1,
                                          NestLoops.size());
  DEBUG(std::cerr << "\nScattering:\n");
  DEBUG(isl_map_print(Scattering, stderr, 20, ISL_FORMAT_ISL));
  DEBUG(std::cerr << std::endl);

  // Build the access function
  if (const AccFuncSetType *AccFuncs = tempSCoP.getAccessFunctions(BB)) {
    // At this moment, getelementptr translate multiple dimension to
    // one dimension.
    polly_dim *dim = isl_dim_alloc(ctx, parent.getNumParams(),
                                    NestLoops.size(), 1);
    for (AccFuncSetType::const_iterator I=AccFuncs->begin(), E=AccFuncs->end();
        I != E; ++I) {
      const SCEVAffFunc &AffFunc = *I;
      polly_basic_map *bmap = isl_basic_map_universe(isl_dim_copy(dim));

      bmap = isl_basic_map_add_constraint(bmap,
        AffFunc.toAccessFunction(ctx, dim, NestLoops, parent.getParams(), SE));

      polly_map *map = isl_map_from_basic_map(bmap);

      // Create the access function and add it to the access function list
      // of the statement.
      DataRef *access = new DataRef(AffFunc.getBaseAddr(),
        AffFunc.isRead() ? DataRef::Read : DataRef::Write, map);

      DEBUG(dbgs() << "Translate access function:\n");
      DEBUG(AffFunc.print(dbgs(), &SE));
      DEBUG(dbgs() << "\nto:\n");
      DEBUG(isl_map_print(map, stderr, 20, ISL_FORMAT_ISL));
      DEBUG(std::cerr << std::endl);

      MemAccs.push_back(access);
    }

  }
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
  }
  else
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

  for (DataRefVec::const_iterator I = MemAccs.begin(), E = MemAccs.end();
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
  // Create the context
  ctx = isl_ctx_alloc();

  ParamSetType &Params = tempSCoP.getParamSet();
  // Initialize parameters
  for (ParamSetType::const_iterator I = Params.begin(), E = Params.end();
      I != E ; ++I)
    Parameters.push_back(*I);

  // Create the dim with 0 output?
  polly_dim *dim = isl_dim_set_alloc(ctx, getNumParams(), 0);
  // TODO: Handle the constrain of parameters.
  // Take the dim.
  Context = isl_set_universe (dim);

  // Build the SCoP.
  SmallVector<Loop*, 8> NestLoops;
  SmallVector<unsigned, 8> Scatter;

  unsigned numScatter = MaxLoopDepth + 1;
  // Initialize the scattering function
  Scatter.assign(numScatter, 0);

  // Build the iterate domain, access functions and scattering functions
  // by traverse the region tree.
  buildSCoP(tempSCoP, getRegion(), NestLoops, Scatter, LI, SE);

  assert(NestLoops.empty() && "NestLoops not empty at top level!");
}

SCoP::~SCoP() {
  // Free the context
  isl_set_free(Context);
  // Free the statements;
  for (iterator I = begin(), E = end(); I != E; ++I)
    delete *I;
  // We need a singleton to manage this?
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
  OS << "SCoP: " << R.getNameStr() << "\tParameters: (";
  // Print Parameters.
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

  // TODO: scattering function for non-linear CFG
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

      // Build the statement
      SCoPStmt *stmt = new SCoPStmt(*this, tempSCoP, CurRegion, *BB,
                                    NestLoops, Scatter, SE);

      // Add the new statement to statements list.
      Stmts.push_back(stmt);

      // Increasing the Scattering function is ok for at the moment, because
      // we are using a depth iterator and the program is linear
      ++Scatter[loopDepth];
    }

  if (L) {
    // Clear the scatter function when leaving the loop.
    Scatter[loopDepth] = 0;
    NestLoops.pop_back();
    // To next loop
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
  // Only build the SCoP when the temporary SCoP information is avaliable.
  if (TempSCoP *tempSCoP = getAnalysis<TempSCoPInfo>().getTempSCoP()) {
    // A SCoP found
    ++SCoPFound;

    // The SCoP have loop inside
    if (tempSCoP->getMaxLoopDepth() > 0) ++RichSCoPFound;

    // Create the scop.
    scop = new SCoP(*tempSCoP,
                    getAnalysis<LoopInfo>(),
                    getAnalysis<ScalarEvolution>());
  }

  return false;
}

char SCoPInfo::ID = 0;

static RegisterPass<SCoPInfo>
X("polly-scop-info", "Polly - Create polyhedral SCoPs Info");

Pass *polly::createSCoPInfoPass() {
  return new SCoPInfo();
}
