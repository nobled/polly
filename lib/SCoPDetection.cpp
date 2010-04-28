//===------ SCoPDetection.cpp  - Detect SCoPs in LLVM Function ---*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Detect SCoPs in LLVM Function and extract loop bounds, access functions and
// conditions while checking.
//
//===----------------------------------------------------------------------===//

#include "SCoPDetection.h"

#include "llvm/Analysis/AffineSCEVIterator.h"
#include "llvm/Analysis/RegionIterator.h"
#include "llvm/Analysis/ScalarEvolutionExpressions.h"
#include "llvm/Assembly/Writer.h"
#include "llvm/Support/CFG.h"
#include "llvm/Support/CommandLine.h"

#define DEBUG_TYPE "polly-scop-detect"
#include "llvm/Support/Debug.h"

using namespace llvm;
using namespace polly;

//===----------------------------------------------------------------------===//
// Temporary Hack for extended regiontree.
// Cast the region to loop if there is a loop have the same header and exit.
Loop *polly::castToLoop(const Region &R, LoopInfo &LI) {
  BasicBlock *entry = R.getEntry();

  if (!LI.isLoopHeader(entry))
    return 0;

  Loop *L = LI.getLoopFor(entry);

  BasicBlock *exit = L->getExitBlock();

  // Is the loop with multiple exits?
  // assert(exit && "Oop! we need the extended region tree :P");
  if (!exit) return 0;

  if (exit != R.getExit()) {
    // SubRegion/ParentRegion with the same entry.
    assert((R.getNode(R.getEntry())->isSubRegion() ||
      R.getParent()->getEntry() == entry) &&
      "Expect the loop is the smaller or bigger region");
    return 0;
  }

  return L;
}

// Get the Loop containing all bbs of this region, for ScalarEvolution
// "getSCEVAtScope"
Loop *polly::getScopeLoop(const Region &R, LoopInfo &LI) {
  const Region *tempR = &R;
  while (tempR) {
    if (Loop *L = castToLoop(*tempR, LI))
      return L;

    tempR = tempR->getParent();
  }
  return 0;
}

//===----------------------------------------------------------------------===//
// SCEVAffFunc Implement
void SCEVAffFunc::print(raw_ostream &OS, ScalarEvolution *SE) const {
  // Print BaseAddr
  if (BaseAddr) {
    WriteAsOperand(OS, BaseAddr, false);
    OS << "[";
  }

  for (LnrTransSet::const_iterator I = LnrTrans.begin(), E = LnrTrans.end();
    I != E; ++I) {
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

      OS << " + ";
  }

  if (TransComp)
    OS << *TransComp;

  if (BaseAddr)
    OS << "]";

}

