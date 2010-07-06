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

STATISTIC(ValidRegion, "Number of regions that a valid part of SCoP");

#define BADSCOP_STAT(NAME, DESC) STATISTIC(Bad##NAME##ForSCoP, \
                                           "Number of bad regions for SCoP: "\
                                           DESC)

#define STATSCOP(NAME); ++Bad##NAME##ForSCoP;

// Note: This also implies loop bounds can not be computed,
// but this one is checked before the loop bounds.
BADSCOP_STAT(LoopNest,    "Some loop has multiple exits");
BADSCOP_STAT(CFG,         "CFG too complex");
BADSCOP_STAT(IndVar,      "Non canonical induction variable in loop");
BADSCOP_STAT(LoopBound,   "Loop bounds can not be computed");
BADSCOP_STAT(FuncCall,    "Function call with side effects appeared");
BADSCOP_STAT(AffFunc,     "Expression not affine");
BADSCOP_STAT(Other,       "Others");

static cl::opt<bool>
AllowConditions("polly-detect-conditions",
                cl::desc("Allow conditions in SCoPs."),
                cl::Hidden,  cl::init(false));

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
    // S is not fully evaluate at current scope, it is not our jobs to fully
    // evaluate it here.
    if (Var != SE->getSCEVAtScope(Var, Scope))
      return false;

    // The constant offset is affine.
    if(isa<SCEVConstant>(Var))
      continue;

    // The pointer is OK.
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
      << (Scope ? Scope->getHeader()->getName() : "Top Level")
      << "Cur BB: " << CurBB->getName()
      << "Ref Region: " << RefRegion.getNameStr()
      << "\n");
    return false;
  }
  return !isMemAcc || PtrExist;
}

// Return the exit of the maximal refined region, that starts at
// BB.
BasicBlock *SCoPDetection::maxRegionExit(BasicBlock *BB) const {
  BasicBlock *Exit = NULL;

  while (true) {
    // Get largest region that starts at BB.
    Region *R = RI->getRegionFor(BB);
    while (R && R->getParent() && R->getParent()->getEntry() == BB)
      R = R->getParent();

    // Get the single exit of BB.
    if (R && R->getEntry() == BB)
      Exit = R->getExit();
    else if (++succ_begin(BB) == succ_end(BB))
      Exit = *succ_begin(BB);
    else
      return Exit;

    // Get largest region that starts at BB.
    Region *ExitR = RI->getRegionFor(Exit);
    while (ExitR && ExitR->getParent()
           && ExitR->getParent()->getEntry() == Exit)
      ExitR = ExitR->getParent();

    for (pred_iterator PI = pred_begin(Exit), PE = pred_end(Exit); PI != PE;
         ++PI)
      if (!R->contains(*PI) && !ExitR->contains(*PI))
        break;

    BB = Exit;
  }

  return Exit;
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

  // Allow loop exit conditions.
  Loop *L = LI->getLoopFor(&BB);
  if (L && L->getExitingBlock() == &BB)
    return true;

  // Allow perfectly nested conditions.
  assert(Br->getNumSuccessors() == 2 && "Unexpected number of successors");

  if (AllowConditions)
    if (maxRegionExit(Br->getSuccessor(0)) == maxRegionExit(Br->getSuccessor(1))
        && maxRegionExit(Br->getSuccessor(0)))
      return true;

  DEBUG(dbgs() << "Bad BB in cfg: " << BB.getName() << "\n");
  STATSCOP(CFG);
  return false;
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

  if (!isValidAffineFunction(SE->getSCEV(Ptr), RefRegion, Inst.getParent(),
                             true)) {
    DEBUG(dbgs() << "Bad memory addr " << *SE->getSCEV(Ptr) << "\n");
    STATSCOP(AffFunc);
    return false;
  }

  // FIXME: Why can expression like int *j = **k; where k has int ** type, pass
  //        the affine function check?
  return true;
}

bool SCoPDetection::isValidInstruction(Instruction &Inst,
                                       Region &RefRegion) const {
  // Only canonical IVs are allowed.
  if (isa<PHINode>(Inst)) {
    Loop *L = LI->getLoopFor(Inst.getParent());
    if (!L || L->getCanonicalInductionVariable() != &Inst)
      return false;
  }

  // We only check the call instruction but not invoke instruction.
  if (CallInst *CI = dyn_cast<CallInst>(&Inst)) {
    if (isValidCallInst(*CI))
      return true;

    DEBUG(dbgs() << "Bad call Inst!\n");
    STATSCOP(FuncCall);
    return false;
  }

  if (!Inst.mayWriteToMemory() && !Inst.mayReadFromMemory()) {
    // Handle cast instruction.
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

  // Is the loop count affine?
  const SCEV *LoopCount = SE->getBackedgeTakenCount(L);
  DEBUG(dbgs() << "Backedge taken count: " << *LoopCount << "\n");
  if (!isValidAffineFunction(LoopCount, RefRegion, L->getHeader(), false)) {
    STATSCOP(LoopBound);
    return false;
  }

  return true;
}

void SCoPDetection::findSCoPs(Region &R) {
  if (isValidRegion(R)) {
    ++ValidRegion;
    ValidRegions.insert(&R);
  }

  // FIXME: We visit the same region multiple times.
  //        All subregions of a valid region are valid. So we can just insert
  //        them in the set.
  for (Region::iterator I = R.begin(), E = R.end(); I != E; ++I)
    findSCoPs(**I);
}

bool SCoPDetection::isValidRegion(Region &R) const {
  // The toplevel region is no valid region.
  if (!R.getParent())
    return false;

  return isValidRegion(R,R);
}

bool SCoPDetection::isValidRegion(Region &RefRegion,
                                  Region &CurRegion) const {
  // Does getScopeLoop work on the current loop nest and region tree?
  if (getScopeLoop(CurRegion, *LI)
      != LI->getLoopFor(CurRegion.getEntry())) {
    STATSCOP(LoopNest);
    return false;
  }

  // Visit all sub region nodes.
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

  findSCoPs(*TopRegion);
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
X("polly-detect", "Polly - Detect SCoPs in functions");

