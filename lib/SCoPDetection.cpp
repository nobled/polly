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
// Temporary Hack for extended region tree.

// Cast the region to loop if there is a loop have the same header and exit.
Loop *polly::castToLoop(const Region &R, LoopInfo &LI) {
  BasicBlock *entry = R.getEntry();

  if (!LI.isLoopHeader(entry))
    return 0;

  Loop *L = LI.getLoopFor(entry);

  BasicBlock *exit = L->getExitBlock();

  // Is the loop with multiple exits?
  if (!exit) return 0;

  if (exit != R.getExit()) {
    // SubRegion/ParentRegion with the same entry.
    assert((R.getNode(R.getEntry())->isSubRegion()
            || R.getParent()->getEntry() == entry)
           && "Expect the loop is the smaller or bigger region");
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

static Value *getPointerOperand(Instruction &Inst) {
  assert((isa<LoadInst>(&Inst) || isa<StoreInst>(&Inst))
    && "Only accept Load or Store!");

  if (LoadInst *load = dyn_cast<LoadInst>(&Inst)) {
    return load->getPointerOperand();
  } else if (StoreInst *store = dyn_cast<StoreInst>(&Inst)) {
    return store->getPointerOperand();
  } else
    llvm_unreachable("Already check in the assert");

  return 0;
}

//===----------------------------------------------------------------------===//
// Helper functions

// Helper function to check parameter
bool polly::isParameter(const SCEV *Var, Region &RefRegion, BasicBlock *CurBB,
                        LoopInfo &LI, ScalarEvolution &SE) {
  assert(Var && CurBB && "Var and CurBB can not be null!");
  // Find the biggest loop that is contained by RefR.
  Loop *topL =  RefRegion.outermostLoopInRegion(&LI, CurBB);

  // The parameter is always loop invariant.
  if (!Var->isLoopInvariant(topL))
      return false;

  if (const SCEVAddRecExpr *AddRec = dyn_cast<SCEVAddRecExpr>(Var)) {
    DEBUG(dbgs() << "Find AddRec: " << *AddRec
      << " at region: " << RefRegion.getNameStr() << "\n");
    // The indvar only expect come from outer loop
    // Or from a loop whose backend taken count could not compute.
    assert((AddRec->getLoop()->contains(getScopeLoop(RefRegion, LI))
            || isa<SCEVCouldNotCompute>(
                 SE.getBackedgeTakenCount(AddRec->getLoop())))
           && "Where comes the indvar?");
    return true;
  } else if (const SCEVUnknown *U = dyn_cast<SCEVUnknown>(Var)) {
    // Some SCEVUnknown will depend on loop variant or conditions:
    // 1. Phi node depend on conditions
    if (PHINode *phi = dyn_cast<PHINode>(U->getValue()))
      // If the phinode contained in the non-entry block of current region,
      // it is not invariant but depend on conditions.
      // TODO: maybe we need special analysis for phi node?
      if (RefRegion.contains(phi) && (RefRegion.getEntry() != phi->getParent()))
        return false;
    // TODO: add others conditions.
    return true;
  }
  // FIXME: Should we accept casts?
  else if (const SCEVCastExpr *Cast = dyn_cast<SCEVCastExpr>(Var))
    return isParameter(Cast->getOperand(), RefRegion, CurBB, LI, SE);
  // Not a SCEVUnknown.
  return false;
}

bool polly::isIndVar(const SCEV *Var, Region &RefRegion, BasicBlock *CurBB,
                     LoopInfo &LI, ScalarEvolution &SE) {
  assert(RefRegion.contains(CurBB)
         && "Expect reference region contain current region!");
  const SCEVAddRecExpr *AddRec = dyn_cast<SCEVAddRecExpr>(Var);
  // Not an Induction variable
  if (!AddRec) return false;

  // If the addrec is the indvar of any loop that containing current region
  Loop *curL = LI.getLoopFor(CurBB),
       *topL = RefRegion.outermostLoopInRegion(curL),
       *recL = const_cast<Loop*>(AddRec->getLoop());
  // If recL contains curL, that means curL will not be null, so topL will not
  // be null because topL will at least contains curL.
  if (recL->contains(curL) && topL->contains(recL))
    return true;

  // If the loop of addrec is not containing current region, that maybe:
  // 1. The loop is containing reference region and this expect to
  //    recognize as parameter
  // 2. The loop is containing by reference region, but not containing the
  //    current region, this because the loop backedge taken count is could
  //    not compute because Var is expect to get by "getSCEVAtScope", and
  //    this means reference region is not valid
  assert((AddRec->getLoop()->contains(getScopeLoop(RefRegion, LI))
          || isa<SCEVCouldNotCompute>(
            SE.getBackedgeTakenCount(AddRec->getLoop())))
        && "getAtScope not work?");
  return false;
}

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

  if (isa<LoadInst>(&Inst) || isa<StoreInst>(&Inst))
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
  // We can only handle loops whose induction variables in are in canonical
  // form.
  PHINode *IndVar = L->getCanonicalInductionVariable();
  Instruction *IndVarInc = L->getCanonicalInductionVariableIncrement();

  if (!IndVar || !IndVarInc) {
    DEBUG(dbgs() << "No canonical iv for loop : " << L->getHeader()->getName()
          << "\n");
    STATSCOP(IndVar);
    return false;
  }

  const SCEV *SIV = SE->getSCEV(IndVar);
  const SCEV *LoopCount = SE->getBackedgeTakenCount(L);

  DEBUG(dbgs() << "Backedge taken count: " << *LoopCount << "\n");

  // We can not handle the loop if its loop bounds can not be computed.
  if (isa<SCEVCouldNotCompute>(LoopCount)) {
    STATSCOP(LoopBound);
    return false;
  }

  // The AffineSCEVIterator will always return the induction variable
  // which start from 0, and step by 1.
  const SCEV *LB = SE->getConstant(SIV->getType(), 0),
        *UB = LoopCount;

  // IV >= LB ==> IV - LB >= 0
  LB = SE->getMinusSCEV(SIV, LB);
  // Match the type, the type of BackedgeTakenCount mismatch when we have
  // something like this in loop exit:
  //    br i1 false, label %for.body, label %for.end
  // In fact, we could do some optimization before SCoPDetecion, so we do not
  // need to worry about this.
  // Be careful of the sign of the upper bounds, if we meet iv <= -1
  // this means the iterate domain is empty since iv >= 0
  // but if we do a zero extend, this will make a non-empty domain
  UB = SE->getTruncateOrSignExtend(UB, SIV->getType());
  // IV <= UB ==> UB - IV >= 0
  UB = SE->getMinusSCEV(UB, SIV);
  // Check the lower bound.
  if (!isValidAffineFunction(LB, RefRegion, L->getHeader(), false)
      || !isValidAffineFunction(UB, RefRegion, L->getHeader(), false)){
    STATSCOP(AffFunc);
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
