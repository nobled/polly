//===----- SCoPDetection.cpp  - Detect SCoPs in LLVM Function ---*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Detect SCoPs in LLVM Function.
//
//===----------------------------------------------------------------------===//

#include "polly/SCoPDetection.h"
#include "polly/Support/SCoPHelper.h"
#include "polly/Support/GmpConv.h"
#include "polly/Support/AffineSCEVIterator.h"

#include "llvm/Intrinsics.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/RegionIterator.h"
#include "llvm/Analysis/ScalarEvolutionExpressions.h"
#include "llvm/Assembly/Writer.h"
#include "llvm/Support/CFG.h"
#include "llvm/Support/CallSite.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ErrorHandling.h"

#define DEBUG_TYPE "polly-scop-detect"
#include "llvm/Support/Debug.h"


#include "isl_constraint.h"

using namespace llvm;
using namespace polly;

//===----------------------------------------------------------------------===//
// Some statistic

STATISTIC(ValidRegion,"Number of regions that a valid part of SCoP");

#define BADSCOP_STAT(NAME, DESC) STATISTIC(Bad##NAME##ForSCoP, \
                                           "Number of bad regions for SCoP: "\
                                           DESC)

#define STATSCOP(NAME);              \
  ++Bad##NAME##ForSCoP;

// Note: This will make loop bounds could not compute,
// but this is checked before loop bounds.
BADSCOP_STAT(LoopNest,    "Some loop have multiple exit");

BADSCOP_STAT(CFG,         "CFG too complex");

BADSCOP_STAT(IndVar,      "Cant canonical induction variable in loop");

BADSCOP_STAT(LoopBound,   "Loop bound could not compute");

BADSCOP_STAT(FuncCall,    "Function call occur");

BADSCOP_STAT(AffFunc,     "Expression not affine");

BADSCOP_STAT(Other,       "Others");


//===----------------------------------------------------------------------===//
// SCoPDetection Implementation.

bool SCoPDetection::isValidAffineFunction(const SCEV *S, Region &RefRegion,
                                          BasicBlock *CurBB,
                                          bool isMemAcc) const {
  bool PtrExist = false;
  assert(S && "S can not be null!");

  if (isa<SCEVCouldNotCompute>(S))
    return false;

  Loop *Scope = LI->getLoopFor(CurBB);

  // Compute S at the smallest loop so the addrec from other loops may
  // evaluate to constant.
  S = SE->getSCEVAtScope(S, Scope);

  for (AffineSCEVIterator I = affine_begin(S, SE), E = affine_end();
      I != E; ++I) {
    // The constant part must be a SCEVConstant.
    // TODO: support sizeof in coefficient.
    if (!isa<SCEVConstant>(I->second))
      return false;

    const SCEV *Var = I->first;
    // The constant offset is affine.
    if(isa<SCEVConstant>(Var))
      continue;

    // The pointer is ok.
    if (Var->getType()->isPointerTy()) {
      // If this is not expect a memory access
      if (!isMemAcc) return false;

      DEBUG(dbgs() << "Find pointer: " << *Var <<"\n");
      assert(I->second->isOne()
        && "The coefficient of pointer expect is one!");
      const SCEVUnknown *BaseAddr = dyn_cast<SCEVUnknown>(Var);

      if (!BaseAddr) return false;

      assert(!PtrExist && "We got two pointer?");
      PtrExist = true;
      continue;
    }

    // Check if it is the parameter of reference region.
    // Or it is some induction variable
    if (isParameter(Var, RefRegion, CurBB, *LI, *SE)
      || isIndVar(Var, RefRegion, CurBB, *LI, *SE))
      continue;

    // A bad SCEV found.
    DEBUG(dbgs() << "Bad SCEV: " << *Var << " at loop"
      << (Scope ?
        Scope->getHeader()->getName():"Top Level")
      << "Cur BB: " << CurBB->getName()
      << "Ref Region: " << RefRegion.getNameStr()
      << "\n");
    return false;
  }
  return !isMemAcc || PtrExist;
}

bool SCoPDetection::isValidCFG(BasicBlock &BB, Region &RefRegion) const {
  TerminatorInst *TI = BB.getTerminator();

  // Return instructions are only valid if the region is the top level region.
  if (isa<ReturnInst>(TI) && !RefRegion.getExit() && TI->getNumOperands() == 0)
    return true;

  BranchInst *Br = dyn_cast<BranchInst>(TI);

  if (!Br) return false;
  if (Br->isUnconditional()) return true;

  // Only instructions and constant expressions are valid branch conditions.
  if (!(isa<Constant>(Br->getCondition())
        || isa<Instruction>(Br->getCondition())))
    return false;

  Loop *L = LI->getLoopFor(&BB);

  // Conditions are not yet supported, except at loop exits.
  // TODO: Allow conditions in structured CFGs.
  if (!L || L->getExitingBlock() != &BB) {
    DEBUG(dbgs() << "Bad BB in cfg: " << BB.getName() << "\n");
    STATSCOP(CFG);
    return false;
  }

  return true;
}

bool SCoPDetection::isValidCallInst(CallInst &CI) {
  if (CI.mayHaveSideEffects() || CI.doesNotReturn())
    return false;

  if (CI.doesNotAccessMemory())
    return true;

  Function *CalledFunction = CI.getCalledFunction();

  // Indirect call is not support now.
  if (CalledFunction == 0)
    return false;

  // TODO: Intrinsics.
  return false;
}

bool SCoPDetection::isValidMemoryAccess(Instruction &Inst,
                                        Region &RefRegion) const {
  Value *Ptr = getPointerOperand(Inst);

  if (!isValidAffineFunction(SE->getSCEV(Ptr),
                             RefRegion, Inst.getParent(), true)) {
    DEBUG(dbgs() << "Bad memory addr "
                 << *SE->getSCEV(Ptr) << "\n");
    STATSCOP(AffFunc);
    return false;
  }
  // FIXME: Why expression like int *j = **k; where k has int ** type can pass
  //        affine function check?
  return true;
}

