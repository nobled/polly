//===--------- SCoPInfo.cpp  - Create SCoPs from LLVM IR --------*- C++ -*-===//
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

#include "polly/SCoP/SCoPInfo.h"


#include "llvm/Analysis/AffineSCEVIterator.h"
#include "llvm/Analysis/ScalarEvolutionExpressions.h"
#include "llvm/Support/CommandLine.h"

#define DEBUG_TYPE "polly-scop-info"
#include "llvm/Support/Debug.h"

using namespace llvm;
using namespace polly;

static cl::opt<bool>
PrintLoopParam("print-loop-params", cl::Hidden,
            cl::desc("Print the params used in loops."));

// The pair of (lower_bound, upper_bound),
// where lower_bound <= indVar <= upper_bound
typedef std::pair<const SCEV*, const SCEV*> LoopBounds;

static inline LoopBounds getLoopBounds(const Loop* L, ScalarEvolution* SE) {

  DEBUG(errs() << "Loop: " << L->getHeader()->getName() << "\n");
  const SCEVAddRecExpr *IndVar =
    dyn_cast<SCEVAddRecExpr>(SE->getSCEV(L->getCanonicalInductionVariable()));

  assert(IndVar && "Expect IndVar a SECVAddRecExpr");
  DEBUG(errs() << *IndVar << "\n");

  const SCEV *LB = IndVar->getStart();

  DEBUG(errs() << "Lower bound:" << *LB << "\n");

  const SCEV *UB = SE->getBackedgeTakenCount(L);
  if (!isa<SCEVCouldNotCompute>(UB))
    UB = SE->getAddExpr(LB, UB);

  DEBUG(errs() << "Upper bound:" << *UB << "\n");

  return std::make_pair(LB, UB);
}

bool SCoPInfo::extractParam(const SCEV *S, const Loop *Scope) {
  assert(Scope && "Scope can not be null!");

  if (isa<SCEVCouldNotCompute>(S))
    return false;

  // Compute S at scope first.
  S = SE->getSCEVAtScope(S, Scope);

  ParamSet &Params = ParamsAtScope[Scope];

  // FIXME: Simplify these code.
  for (AffineSCEVIterator I = affine_begin(S, SE), E = affine_end();
      I != E; ++I){
    const SCEV *Var = I->first;

    // Ignore the constant offest.
    if(isa<SCEVConstant>(Var))
      continue;

    if (!Var->isLoopInvariant(Scope)) {
      // If var the induction variable of the loop?
      if (const SCEVAddRecExpr *AddRec = dyn_cast<SCEVAddRecExpr>(Var)){
        assert(AddRec->getLoop() == Scope && "getAtScope not work?");
        continue;
      }
      // A bad SCEV found.
      DEBUG(errs() << "Bad SCEV: " << *S << " in "
                   << Scope->getHeader()->getName() << "\n");
      return false;
    }
    // TODO: handle the paramer0 * parameter1 case.
    // A dirty hack
    assert(isa<SCEVUnknown>(Var) && "Can only process unknow right now.");
    // Add the loop invariants to parameter lists.
    Params.insert(Var);
  }

  return true;
}

void SCoPInfo::visitLoopsInFunction(Function &F, LoopInfo &LI) {
  for (LoopInfo::iterator I = LI.begin(), E = LI.end(); I != E; ++I)
      if (!visitLoop(*I))
        AddBadLoop(*I);

  // TODO: Find Bad Blocks.
}

bool SCoPInfo::visitLoop(const Loop* L) {
  bool isGoodLoop = true;

  // Visit sub loop first
  for (Loop::iterator I = L->begin(), E = L->end(); I != E; ++I) {
    if (!visitLoop(*I)) {
      DEBUG(errs() << "Adding Bad loop: "
                   << (*I)->getHeader()->getName() <<"\n");
      AddBadLoop(*I);
      isGoodLoop = false;
    }
  }

  if (!isGoodLoop)
    return false;

  // Find the parameters used in loop bounds
  LoopBounds bounds = getLoopBounds(L, SE);
  if (!extractParam(bounds.first, L))
    return false;
  if (!extractParam(bounds.second, L))
    return false;

  // Find the parameters used in conditions
  // TODO: Write the code.
  // Find the parameters used in load/store
  // TODO: Write the code.

  // Merge parameters
  // Create the Parameter set for this loop.
  ParamSet &ParamAtLoop = ParamsAtScope[L];
  for (Loop::iterator I = L->begin(), E = L->end(); I != E; ++I) {
    ParamMap::iterator At = ParamsAtScope.find(*I);
    if (At == ParamsAtScope.end())
      continue;
    // Merge the parameters.
    ParamAtLoop.insert(At->second.begin(), At->second.end());
    // Remove the parameters of sub loop.
    ParamsAtScope.erase(At);
  }

  // Remove the induction variable of this loop,
  // clean up the parameter set first.
  ParamAtLoop.erase(SE->getSCEV(L->getCanonicalInductionVariable()));
  return true;
}

bool SCoPInfo::runOnFunction(llvm::Function &F) {
  SE = &getAnalysis<ScalarEvolution>();
  LoopInfo &LI = getAnalysis<LoopInfo>();

  visitLoopsInFunction(F, LI);
  return false;
}

void SCoPInfo::printParams(raw_ostream &OS) const {
  if (ParamsAtScope.empty()) {
    OS << "No good loops found!\n";
    return;
  }
  for (ParamMap::const_iterator I = ParamsAtScope.begin(),
      E = ParamsAtScope.end(); I != E; ++I){
    OS << "Parameters used in Loop: " << I->first->getHeader()->getName()
       << ":\t";
    if (I->second.empty())
      OS << "< none >";
    else {
      for (ParamSet::const_iterator PI = I->second.begin(), PE = I->second.end();
          PI != PE; ++PI)
        OS << **PI << ", ";
    }
    OS << "\n";
  }

}

void SCoPInfo::print(raw_ostream &OS, const Module *) const {
  if (PrintLoopParam)
    printParams(OS);
}

SCoPInfo::~SCoPInfo() {
  clear();

  // Do others thing.
}

char SCoPInfo::ID = 0;

static RegisterPass<SCoPInfo>
X("polly-scop-info", "Polly - Create polyhedral SCoPs Info");

FunctionPass *polly::createSCoPInfoPass() {
  return new SCoPInfo();
}