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
PrintAccessFunctions("print-access-functions", cl::Hidden,
               cl::desc("Print the access functions of BBs."));

static cl::opt<bool>
PrintLoopBound("print-loop-bounds", cl::Hidden,
            cl::desc("Print the bounds of loops."));

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

    // Ignore the pointer.
    if (Var->getType()->isPointerTy())
      continue;

    if (!Var->isLoopInvariant(Scope)) {
      // If var the induction variable of the loop?
      if (const SCEVAddRecExpr *AddRec = dyn_cast<SCEVAddRecExpr>(Var)){
        assert(AddRec->getLoop() == Scope && "getAtScope not work?");
        continue;
      }
      // A bad SCEV found.
      DEBUG(dbgs() << "Bad SCEV: " << *Var << " in " << *S << " at "
                   << (Scope?Scope->getHeader()->getName():"Top Level")
                   << "\n");
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

bool SCoPInfo::checkBasicBlock(BasicBlock *BB, LLVMSCoP *SCoP) {
  // Iterate over the BB to check its instructions, dont visit terminator
  for (BasicBlock::iterator I = BB->begin(), E = --BB->end(); I != E; ++I){
    // Find the parameters used in load/store
    // TODO: Write the code.
    Instruction &Inst = *I;
    DEBUG(dbgs() << Inst <<"\n");
    // Break the execute flow is not allow.
    if (Inst.mayThrow())
      return false;

    if (!Inst.mayWriteToMemory() && !Inst.mayReadFromMemory()) {
      // Unkonwn side dffects is not allow.
      if (Inst.mayHaveSideEffects())
        return false;
      continue;
    }

    // Try to handle the load/store.
    Value *Pointer = 0;
    bool isStore = false;

    if (LoadInst *load = dyn_cast<LoadInst>(&Inst)) {
      Pointer = load->getPointerOperand();
      DEBUG(dbgs() << "Read Addr " << *SE->getSCEV(Pointer) << "\n");
    }
    else if (StoreInst *store = dyn_cast<StoreInst>(&Inst)) {
      Pointer = store->getPointerOperand();
      DEBUG(dbgs() << "Write Addr " << *SE->getSCEV(Pointer) << "\n");
      isStore = true;
    }

    // Can we get the pointer?
    if (!Pointer)
      return false;

    // Get the function set
    AccFuncSetType &AccFuncSet = AccFuncMap[BB];
    // Make the access function.
    AccFuncSet.push_back(std::make_pair(AffFuncType(), isStore));

    // Is the access function affine?
    if (!buildAffineFunc(SE->getSCEV(Pointer), SCoP, AccFuncSet.back().first)){
      AccFuncMap.erase(BB);
      return false;
    }
  }

  // Find the parameters used in conditions
  // TerminatorInst *TI = BB->getTerminator();

  return true;
}

SCoPInfo::LLVMSCoP *SCoPInfo::findSCoPs(Region* R, SCoPSetType &SCoPs) {
  bool isValidRegion = true;
  SCoPSetType SubSCoPs;

  LLVMSCoP *SCoP = new LLVMSCoP(R);

  // Visit all sub region node.
  for (Region::element_iterator I = R->element_begin(), E = R->element_end();
      I != E; ++I){

    if (I->isSubRegion()) {
      Region *SubR = I->getNodeAs<Region>();
      if (LLVMSCoP *SubSCoP = findSCoPs(SubR, SCoPs)) {
        SubSCoPs.push_back(SubSCoP);
        // Update the loop depth.
        if (SubSCoP->MaxLoopDepth > SCoP->MaxLoopDepth)
          SCoP->MaxLoopDepth = SubSCoP->MaxLoopDepth;
      }
      else
        isValidRegion = false;
    }
    else if (isValidRegion) {
      // We check the basic blocks only the region is valid.
      if (!checkBasicBlock(I->getNodeAs<BasicBlock>(), SCoP))
        isValidRegion = false;
    }
  }

  //// Find the parameters used in loop bounds
  Loop *L = castToLoop(R);
  if (L) {
    // Increase the max loop depth
    ++SCoP->MaxLoopDepth;

    const SCEV *LoopCount = SE->getBackedgeTakenCount(L);

    DEBUG(dbgs() << "Backedge taken count: "<< *LoopCount <<"\n");

    if (!isa<SCEVCouldNotCompute>(LoopCount)) {

      const SCEVAddRecExpr *IndVar =
        dyn_cast<SCEVAddRecExpr>(SE->getSCEV(L->getCanonicalInductionVariable()));

      assert(IndVar && "Expect IndVar a SECVAddRecExpr");

      // The loop will aways start with 0?
      const SCEV *LB = IndVar->getStart();

      const SCEV *UB = SE->getAddExpr(LB, LoopCount);

      AffBoundType &affbounds = LoopBounds[L];

      // Build the lower bound.
      if (!buildAffineFunc(LB, SCoP, affbounds.first) ||
          !buildAffineFunc(UB, SCoP, affbounds.second))
          isValidRegion = false;
    }
    else // We can handle the loop if its loop bounds could not compute.
      isValidRegion = false;
  }

  if (!isValidRegion) {
    // If this Region is not a part of SCoP,
    // add the sub scops to the SCoP vector.
    SCoPs.insert(SCoPs.begin(), SubSCoPs.begin(), SubSCoPs.end());
    // Erase the Loop bounds, so it looks better when we are dumping
    // the loop bounds
    if (L)
      LoopBounds.erase(L);
    // discard the SCoP
    delete SCoP;

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

void SCoPInfo::printAccFunc(llvm::raw_ostream &OS) const {
  for (AccFuncMapType::const_iterator I = AccFuncMap.begin(),
      E = AccFuncMap.end(); I != E; ++I) {
    OS << "BB: " << I->first->getName() << "{\n";
    for (AccFuncSetType::const_iterator FI = I->second.begin(),
        FE = I->second.end(); FI != FE; ++FI) {
      OS << (FI->second?"Writes":"Reads") << " addr: ";
      printAffineFunction(FI->first, OS, SE);
      OS << "\n";
    }
    OS << "}\n";
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
  OS << "SCoP: " << R->getNameStr() << "\tParameters: (";
  // Print Parameters.
  for (ParamSetType::const_iterator PI = Params.begin(), PE = Params.end();
      PI != PE; ++PI)
    OS << **PI << ", ";

  OS << "), Max Loop Depth: "<< MaxLoopDepth <<"\n";
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

  if (PrintAccessFunctions)
    printAccFunc(OS);

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