bool SCEVAffFunc::buildAffineFunc(const SCEV *S, LLVMSCoP &SCoP,
                                  SCEVAffFunc &FuncToBuild,
                                  LoopInfo &LI, ScalarEvolution &SE) {
  Region &R = SCoP.R;

  if (isa<SCEVCouldNotCompute>(S))
    return false;

  Loop *Scope = getScopeLoop(R, LI);

  // Compute S at scope first.
  //if (ScopeL)
  S = SE.getSCEVAtScope(S, Scope);

  // FIXME: Simplify these code.
  for (AffineSCEVIterator I = affine_begin(S, &SE), E = affine_end();
    I != E; ++I){

      const SCEV *Var = I->first;

      // Ignore the constant offset.
      if(isa<SCEVConstant>(Var)) {
        // Add the translation component
        FuncToBuild.TransComp = I->second;
        continue;
      }

      // Ignore the pointer.
      if (Var->getType()->isPointerTy()) {
        DEBUG(dbgs() << "Find pointer: " << *Var <<"\n");
        assert(I->second->isOne() && "The coefficient of pointer expect is one!");
        const SCEVUnknown *BaseAddr = dyn_cast<SCEVUnknown>(Var);
        //assert(BaseAddr && "Can only handle Unknown Addr!");
        if (!BaseAddr) return false;
        // Setup the base address
        FuncToBuild.BaseAddr = BaseAddr->getValue();
        continue;
      }


      // Build the affine function.
      FuncToBuild.LnrTrans.insert(*I);

      if (!Var->isLoopInvariant(Scope)) {
        // If var the induction variable of the loop?
        if (const SCEVAddRecExpr *AddRec = dyn_cast<SCEVAddRecExpr>(Var)){
          // Is the loop with multiple exit?
          //assert(AddRec->getLoop()->getExitBlock() &&
          //  "Oop! we need the extended region tree :P");
          if (!AddRec->getLoop()->getExitBlock()) return false;

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
      else if (const SCEVCastExpr *Cast = dyn_cast<SCEVCastExpr>(Var)) {
        DEBUG(dbgs() << "Warning: Ignore cast expression: " << *Cast << "\n");
        // Dirty hack. replace the cast expression by its operand.
        FuncToBuild.LnrTrans.erase(FuncToBuild.LnrTrans.find(Var));
        Var = Cast->getOperand();
        FuncToBuild.LnrTrans.insert(std::make_pair(Var, I->first));
      }
      else {
        // TODO: handle the paramer0 * parameter1 case.
        // A dirty hack
        DEBUG(dbgs() << *Var << "\n");
        assert(isa<SCEVUnknown>(Var) && "Can only process unknown right now.");
      }
      // Add the loop invariants to parameter lists.
      SCoP.Params.insert(Var);
  }

  return true;
}

//===----------------------------------------------------------------------===//
// LLVMSCoP Implement

void LLVMSCoP::mergeSubSCoPs(TempSCoPSetType &SubSCoPs,
                             LoopInfo &LI, ScalarEvolution &SE) {
  while (!SubSCoPs.empty()) {
    LLVMSCoP *SubSCoP = SubSCoPs.back();
    // Merge the parameters.
    Params.insert(SubSCoP->Params.begin(),
                  SubSCoP->Params.end());
    // Merge loop bounds
    LoopBounds.insert(SubSCoP->LoopBounds.begin(),
                      SubSCoP->LoopBounds.end());
    // Merge Access function
    AccFuncMap.insert(SubSCoP->AccFuncMap.begin(),
                      SubSCoP->AccFuncMap.end());
    // Discard the merged scop.
    SubSCoPs.pop_back();
    delete SubSCoP;
  }
  // The induction variable is not parameter at this scope.
  if (Loop *L = castToLoop(R, LI))
    Params.erase(SE.getSCEV(L->getCanonicalInductionVariable()));
}


void LLVMSCoP::print(raw_ostream &OS, ScalarEvolution *SE) const {
  OS << "SCoP: " << R.getNameStr() << "\tParameters: (";
  // Print Parameters.
  for (ParamSetType::const_iterator PI = Params.begin(), PE = Params.end();
    PI != PE; ++PI)
    OS << **PI << ", ";

  OS << "), Max Loop Depth: "<< MaxLoopDepth <<"\n";

  printBounds(OS, SE);

  printAccFunc(OS, SE);
}

void LLVMSCoP::printAccFunc(llvm::raw_ostream &OS, ScalarEvolution *SE) const {
  for (AccFuncMapType::const_iterator I = AccFuncMap.begin(),
    E = AccFuncMap.end(); I != E; ++I) {
      OS << "BB: " << I->first->getName() << "{\n";
      for (AccFuncSetType::const_iterator FI = I->second.begin(),
        FE = I->second.end(); FI != FE; ++FI) {
          OS << (FI->second?"Writes ":"Reads ");
          FI->first.print(OS,SE);
          OS << "\n";
      }
      OS << "}\n";
  }
}

void LLVMSCoP::printBounds(raw_ostream &OS, ScalarEvolution *SE) const {
  if (LoopBounds.empty()) {
    OS << "No good loops found!\n";
    return;
  }
  for (BoundMapType::const_iterator I = LoopBounds.begin(),
    E = LoopBounds.end(); I != E; ++I){
      OS << "Bounds of Loop: " << I->first->getHeader()->getName()
        << ":\t{ ";
      I->second.first.print(OS,SE);
      OS << ", ";
      I->second.second.print(OS,SE);
      OS << "}\n";
  }
}

//===----------------------------------------------------------------------===//
// SCoPDetection Implement

// TODO: Move to LoopInfo?
static bool isPreHeader(BasicBlock *BB, LoopInfo *LI) {
  TerminatorInst *TI = BB->getTerminator();
  return (TI->getNumSuccessors() == 1) &&
    LI->isLoopHeader(TI->getSuccessor(0));
}

bool SCoPDetection::checkCFG(BasicBlock &BB, Region &R) {
  TerminatorInst *TI = BB.getTerminator();

  unsigned int numSucc = TI->getNumSuccessors();

  // Ret and unconditional branch is ok.
  if (numSucc < 2) return true;

  if (Loop *L = getScopeLoop(R, *LI)) {
    // Only allow branches that are loop exits. This stops anything
    // except loops that have just one exit and are detected by our LoopInfo
    // analysis

    // It is ok if BB is branching out of the loop
    if (L->isLoopExiting(&BB))
      return true;

    assert(L->getExitingBlock() &&
      "Only can handle Loop with 1 exiting block now!");

    // Now bb is not the exit block of the loop.
    // Is BB branching to inner loop?
    for (unsigned int i = 0; i < numSucc; ++i) {
      BasicBlock *SuccBB = TI->getSuccessor(i);
      if (isPreHeader(SuccBB, LI)) {
        // If branching to inner loop
        assert(numSucc == 2 && "Not a natural loop?");
        return true;
      }
    }

    // Now bb is not branching to inner loop.
    // FIXME: Handle the branch condition
    DEBUG(dbgs() << "Bad BB in cfg: " << BB.getName() << "\n");
    return false;
  }

  // BB is not in any loop.
  return true;
  // TODO: handle the branch condition
}

// FIXME: cause regression
//static bool allUsedInRegion(const Region &R, const Value &V) {
//  for (Value::const_use_iterator I = V.use_begin(),
//      E = V.use_end(); I != E; ++I) {
//    Instruction *Inst = (Instruction*)*I;
//    if (!R.contains(Inst)) {
//      DEBUG(dbgs() << "Find bad Instruction " << V << " used by"
//        << *Inst << " in BB: "
//        << Inst->getParent()->getName() << "\n");
//      return false;
//    }
//  }
//
//  return true;
//}

bool SCoPDetection::checkBasicBlock(BasicBlock &BB, LLVMSCoP &SCoP) {
  // Iterate over the BB to check its instructions, dont visit terminator
  for (BasicBlock::iterator I = BB.begin(), E = --BB.end(); I != E; ++I){
    Instruction &Inst = *I;

    //
    //if (!allUsedInRegion(SCoP->R, Inst)) return false;

    DEBUG(dbgs() << Inst <<"\n");
    // Function call is not allowed in SCoP at the moment.
    if (/*CallInst *CI = */dyn_cast<CallInst>(&Inst)) {
      // TODO: Handle CI
      DEBUG(dbgs() << "Bad call Inst!\n");
      return false;
    }

    if (!Inst.mayWriteToMemory() && !Inst.mayReadFromMemory()) {
      // Handle cast instruction
      if (isa<IntToPtrInst>(I) || isa<BitCastInst>(I)) {
        DEBUG(dbgs() << "Bad cast Inst!\n");
        return false;
      }

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
    if (!Pointer) {
      DEBUG(dbgs() << "Bad Inst accessing memory!\n");
      return false;
    }

    // Get the function set
    AccFuncSetType &AccFuncSet = SCoP.AccFuncMap[&BB];
    // Make the access function.
    AccFuncSet.push_back(std::make_pair(SCEVAffFunc(), isStore));

    // Is the access function affine?
    const SCEV *Addr = SE->getSCEV(Pointer);
    if (!SCEVAffFunc::buildAffineFunc(Addr, SCoP, AccFuncSet.back().first, *LI, *SE)) {
      DEBUG(dbgs() << "Bad memory addr " << *Addr << "\n");
      return false;
    }
  }
  // All instruction is ok.
  return true;
}

LLVMSCoP *SCoPDetection::findSCoPs(Region& R, TempSCoPSetType &SCoPs) {
  bool isValidRegion = true;
  TempSCoPSetType SubSCoPs;

  LLVMSCoP *SCoP = new LLVMSCoP(R);

  // Visit all sub region node.
  for (Region::element_iterator I = R.element_begin(), E = R.element_end();
    I != E; ++I){

      if (I->isSubRegion()) {
        Region *SubR = I->getNodeAs<Region>();
        if (LLVMSCoP *SubSCoP = findSCoPs(*SubR, SCoPs)) {
          SubSCoPs.push_back(SubSCoP);
          // Update the loop depth.
          if (SubSCoP->MaxLoopDepth > SCoP->MaxLoopDepth)
            SCoP->MaxLoopDepth = SubSCoP->MaxLoopDepth;
        }
        else
          isValidRegion = false;
      }
      else if (isValidRegion) {
        BasicBlock &BB = *(I->getNodeAs<BasicBlock>());
        // We check the basic blocks only the region is valid.
        if (!checkCFG(BB, R) || // Check cfg
          !checkBasicBlock(BB, *SCoP)) { // Check all non terminator instruction
            DEBUG(dbgs() << "Bad BB found:" << BB.getName() << "\n");
            // Clean up the access function map, so we get a clear dump.
            SCoP->AccFuncMap.erase(&BB);
            isValidRegion = false;
        }
      }
  }

  // Find the parameters used in loop bounds
  Loop *L = castToLoop(R, *LI);
  if (L) {
    // Increase the max loop depth
    ++SCoP->MaxLoopDepth;

    const SCEV *LoopCount = SE->getBackedgeTakenCount(L);

    DEBUG(dbgs() << "Backedge taken count: "<< *LoopCount <<"\n");

    if (!isa<SCEVCouldNotCompute>(LoopCount)) {

      Value* V = L->getCanonicalInductionVariable();

      assert(V && "IndVarSimplify pass not work!");

      const SCEVAddRecExpr *IndVar =
        dyn_cast<SCEVAddRecExpr>(SE->getSCEV(V));

      assert(IndVar && "Expect IndVar a SECVAddRecExpr");

      // The loop will aways start with 0?
      const SCEV *LB = IndVar->getStart();

      const SCEV *UB = SE->getAddExpr(LB, LoopCount);

      AffBoundType &affbounds = SCoP->LoopBounds[L];

      // Build the lower bound.
      if (!SCEVAffFunc::buildAffineFunc(LB, *SCoP, affbounds.first, *LI, *SE)||
        !SCEVAffFunc::buildAffineFunc(UB, *SCoP, affbounds.second, *LI, *SE))
        isValidRegion = false;
    }
    else // We can handle the loop if its loop bounds could not compute.
      isValidRegion = false;
  }

  if (!isValidRegion) {
    DEBUG(dbgs() << "Bad region found: " << R.getNameStr() << "!\n");
    // If this Region is not a part of SCoP,
    // add the sub scops to the SCoP vector.
    SCoPs.insert(SCoPs.end(), SubSCoPs.begin(), SubSCoPs.end());
    // Erase the Loop bounds, so it looks better when we are dumping
    // the loop bounds
    //if (L)
    //  SCoP->LoopBounds.erase(L);
    // discard the SCoP
    delete SCoP;

    return 0;
  }

  SCoP->mergeSubSCoPs(SubSCoPs, *LI, *SE);

  return SCoP;
}


bool SCoPDetection::runOnFunction(llvm::Function &F) {
  SE = &getAnalysis<ScalarEvolution>();
  LI = &getAnalysis<LoopInfo>();

  RI = &getAnalysis<RegionInfo>();

  Region *TopRegion = RI->getTopLevelRegion();

  // found SCoPs.
  TempSCoPSetType TempSCoPs;

  if(LLVMSCoP *SCoP = findSCoPs(*TopRegion, TempSCoPs))
    TempSCoPs.push_back(SCoP);

  while (!TempSCoPs.empty()) {
    LLVMSCoP *TempSCoP = TempSCoPs.back();
    RegionToSCoPs.insert(std::make_pair(&(TempSCoP->R), TempSCoP));
    DEBUG(TempSCoP->print(dbgs(), SE));
    DEBUG(dbgs() << "\n");
    TempSCoPs.pop_back();
  }

  return false;
}

void SCoPDetection::clear() {
  while (!RegionToSCoPs.empty()) {
    TempSCoPMapType::iterator I = RegionToSCoPs.begin();
    delete I->second;
    RegionToSCoPs.erase(I);
  }
}

/// Debug/Testing function

void SCoPDetection::print(raw_ostream &OS, const Module *) const {
  if (RegionToSCoPs.empty())
    OS << "No SCoP found!\n";
  else
    // Print all SCoPs.
    for (TempSCoPMapType::const_iterator I = RegionToSCoPs.begin(),
      E = RegionToSCoPs.end(); I != E; ++I){
        I->second->print(OS, SE);
    }

  OS << "\n";
}

SCoPDetection::~SCoPDetection() {
  clear();
}

char SCoPDetection::ID = 0;

static RegisterPass<SCoPDetection>
X("polly-scop-detect", "Polly - Detect SCoPs");