bool SCoPDetection::isValidInstruction(Instruction &Inst,
                                       Region &RefRegion) const {
  // We only check the call instruction but not invoke instruction.
  if (CallInst *CI = dyn_cast<CallInst>(&Inst)) {
    if (isValidCallInst(*CI))
      return true;

    DEBUG(dbgs() << "Bad call Inst!\n");
    STATSCOP(FuncCall);
    return false;
  }

  if (!Inst.mayWriteToMemory() && !Inst.mayReadFromMemory()) {
    // Handle cast instruction
    if (isa<IntToPtrInst>(Inst) || isa<BitCastInst>(Inst)) {
      DEBUG(dbgs() << "Bad cast Inst!\n");
      STATSCOP(Other);
      return false;
    }

    return true;
  }
  // Check the access function.
  if (isa<LoadInst>(Inst) || isa<StoreInst>(Inst))
    return isValidMemoryAccess(Inst, RefRegion);

  // We do not know this instruction, therefore we assume it is invalid.
  STATSCOP(Other);
  return false;
}

bool SCoPDetection::isValidBasicBlock(BasicBlock &BB,
                                      Region &RefRegion) const {

  // Check all instructions, except the terminator instruction.
  for (BasicBlock::iterator I = BB.begin(), E = --BB.end(); I != E; ++I)
    if (!isValidInstruction(*I, RefRegion))
      return false;

  return true;
}

bool SCoPDetection::isValidLoop(Loop *L, Region &RefRegion) const {
  PHINode *IndVar = L->getCanonicalInductionVariable();
  Instruction *IndVarInc = L->getCanonicalInductionVariableIncrement();

  // No canonical induction variable.
  if (!IndVar || !IndVarInc) {
    DEBUG(dbgs() << "No canonical iv for loop : " << L->getHeader()->getName()
          << "\n");
    STATSCOP(IndVar);
    return false;
  }

  // If the loop count affine?
  const SCEV *LoopCount = SE->getBackedgeTakenCount(L);
  DEBUG(dbgs() << "Backedge taken count: " << *LoopCount << "\n");
  if (!isValidAffineFunction(LoopCount, RefRegion, L->getHeader(), false)) {
    STATSCOP(LoopBound);
    return false;
  }

  return true;
}

void SCoPDetection::runOnRegion(Region &R) {
  // FIXME: We visit the same region multiple times.
  for (Region::iterator I = R.begin(), E = R.end(); I != E; ++I)
    // TODO: Analyse the failure and hide the region.
    runOnRegion(**I);

  // Check current region.
  // Do not check toplevel region, it is not support at this moment.
  if (R.getParent() == 0)
    return;

  // Check the Region and remember it if it is valid
  if (isValidRegion(R, R)) {
    ++ValidRegion;
    ValidRegions.insert(&R);
  }
}

bool SCoPDetection::isValidRegion(Region &RefRegion,
                                  Region &CurRegion) const {
  // Check if getScopeLoop work on the current loop nest and region tree,
  // if it not work, we could not handle any further
  if (getScopeLoop(CurRegion, *LI)
      != LI->getLoopFor(CurRegion.getEntry())) {
    STATSCOP(LoopNest);
    return false;
  }

  // Visit all sub region node.
  for (Region::element_iterator I = CurRegion.element_begin(),
       E = CurRegion.element_end(); I != E; ++I) {
    if (I->isSubRegion()) {
      Region &subR = *(I->getNodeAs<Region>());
      if (!isValidRegion(RefRegion, subR))
        return false;
    } else {
      BasicBlock &BB = *(I->getNodeAs<BasicBlock>());

      if (isValidCFG(BB, RefRegion)
          && isValidBasicBlock(BB, RefRegion))
        continue;

      DEBUG(dbgs() << "Bad BB found:" << BB.getName() << "\n");
      return false;
    }
  }

  Loop *L = castToLoop(CurRegion, *LI);

  if (L && !isValidLoop(L, RefRegion))
    return false;

  return true;
}

bool SCoPDetection::runOnFunction(llvm::Function &F) {
  DT = &getAnalysis<DominatorTree>();
  PDT = &getAnalysis<PostDominatorTree>();
  SE = &getAnalysis<ScalarEvolution>();
  LI = &getAnalysis<LoopInfo>();
  RI = &getAnalysis<RegionInfo>();
  Region *TopRegion = RI->getTopLevelRegion();

  ParamSetType Params;
  runOnRegion(*TopRegion);
  return false;
}

void SCoPDetection::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequiredTransitive<DominatorTree>();
  AU.addRequiredTransitive<PostDominatorTree>();
  AU.addRequiredTransitive<LoopInfo>();
  AU.addRequiredTransitive<RegionInfo>();
  AU.addRequiredTransitive<ScalarEvolution>();
  AU.setPreservesAll();
}

void SCoPDetection::clear() {
  ValidRegions.clear();
}

/// Debug/Testing function

void SCoPDetection::print(raw_ostream &OS, const Module *) const {
  for (RegionSet::const_iterator I = ValidRegions.begin(),
      E = ValidRegions.end(); I != E; ++I)
    OS << "Valid Region for SCoP: " << (*I)->getNameStr() << '\n';

  OS << "\n";
}

SCoPDetection::~SCoPDetection() {
  clear();
}

char SCoPDetection::ID = 0;

static RegisterPass<SCoPDetection>
X("polly-scop-detect", "Polly - Detect SCoPs");

// Do not link this pass. This pass suppose to be only used by SCoPInfo.
