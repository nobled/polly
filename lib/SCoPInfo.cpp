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

#include "SCoPDetection.h"
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
SCoPStmt::SCoPStmt(SCoP &parent, BasicBlock &bb,
                   polly_set *domain, polly_map *scat,
                   const SmallVectorImpl<Loop*> &NestLoops)
  : Parent(parent), BB(&bb), Domain(domain), Scattering(scat),
    IVS(NestLoops.size()) {
    assert(Domain && Scattering && "Domain and Scattering can not be null!");
    for (unsigned i = 0, e = NestLoops.size(); i < e; ++i) {
      PHINode *PN = NestLoops[i]->getCanonicalInductionVariable();
      assert(PN && "SCoPDetect never allow loop without CanIV in SCoP!");
      IVS[i] = PN;
    }
}
unsigned SCoPStmt::getNumParams() {
  return isl_set_n_param(Domain);
}
unsigned SCoPStmt::getNumIterators() {
  return isl_set_n_dim(Domain);
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

  for (MemAccVec::const_iterator I = MemAccs.begin(), E = MemAccs.end();
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
polly_basic_set *buildIterateDomain(SCoP &SCoP, TempSCoP &TempSCoP,
                                    ScalarEvolution &SE,
                                    SmallVectorImpl<Loop*> &NestLoops) {
  // Create the basic set;
  polly_dim *dim = isl_dim_set_alloc(SCoP.getCtx(), SCoP.getNumParams(),
    NestLoops.size());
  polly_basic_set *bset = isl_basic_set_universe(isl_dim_copy(dim));


  SmallVector<const SCEV*, 8> IndVars;

  for (int i = 0, e = NestLoops.size(); i != e; ++i) {
    Loop *L = NestLoops[i];
    Value *IndVar = L->getCanonicalInductionVariable();
    IndVars.push_back(SE.getSCEV(IndVar));

    const AffBoundType *bounds = TempSCoP.getLoopBound(L);
    assert(bounds && "Can not get loop bound when building statement!");

    // Build the constrain of lower bound
    polly_constraint *lb = bounds->first.toLoopBoundConstrain(SCoP.getCtx(),
      dim, IndVars, SCoP.getParams(), true);

    bset = isl_basic_set_add_constraint(bset, lb);

    polly_constraint *ub = bounds->second.toLoopBoundConstrain(SCoP.getCtx(),
      dim, IndVars, SCoP.getParams(), false);

    bset = isl_basic_set_add_constraint(bset, ub);
  }

  isl_dim_free(dim);
  return bset;
}

void SCoP::buildStmt(TempSCoP &TempSCoP, BasicBlock &BB,
                      SmallVectorImpl<Loop*> &NestLoops,
                      SmallVectorImpl<unsigned> &Scatter,
                      ScalarEvolution &SE) {

  polly_basic_set *bset = buildIterateDomain(*this, TempSCoP, SE, NestLoops);

  polly_set *Domain = isl_set_from_basic_set(bset);

  DEBUG(std::cerr << "\n\nIterate domain of BB: " << BB.getNameStr() << " is:\n");
  DEBUG(isl_set_print(Domain, stderr, 20, ISL_FORMAT_ISL));
  DEBUG(std::cerr << std::endl);

  // Scattering function
  DEBUG(dbgs() << "For BB: " << BB.getName() << " ScatDim: " << Scatter.size()
               << " LoopDepth: " << NestLoops.size() << " { ");
  DEBUG(
    for (unsigned i = 0, e = Scatter.size(); i != e; ++i)
      dbgs() << Scatter[i] << ", ";
    dbgs() << "}\n";
    );
  polly_map *Scattering = buildScattering(ctx, getNumParams(),
                                          Scatter,
                                          getScatterDim(),
                                          NestLoops.size());
  DEBUG(std::cerr << "\nScattering:\n");
  DEBUG(isl_map_print(Scattering, stderr, 20, ISL_FORMAT_ISL));
  DEBUG(std::cerr << std::endl);

  // Instert the statement
  SCoPStmt *stmt = new SCoPStmt(*this, BB, Domain, Scattering, NestLoops);

  // Access function
  if (const AccFuncSetType *AccFuncs = TempSCoP.getAccessFunctions(&BB)) {
    // At this moment, getelementptr translate multiple dimension to
    // one dimension.
    polly_dim *dim = isl_dim_alloc(ctx,
                                   Parameters.size(), NestLoops.size(), 1);
    for (AccFuncSetType::const_iterator I=AccFuncs->begin(), E=AccFuncs->end();
        I != E; ++I) {
      const SCEVAffFunc &AffFunc = *I;
      polly_basic_map *bmap = isl_basic_map_universe(isl_dim_copy(dim));

      bmap = isl_basic_map_add_constraint(bmap,
        AffFunc.toAccessFunction(ctx, dim, NestLoops, Parameters, SE));

      polly_map *map = isl_map_from_basic_map(bmap);

      DataRef *access = new DataRef(AffFunc.getBaseAddr(),
        AffFunc.isRead() ? DataRef::Read : DataRef::Write, map);

      DEBUG(dbgs() << "Translate access function:\n");
      DEBUG(AffFunc.print(dbgs(), &SE));
      DEBUG(dbgs() << "\nto:\n");
      DEBUG(isl_map_print(map, stderr, 20, ISL_FORMAT_ISL));
      DEBUG(std::cerr << std::endl);

      stmt->addMemoryAccess(access);
    }

  }

  Stmts.push_back(stmt);
}

PHINode *SCoPStmt::getIVatLevel(unsigned L) {
  return IVS[L];
}

void SCoP::buildSCoP(TempSCoP &TempSCoP,
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
      E = CurRegion.element_end(); I != E; ++I) {
    if (I->isSubRegion())
      buildSCoP(TempSCoP, *(I->getNodeAs<Region>()), NestLoops, Scatter,
                LI, SE);
    else {
      // Build the statement
      buildStmt(TempSCoP, *(I->getNodeAs<BasicBlock>()), NestLoops, Scatter,
                SE);
      // Increasing the Scattering function is ok for at the moment, because
      // we are using a depth iterator and the program is linear
      ++Scatter[loopDepth];
    }
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
  AU.addRequired<SCoPDetection>();
  AU.setPreservesAll();
}

bool SCoPInfo::runOnRegion(Region *R, RGPassManager &RGM) {
  SCoPDetection &SCoPDetect = getAnalysis<SCoPDetection>();

  TempSCoP *tempSCoP = SCoPDetect.getTempSCoPFor(R);

  if (!tempSCoP) return false;

  // A valid region for SCoP found.
  ++ValidRegion;

  // Only analyse the maximal SCoPs.
  if (!SCoPDetect.isMaxRegionInSCoP(*R)) {
    Region *Parent = R->getParent();
    assert(Parent && "Non max region will always have parent!");
    // If the current region is the child of toplevel region,
    // then this region is the maximal region we could handle,
    // because we cannot yet handle complete functions.
    if(Parent->getParent())
      return false;
  }
  // Do not build the scopinfo for toplevel region
  else if (!R->getParent())
      return false;

  // A SCoP found
  ++SCoPFound;

  // The SCoP have loop inside
  if (tempSCoP->getMaxLoopDepth() > 0) ++RichSCoPFound;

  // Create the scop.
  scop = new SCoP(*tempSCoP,
                  getAnalysis<LoopInfo>(),
                  getAnalysis<ScalarEvolution>());

  return false;
}

char SCoPInfo::ID = 0;

static RegisterPass<SCoPInfo>
X("polly-scop-info", "Polly - Create polyhedral SCoPs Info");

Pass *polly::createSCoPInfoPass() {
  return new SCoPInfo();
}
