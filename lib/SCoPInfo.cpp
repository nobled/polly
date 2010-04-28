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

#include "llvm/Analysis/AffineSCEVIterator.h"
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
  for (StmtSet::iterator I = Stmts.begin(), E = Stmts.end(); I != E; ++I)
    delete *I;
  // We need a singleton to manage this?
  //isl_ctx_free(ctx);
}

void SCoP::print(raw_ostream &OS) const {
  OS << "SCoP: " << R.getNameStr() << "\tParameters: (";
  // Print Parameters.
  for (const_param_iterator PI = param_begin(), PE = param_end();
      PI != PE; ++PI)
    OS << **PI << ", ";

  OS << "), Max Loop Depth: "<< MaxLoopDepth <<"\n";


}
//===----------------------------------------------------------------------===//
/// Help function to build isl objects

static void setCoefficient(const SCEV *Coeff, mpz_t v, bool isLower) {
  if (Coeff) { // If the coefficient exist
    const SCEVConstant *C = dyn_cast<SCEVConstant>(Coeff);
    const APInt &CI = C->getValue()->getValue();
    DEBUG(errs() << "Setting Coeff: " << CI);
    DEBUG(errs() << " neg: " << -CI <<"\n" );
    // Convert i >= expr to i - expr >= 0
    MPZ_from_APInt(v, isLower?(-CI):CI);
  }
  else
    isl_int_set_si(v, 0);
}

static __isl_give
polly_constraint *toLoopBoundConstrain(__isl_keep polly_ctx *ctx,
                                       __isl_keep polly_dim *dim, SCEVAffFunc &func,
                                       const SmallVectorImpl<const SCEV*> &IndVars,
                                       const SmallVectorImpl<const SCEV*> &Params,
                                       bool isLower) {
  unsigned num_in = IndVars.size(),
           num_param = Params.size();

  polly_constraint *c = isl_inequality_alloc(isl_dim_copy(dim));
  isl_int v;
  isl_int_init(v);

  // Dont touch the current iterator.
  for (unsigned i = 0, e = num_in - 1; i != e; ++i) {
    setCoefficient(func.getCoeff(IndVars[i]), v, isLower);
    isl_constraint_set_coefficient(c, isl_dim_set, i, v);
  }

  assert(!func.getCoeff(IndVars[num_in - 1]) &&
          "Current iterator should not have any coff.");
  // Set the coefficient of current iterator, convert i <= expr to expr - i >= 0
  isl_int_set_si(v, isLower?1:(-1));
  isl_constraint_set_coefficient(c, isl_dim_set, num_in - 1, v);

  // Set the value of indvar of inner loops?

  // Setup the coefficient of parameters
  for (unsigned i = 0, e = num_param; i != e; ++i) {
    setCoefficient(func.getCoeff(Params[i]), v, isLower);
    isl_constraint_set_coefficient(c, isl_dim_param, i, v);
  }

  // Set the const.
  setCoefficient(func.TransComp, v, isLower);
  isl_constraint_set_constant(c, v);
  // Free v
  isl_int_clear(v);
  return c;
}

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
polly_basic_set *buildIterateDomain(SCoP &SCoP, LLVMSCoP &TempSCoP,
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

    AffBoundType *bounds = TempSCoP.getLoopBound(L);
    assert(bounds && "Can not get loop bound when building statement!");

    // Build the constrain of lower bound
    polly_constraint *lb = toLoopBoundConstrain(SCoP.getCtx(), dim,
      bounds->first, IndVars, SCoP.getParams(), true);

    bset = isl_basic_set_add_constraint(bset, lb);

    polly_constraint *ub = toLoopBoundConstrain(SCoP.getCtx(), dim,
      bounds->second, IndVars, SCoP.getParams(), false);

    bset = isl_basic_set_add_constraint(bset, ub);
  }

  isl_dim_free(dim);
  return bset;
}

void SCoP::buildStmt(LLVMSCoP &TempSCoP, BasicBlock &BB,
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

void SCoP::buildSCoP(LLVMSCoP &TempSCoP,
                      const Region &CurRegion,
                      SmallVectorImpl<Loop*> &NestLoops,
                      SmallVectorImpl<unsigned> &Scatter,
                      LoopInfo &LI, ScalarEvolution &SE) {
  Loop *L = castToLoop(&CurRegion, &LI);

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
  LLVMSCoP *TempSCoP = getAnalysis<SCoPDetection>().getTempSCoPFor(R);

  if (!TempSCoP) return false;

  SmallVector<Loop*, 8> NestLoops;
  SmallVector<unsigned, 8> Scatter;

  // Create the scop.
  scop = new SCoP(TempSCoP->R, TempSCoP->MaxLoopDepth,
                  TempSCoP->Params.begin(), TempSCoP->Params.end());

  unsigned numScatter = TempSCoP->MaxLoopDepth + 1;
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
