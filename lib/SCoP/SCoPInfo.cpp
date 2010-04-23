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
#include "llvm/Analysis/RegionIterator.h"
#include "llvm/Analysis/ScalarEvolutionExpressions.h"
#include "llvm/Assembly/Writer.h"
#include "llvm/Support/CommandLine.h"

#define DEBUG_TYPE "polly-scop-info"
#include "llvm/Support/Debug.h"

using namespace llvm;
using namespace polly;

static cl::opt<bool>
PrintLoopBound("print-loop-bounds", cl::Hidden,
            cl::desc("Print the bounds of loops."));

// The pair of (lower_bound, upper_bound),
// where lower_bound <= indVar <= upper_bound
typedef std::pair<const SCEV*, const SCEV*> SCEVBoundsType;

static inline SCEVBoundsType getLoopBounds(const Loop* L, ScalarEvolution* SE) {

  DEBUG(errs() << "Loop: " << L->getHeader()->getName() << "\n");
  const SCEVAddRecExpr *IndVar =
    dyn_cast<SCEVAddRecExpr>(SE->getSCEV(L->getCanonicalInductionVariable()));

  assert(IndVar && "Expect IndVar a SECVAddRecExpr");
  DEBUG(errs() << *IndVar << "\n");

  // The loop will aways start with 0?
  const SCEV *LB = IndVar->getStart();

  DEBUG(errs() << "Lower bound:" << *LB << "\n");

  const SCEV *UB = SE->getBackedgeTakenCount(L);

  DEBUG(errs() << "Backedge taken count: "<< *UB <<"\n");

  if (!isa<SCEVCouldNotCompute>(UB))
    UB = SE->getAddExpr(LB, UB);

  DEBUG(errs() << "Upper bound:" << *UB << "\n");

  return std::make_pair(LB, UB);
}

bool SCoPInfo::buildAffineFunc(const SCEV *S, LLVMSCoP *SCoP,
                            AffFuncType &FuncToBuild) {
  assert(SCoP && "Scope can not be null!");

  Region *R = SCoP->R;

  if (isa<SCEVCouldNotCompute>(S))
    return false;

  Loop *Scope = getScopeLoop(R);

  // Compute S at scope first.
  //if (ScopeL)
  S = SE->getSCEVAtScope(S, Scope);

  // FIXME: Simplify these code.
  for (AffineSCEVIterator I = affine_begin(S, SE), E = affine_end();
      I != E; ++I){
    // Build the affine function.
    FuncToBuild.insert(*I);

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

    if (const SCEVAddRecExpr *AddRec = dyn_cast<SCEVAddRecExpr>(Var))
      assert(AddRec->getLoop()->contains(Scope) && "Where comes the indvar?");
    else
    // TODO: handle the paramer0 * parameter1 case.
    // A dirty hack
      assert(isa<SCEVUnknown>(Var) && "Can only process unknow right now.");
    // Add the loop invariants to parameter lists.
    SCoP->Params.insert(Var);
  }

  return true;
}

SCoPInfo::LLVMSCoP *SCoPInfo::findSCoPs(Region* R, SCoPSetType &SCoPs) {
  bool isGoodRegion = true;
  SCoPSetType SubSCoPs;

  LLVMSCoP *SCoP = new LLVMSCoP(R);

  // Visit all sub region node.
  for (Region::element_iterator I = R->element_begin(), E = R->element_end();
      I != E; ++I){

    if (I->isSubRegion()) {
      Region *SubR = I->getNodeAs<Region>();
      if (LLVMSCoP *SubSCoP = findSCoPs(SubR, SCoPs))
        SubSCoPs.push_back(SubSCoP);
      else
        isGoodRegion = false;
    }
    else {
      // Find the parameters used in conditions
      // TODO: Write the code.
      // Find the parameters used in load/store
      // TODO: Write the code.
    }

  }

  //// Find the parameters used in loop bounds
  Loop *L = castToLoop(R);
  if (L) {
    SCEVBoundsType bounds = getLoopBounds(L, SE);

    AffBoundType &affbounds = LoopBounds[L];

    // Build the lower bound.
    if (!buildAffineFunc(bounds.first, SCoP, affbounds.first) ||
        !buildAffineFunc(bounds.second, SCoP, affbounds.second))
        isGoodRegion = false;
  }

  if (!isGoodRegion) {
    // If this Region is not a part of SCoP,
    // add the sub scops to the SCoP vector.
    SCoPs.insert(SCoPs.begin(), SubSCoPs.begin(), SubSCoPs.end());
    // TODO: other finalization.
    return 0;
  }

  mergeSubSCoPs(SCoP, SubSCoPs);

  return SCoP;
}

bool SCoPInfo::runOnFunction(llvm::Function &F) {
  SE = &getAnalysis<ScalarEvolution>();
  LI = &getAnalysis<LoopInfo>();

  RI = &getAnalysis<RegionInfo>();

  Region *TopRegion = RI->getTopLevelRegion();

  if(LLVMSCoP *SCoP = findSCoPs(TopRegion, SCoPs))
    SCoPs.push_back(SCoP);


  return false;
}

/// Debug/Testing function
static void printAffineFunction(const std::map<const SCEV*, const SCEV*> &F,
                                raw_ostream &OS, ScalarEvolution *SE) {
  size_t size = F.size();

  for (std::map<const SCEV*, const SCEV*>::const_iterator I = F.begin(),
      E = F.end(); I != E; ++I){
    // Print the coefficient (constant part)
    I->second->print(OS);

    OS << " * ";

    // Print the variable
    const SCEV* S = I->first;

    if (const SCEVUnknown *U = dyn_cast<SCEVUnknown>(S))
      WriteAsOperand(OS, U->getValue(), false);
    else if(const SCEVAddRecExpr *AddRec = dyn_cast<SCEVAddRecExpr>(S)) {
      PHINode *V = AddRec->getLoop()->getCanonicalInductionVariable();
      // Get the induction variable of the loop.
      const SCEV *IndVar = SE->getSCEV(V);

      if (IndVar == AddRec)
        // Print out the llvm value if AddRec is the induction variable.
        WriteAsOperand(OS, V, false);
      else
        S->print(OS);
    }
    else
      S->print(OS);


    if (--size)
      OS << " + ";
  }
}

void SCoPInfo::printBounds(raw_ostream &OS) const {
  if (LoopBounds.empty()) {
    OS << "No good loops found!\n";
    return;
  }
  for (BoundMapType::const_iterator I = LoopBounds.begin(),
      E = LoopBounds.end(); I != E; ++I){
    OS << "Bounds of Loop: " << I->first->getHeader()->getName()
      << ":\t{ ";
    printAffineFunction(I->second.first, OS, SE);
    OS << ", ";
    printAffineFunction(I->second.second, OS, SE);
    OS << "}\n";
  }
}

void SCoPInfo::LLVMSCoP::print(raw_ostream &OS) const {
  OS << "SCoP: " << R->getNameStr() << "\tParameters: {";
  // Print Parameters.
  for (ParamSetType::const_iterator PI = Params.begin(), PE = Params.end();
      PI != PE; ++PI)
    OS << **PI << ", ";

  OS << "}\n";
}

void SCoPInfo::print(raw_ostream &OS, const Module *) const {
  if (SCoPs.empty())
    OS << "No SCoP found!\n";
  else
    // Print all SCoPs.
    for (SCoPSetType::const_iterator I = SCoPs.begin(), E = SCoPs.end();
        I != E; ++I){
      (*I)->print(OS);
    }

  if (PrintLoopBound)
    printBounds(OS);

  OS << "\n";
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