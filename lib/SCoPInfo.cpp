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


static cl::opt<bool>
PrintAccessFunctions("print-access-functions", cl::Hidden,
               cl::desc("Print the access functions of BBs."));

static cl::opt<bool>
PrintLoopBound("print-loop-bounds", cl::Hidden,
            cl::desc("Print the bounds of loops."));

static cl::opt<bool>
BuildSubSCoP("build-sub-scop", cl::Hidden, cl::desc("Build subSCoPs."));

//===----------------------------------------------------------------------===//
SCoPStmt::SCoPStmt(SCoP &parent, BasicBlock &bb,
                   polly_set *domain, polly_map *scat)
  : Parent(parent), BB(bb), Domain(domain), Scattering(scat) {
    assert(Domain && Scattering && "Domain and Scattering can not be null!");
}

SCoPStmt::~SCoPStmt() {
  isl_set_free(Domain);
  isl_map_free(Scattering);
}

void SCoPStmt::print(raw_ostream &OS) const {
  OS << "\tStatement " << BB.getNameStr() << ":\n";

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
}

void SCoPStmt::dump() const { print(dbgs()); }

//===----------------------------------------------------------------------===//
/// SCoP class implement
template<class It>
SCoP::SCoP(Region &r, unsigned maxLoopDepth, It ParamBegin, It ParamEnd)
           : R(r), MaxLoopDepth(maxLoopDepth) {
   // Create the context
   ctx = isl_ctx_alloc();

   // Initialize parameters
   for (It I = ParamBegin, E = ParamEnd; I != E ; ++I)
     Parameters.push_back(*I);

   // Create the dim with 0 parameters
   polly_dim *dim = isl_dim_set_alloc(ctx, 0, getNumParams());
   // TODO: Handle the constrain of parameters.
   // Take the dim.
   Context = isl_set_universe (dim);
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
  // Access function

  // Instert the statement
  Stmts.insert(new SCoPStmt(*this, BB, Domain, Scattering));
}

void SCoP::buildSCoP(TempSCoP &TempSCoP,
                      const Region &CurRegion,
                      SmallVectorImpl<Loop*> &NestLoops,
                      SmallVectorImpl<unsigned> &Scatter,
                      LoopInfo &LI, ScalarEvolution &SE) {
  Loop *L = castToLoop(CurRegion, LI);

  if (L)
    NestLoops.push_back(L);

  // TODO: scattering function
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
    // TODO: scattering function
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
  TempSCoP *TempSCoP =
    getAnalysis<SCoPDetection>().getTempSCoPFor(R);

  if (!TempSCoP) return false;

  if (!BuildSubSCoP && !TempSCoP->isMaxSCoP()) return false;

  SmallVector<Loop*, 8> NestLoops;
  SmallVector<unsigned, 8> Scatter;

  ParamSetType &Params = TempSCoP->getParamSet();
  unsigned maxLoopDepth = TempSCoP->getMaxLoopDepth();
  // Create the scop.
  scop = new SCoP(*R, maxLoopDepth, Params.begin(), Params.end());

  unsigned numScatter = maxLoopDepth + 1;
  // Initialize the scattering function
  Scatter.assign(numScatter, 0);

  scop->buildSCoP(*TempSCoP, scop->getRegion(), NestLoops, Scatter,
                  getAnalysis<LoopInfo>(), getAnalysis<ScalarEvolution>());

  assert(NestLoops.empty() && "NestLoops not empty at top level!");

  return false;
}

char SCoPInfo::ID = 0;

static RegisterPass<SCoPInfo>
X("polly-scop-info", "Polly - Create polyhedral SCoPs Info");

Pass *polly::createSCoPInfoPass() {
  return new SCoPInfo();
}
