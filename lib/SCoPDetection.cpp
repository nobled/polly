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
BADSCOP_STAT(Scalar,      "Found scalar dependency");
BADSCOP_STAT(Other,       "Others");

// Checks if a SCEV is independent.
// This means it only references Instructions that are either
//   * defined in the same BasicBlock
//   * defined outside the Region
//   * canonical induction variables.
class IndependentInstructionChecker:
                            SCEVVisitor<IndependentInstructionChecker, bool> {
  Region &R;
  LoopInfo *LI;
  BasicBlock *BB;

public:
  bool visitConstant(const SCEVConstant *S) {
    return true;
  }

  bool visitUnknown(const SCEVUnknown* S) {
    Value *V = S->getValue();

    if (Instruction *I = dyn_cast<Instruction>(V)) {
      Loop *L = LI->getLoopFor(I->getParent());

      if (!R.contains(I))
        return true;

      if (L && L->getCanonicalInductionVariable() == I)
        return true;

      if (I->getParent() == BB)
        return true;

      return false;
    }

    return true;
  }

  bool visitNAryExpr(const SCEVNAryExpr *S) {
    for (SCEVNAryExpr::op_iterator OI = S->op_begin(), OE = S->op_end();
         OI != OE; ++OI)
      if(!visit(*OI))
        return false;

    return true;
  }

  bool visitMulExpr(const SCEVMulExpr* S) {
    return visitNAryExpr(S);
  }

  bool visitCastExpr(const SCEVCastExpr *S) {
    return visit(S->getOperand());
  }

  bool visitTruncateExpr(const SCEVTruncateExpr *S) {
    return visit(S->getOperand());
  }

  bool visitZeroExtendExpr(const SCEVZeroExtendExpr *S) {
    return visit(S->getOperand());
  }

  bool visitSignExtendExpr(const SCEVSignExtendExpr *S) {
    return visit(S->getOperand());
  }

  bool visitAddExpr(const SCEVAddExpr *S) {
    return visitNAryExpr(S);
  }

  bool visitAddRecExpr(const SCEVAddRecExpr *S) {
    return visitNAryExpr(S);
  }

  bool visitUDivExpr(const SCEVUDivExpr *S) {
    return visit(S->getLHS()) && visit(S->getRHS());
  }

  bool visitSMaxExpr(const SCEVSMaxExpr *S) {
    return visitNAryExpr(S);
  }

  bool visitUMaxExpr(const SCEVUMaxExpr *S) {
    return visitNAryExpr(S);
  }

  bool visitCouldNotCompute(const SCEVCouldNotCompute *S) {
    llvm_unreachable("SCEV cannot be checked");
  }

public:
  IndependentInstructionChecker(Region &RefRegion, LoopInfo *LInfo)
    : R(RefRegion), LI(LInfo) {}

  bool isIndependent(const SCEV *S, BasicBlock *Block) {
    BB = Block;
    return visit(S);
  }
};

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

    if (isParameter(Var, RefRegion, *LI, *SE)
        || isIndVar(Var, RefRegion, *LI, *SE))
      continue;

    // A bad SCEV found.
    DEBUG(dbgs() << "Bad SCEV: " << *Var << " at loop";
          if (Scope)
            WriteAsOperand(dbgs(), Scope->getHeader(), false);
          else
            dbgs() << "Top Level";
          dbgs()  << "Cur BB: " << CurBB->getName()
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
    else // No single exit exists.
      return Exit;

    // Get largest region that starts at Exit.
    Region *ExitR = RI->getRegionFor(Exit);
    while (ExitR && ExitR->getParent()
           && ExitR->getParent()->getEntry() == Exit)
      ExitR = ExitR->getParent();

    for (pred_iterator PI = pred_begin(Exit), PE = pred_end(Exit); PI != PE;
         ++PI)
      if (!R->contains(*PI) && !ExitR->contains(*PI))
        break;

    // This stops infinite cycles.
    if (DT->dominates(Exit, BB))
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

  if (!Br) {
    STATSCOP(CFG);
    return false;
  }
  if (Br->isUnconditional()) return true;

  Value *Condition = Br->getCondition();

  // UndefValue is not allowed as condition.
  if (isa<UndefValue>(Condition)) {
    STATSCOP(AffFunc);
    return false;
  }

  // Only Constant and ICmpInst are allowed as condition.
  if (!(isa<Constant>(Condition) || isa<ICmpInst>(Condition))) {
    STATSCOP(AffFunc);
    return false;
  }

  // Allow perfectly nested conditions.
  assert(Br->getNumSuccessors() == 2 && "Unexpected number of successors");

  // We need to check if both operands of the ICmp are affine.
  if (ICmpInst *ICmp = dyn_cast<ICmpInst>(Condition)) {
    if (isa<UndefValue>(ICmp->getOperand(0))
        || isa<UndefValue>(ICmp->getOperand(1))) {
      STATSCOP(AffFunc);
      return false;
    }

    bool affineLHS = isValidAffineFunction(SE->getSCEV(ICmp->getOperand(0)),
                                            RefRegion, &BB, false);
    bool affineRHS = isValidAffineFunction(SE->getSCEV(ICmp->getOperand(1)),
                                            RefRegion, &BB, false);

    if (!(affineLHS && affineRHS)) {
      STATSCOP(AffFunc);
      return false;
    }
  }

  // Allow loop exit conditions.
  Loop *L = LI->getLoopFor(&BB);
  if (L && L->getExitingBlock() == &BB)
    return true;

  // Only well structured conditions.
  if (!(maxRegionExit(Br->getSuccessor(0)) == maxRegionExit(Br->getSuccessor(1))
        && maxRegionExit(Br->getSuccessor(0)))) {
    STATSCOP(CFG);
    return false;
  }

  // Everything is ok if we reach here.
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


bool SCoPDetection::hasScalarDependency(Instruction &Inst,
                                        Region &RefRegion) const {
  IndependentInstructionChecker Checker(RefRegion, LI);

  for (Instruction::op_iterator UI = Inst.op_begin(), UE = Inst.op_end();
       UI != UE; ++UI)
    if (Instruction *OpInst = dyn_cast<Instruction>((*UI).get())) {
      if (SE->isSCEVable(OpInst->getType())) {
        const SCEV *scev = SE->getSCEV(OpInst);
        if (!Checker.isIndependent(scev, Inst.getParent()))
          return false;
      } else if (OpInst->getParent() == Inst.getParent()
                 || !RefRegion.contains(OpInst))
          continue;
      else
        return false;
    }

  for (Instruction::use_iterator UI = Inst.use_begin(), UE = Inst.use_end();
       UI != UE; ++UI)
    if (Instruction *Use = dyn_cast<Instruction>(UI))
      if (!RefRegion.contains(Use->getParent()))
        return false;
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

  // Scalar dependencies are not allowed.
  if (!hasScalarDependency(Inst, RefRegion)) {
    DEBUG(dbgs() << "Scalar dependency found: ";
          WriteAsOperand(dbgs(), &Inst, false);
          dbgs() << "\n");
    STATSCOP(Scalar);
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
    DEBUG(dbgs() << "No canonical iv for loop: ";
          WriteAsOperand(dbgs(), L->getHeader(), false);
          dbgs() << "\n");
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

  // PHI nodes are not allowed in the exit basic block.
  if (BasicBlock *Exit = R.getExit()) {
    BasicBlock::iterator I = Exit->begin();
    if (I != Exit->end() && isa<PHINode> (*I))
      return false;
  }

  if (!isValidRegion(R,R))
    return false;

  return true;
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

      DEBUG(dbgs() << "Bad BB found: ";
            WriteAsOperand(dbgs(), &BB, false);
            dbgs() << "\n");
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

