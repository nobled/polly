//===----- SCoPDetection.cpp  - Detect SCoPs --------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Pass to detect the maximal static control parts (SCoPs) of a function.
//
//===----------------------------------------------------------------------===//

#include "polly/SCoPDetection.h"

#include "polly/LinkAllPasses.h"
#include "polly/Support/SCoPHelper.h"
#include "polly/Support/AffineSCEVIterator.h"

#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/RegionIterator.h"

#define DEBUG_TYPE "polly-detect"
#include "llvm/Support/Debug.h"

using namespace llvm;
using namespace polly;

//===----------------------------------------------------------------------===//
// Statistics.

STATISTIC(ValidRegion, "Number of regions that a valid part of SCoP");

#define BADSCOP_STAT(NAME, DESC) STATISTIC(Bad##NAME##ForSCoP, \
                                           "Number of bad regions for SCoP: "\
                                           DESC)

#define STATSCOP(NAME); assert(!verifying && #NAME); ++Bad##NAME##ForSCoP;

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
// SCoPDetection.

bool SCoPDetection::isMaxRegionInSCoP(const Region &R) const {
  // The Region is valid only if it could be found in the set.
  return ValidRegions.count(&R);
}

bool SCoPDetection::isValidAffineFunction(const SCEV *S, Region &RefRegion,
                                          bool isMemoryAccess) const {
  bool PointerExists = false;
  assert(S && "S must not be null!");

  if (isa<SCEVCouldNotCompute>(S))
    return false;

  for (AffineSCEVIterator I = affine_begin(S, SE), E = affine_end(); I != E;
       ++I) {
    // The constant part must be a SCEVConstant.
    // TODO: support sizeof in coefficient.
    if (!isa<SCEVConstant>(I->second))
      return false;

    const SCEV *Var = I->first;

    // A constant offset is affine.
    if(isa<SCEVConstant>(Var))
      continue;

    // Memory accesses are allowed to have a base pointer.
    if (Var->getType()->isPointerTy()) {
      if (!isMemoryAccess) return false;

      assert(I->second->isOne() && "Only one as pointer coefficient allowed.");
      const SCEVUnknown *BaseAddr = dyn_cast<SCEVUnknown>(Var);

      if (!BaseAddr) return false;

      assert(!PointerExists && "Found second base pointer.");
      PointerExists = true;
      continue;
    }

    if (isParameter(Var, RefRegion, *LI, *SE)
        || isIndVar(Var, RefRegion, *LI, *SE))
      continue;

    return false;
  }
  return !isMemoryAccess || PointerExists;
}

bool SCoPDetection::isValidCFG(BasicBlock &BB, Region &RefRegion) const {
  TerminatorInst *TI = BB.getTerminator();

  // Return instructions are only valid if the region is the top level region.
  if (isa<ReturnInst>(TI) && !RefRegion.getExit() && TI->getNumOperands() == 0)
    return true;

  BranchInst *Br = dyn_cast<BranchInst>(TI);

  if (!Br) {
    DEBUG(dbgs() << "Non branch instruction as terminator of BB: ";
          WriteAsOperand(dbgs(), &BB, false);
          dbgs() << "\n");
    STATSCOP(CFG);
    return false;
  }

  if (Br->isUnconditional()) return true;

  Value *Condition = Br->getCondition();

  // UndefValue is not allowed as condition.
  if (isa<UndefValue>(Condition)) {
    DEBUG(dbgs() << "Undefined value in branch instruction of BB: ";
          WriteAsOperand(dbgs(), &BB, false);
          dbgs() << "\n");
    STATSCOP(AffFunc);
    return false;
  }

  // Only Constant and ICmpInst are allowed as condition.
  if (!(isa<Constant>(Condition) || isa<ICmpInst>(Condition))) {
    DEBUG(dbgs() << "Non Constant and non ICmpInst instruction in BB: ";
          WriteAsOperand(dbgs(), &BB, false);
          dbgs() << "\n");
    STATSCOP(AffFunc);
    return false;
  }

  // Allow perfectly nested conditions.
  assert(Br->getNumSuccessors() == 2 && "Unexpected number of successors");

  // We need to check if both operands of the ICmp are affine.
  if (ICmpInst *ICmp = dyn_cast<ICmpInst>(Condition)) {
    if (isa<UndefValue>(ICmp->getOperand(0))
        || isa<UndefValue>(ICmp->getOperand(1))) {
      DEBUG(dbgs() << "Undefined operand in branch instruction of BB: ";
            WriteAsOperand(dbgs(), &BB, false);
            dbgs() << "\n");
      STATSCOP(AffFunc);
      return false;
    }

    const SCEV *ScevLHS = SE->getSCEV(ICmp->getOperand(0));
    const SCEV *ScevRHS = SE->getSCEV(ICmp->getOperand(1));

    bool affineLHS = isValidAffineFunction(ScevLHS, RefRegion);
    bool affineRHS = isValidAffineFunction(ScevRHS, RefRegion);

    if (!affineLHS || !affineRHS) {
      DEBUG(dbgs() << "Non affine branch instruction in BB: ";
            WriteAsOperand(dbgs(), &BB, false);
            dbgs() << "\n");
      STATSCOP(AffFunc);
      return false;
    }
  }

  // Allow loop exit conditions.
  Loop *L = LI->getLoopFor(&BB);
  if (L && L->getExitingBlock() == &BB)
    return true;

  // Allow perfectly nested conditions.
  Region *R = RI->getRegionFor(&BB);
  if (R->getEntry() != &BB) {
    DEBUG(dbgs() << "Non well structured condition starting at BB: ";
          WriteAsOperand(dbgs(), &BB, false);
          dbgs() << "\n");
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

  // Indirect calls are not supported.
  if (CalledFunction == 0)
    return false;

  // TODO: Intrinsics.
  return false;
}

bool SCoPDetection::isValidMemoryAccess(Instruction &Inst,
                                        Region &RefRegion) const {
  Value *Ptr = getPointerOperand(Inst);
  const SCEV *AccessFunction = SE->getSCEV(Ptr);

  if (!isValidAffineFunction(AccessFunction, RefRegion, true)) {
    DEBUG(dbgs() << "Bad memory addr " << *AccessFunction << "\n");
    STATSCOP(AffFunc);
    return false;
  }

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
          return true;
      } else if (OpInst->getParent() == Inst.getParent()
                 || !RefRegion.contains(OpInst))
          continue;
      else
        return true;
    }

  for (Instruction::use_iterator UI = Inst.use_begin(), UE = Inst.use_end();
       UI != UE; ++UI)
    if (Instruction *Use = dyn_cast<Instruction>(*UI))
      if (!RefRegion.contains(Use->getParent()))
        return true;

  return false;
}

bool SCoPDetection::isValidInstruction(Instruction &Inst,
                                       Region &RefRegion) const {
  // Only canonical IVs are allowed.
  if (isa<PHINode>(Inst)) {
    Loop *L = LI->getLoopFor(Inst.getParent());
    if (!L || L->getCanonicalInductionVariable() != &Inst) {
      DEBUG(dbgs() << "Non canonical PHI node found: ";
            WriteAsOperand(dbgs(), &Inst, false);
            dbgs() << "\n");
      return false;
    }
  }

  // Scalar dependencies are not allowed.
  if (hasScalarDependency(Inst, RefRegion)) {
    DEBUG(dbgs() << "Scalar dependency found: ";
    WriteAsOperand(dbgs(), &Inst, false);
    dbgs() << "\n");
    STATSCOP(Scalar);
    return false;
  }

  // We only check the call instruction but not invoke instruction.
  if (CallInst *CI = dyn_cast<CallInst>(&Inst)) {
    if (isValidCallInst(*CI))
      return true;

    DEBUG(dbgs() << "Bad call Inst: ";
          WriteAsOperand(dbgs(), &Inst, false);
          dbgs() << "\n");
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
  DEBUG(dbgs() << "Bad instruction found: ";
        WriteAsOperand(dbgs(), &Inst, false);
        dbgs() << "\n");
  STATSCOP(Other);
  return false;
}

bool SCoPDetection::isValidBasicBlock(BasicBlock &BB,
                                      Region &RefRegion) const {
  if (!isValidCFG(BB, RefRegion))
    return false;

  // Check all instructions, except the terminator instruction.
  for (BasicBlock::iterator I = BB.begin(), E = --BB.end(); I != E; ++I)
    if (!isValidInstruction(*I, RefRegion))
      return false;

  Loop *L = LI->getLoopFor(&BB);
  if (L && L->getHeader() == &BB && !isValidLoop(L, RefRegion))
    return false;

  return true;
}

bool SCoPDetection::isValidLoop(Loop *L, Region &RefRegion) const {
  PHINode *IndVar = L->getCanonicalInductionVariable();
  // No canonical induction variable.
  if (!IndVar) {
    DEBUG(dbgs() << "No canonical iv for loop: ";
          WriteAsOperand(dbgs(), L->getHeader(), false);
          dbgs() << "\n");
    STATSCOP(IndVar);
    return false;
  }

  // Is the loop count affine?
  const SCEV *LoopCount = SE->getBackedgeTakenCount(L);
  if (!isValidAffineFunction(LoopCount, RefRegion, false)) {
    STATSCOP(LoopBound);
    return false;
  }

  return true;
}

Region *SCoPDetection::expandRegion(Region &R) {
  Region *CurrentRegion = &R;
  Region *TmpRegion = R.getExpandedRegion();

  DEBUG(dbgs() << "\tExpanding " << R.getNameStr() << "\n");

  while (TmpRegion) {
    DEBUG(dbgs() << "\t\tTrying " << TmpRegion->getNameStr() << "\n");

    if (!allBlocksValid(*TmpRegion))
      break;

    if (isValidExit(*TmpRegion)) {
      if (CurrentRegion != &R)
        delete CurrentRegion;

      CurrentRegion = TmpRegion;
    }

    Region *TmpRegion2 = TmpRegion->getExpandedRegion();

    if (TmpRegion != &R && TmpRegion != CurrentRegion)
      delete TmpRegion;

    TmpRegion = TmpRegion2;
  }

  if (&R == CurrentRegion)
    return NULL;

  DEBUG(dbgs() << "\tto " << CurrentRegion->getNameStr() << "\n");

  return CurrentRegion;
}


void SCoPDetection::findSCoPs(Region &R) {

  if (isValidRegion(R)) {
    ++ValidRegion;
    ValidRegions.insert(&R);
    return;
  }

  for (Region::iterator I = R.begin(), E = R.end(); I != E; ++I)
    findSCoPs(**I);

  // Try to expand regions.
  //
  // As the region tree normally only contains canonical regions, non canonical
  // regions that form a SCoP are not found. Therefore, those non canonical
  // regions are checked by expanding the canonical ones.

  std::vector<Region*> ToExpand;

  for (Region::iterator I = R.begin(), E = R.end(); I != E; ++I)
    ToExpand.push_back(*I);

  for (std::vector<Region*>::iterator RI = ToExpand.begin(),
       RE = ToExpand.end(); RI != RE; ++RI) {
    Region *CurrentRegion = *RI;

    // Skip invalid regions. Regions may become invalid, if they are element of
    // an already expanded region.
    if (ValidRegions.find(CurrentRegion) == ValidRegions.end())
      continue;

    Region *ExpandedR = expandRegion(*CurrentRegion);

    if (!ExpandedR)
      continue;

    R.addSubRegion(ExpandedR, true);
    ValidRegions.insert(ExpandedR);
    ValidRegions.erase(CurrentRegion);

    for (Region::iterator I = ExpandedR->begin(), E = ExpandedR->end(); I != E;
         ++I)
      ValidRegions.erase(*I);
  }
}

bool SCoPDetection::allBlocksValid(Region &R) const {
  for (Region::block_iterator I = R.block_begin(), E = R.block_end(); I != E;
       ++I)
    if (!isValidBasicBlock(*(I->getNodeAs<BasicBlock>()), R))
      return false;

  return true;
}

bool SCoPDetection::isValidExit(Region &R) const {
  // PHI nodes are not allowed in the exit basic block.
  if (BasicBlock *Exit = R.getExit()) {
    BasicBlock::iterator I = Exit->begin();
    if (I != Exit->end() && isa<PHINode> (*I)) {
      DEBUG(dbgs() << "PHI node in exit";
            dbgs() << "\n");
      return false;
    }
  }

  return true;
}

bool SCoPDetection::isValidRegion(Region &R) const {
  DEBUG(dbgs() << "Checking region: " << R.getNameStr() << "\n\t");

  // The toplevel region is no valid region.
  if (!R.getParent()) {
    DEBUG(dbgs() << "Top level region is invalid";
          dbgs() << "\n");
    return false;
  }

  if (!allBlocksValid(R))
    return false;

  if (!isValidExit(R))
    return false;

  DEBUG(dbgs() << "OK\n");
  return true;
}

bool SCoPDetection::runOnFunction(llvm::Function &F) {
  SE = &getAnalysis<ScalarEvolution>();
  LI = &getAnalysis<LoopInfo>();
  RI = &getAnalysis<RegionInfo>();
  Region *TopRegion = RI->getTopLevelRegion();

  findSCoPs(*TopRegion);
  return false;
}


void polly::SCoPDetection::verifyRegion(const Region &R) const {
  assert(isMaxRegionInSCoP(R) && "Expect R is a valid region.");
  verifying = true;
  isValidRegion(const_cast<Region&>(R));
  verifying = false;
}

void polly::SCoPDetection::verifyAnalysis() const {
  for (RegionSet::const_iterator I = ValidRegions.begin(),
      E = ValidRegions.end(); I != E; ++I)
    verifyRegion(**I);
}

void SCoPDetection::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<DominatorTree>();
  AU.addRequired<PostDominatorTree>();
  AU.addRequired<LoopInfo>();
  AU.addRequired<ScalarEvolution>();
  AU.addRequiredTransitive<RegionInfo>();
  AU.setPreservesAll();
}

void SCoPDetection::print(raw_ostream &OS, const Module *) const {
  for (RegionSet::const_iterator I = ValidRegions.begin(),
      E = ValidRegions.end(); I != E; ++I)
    OS << "Valid Region for SCoP: " << (*I)->getNameStr() << '\n';

  OS << "\n";
}

void SCoPDetection::releaseMemory() {
  ValidRegions.clear();
}

char SCoPDetection::ID = 0;

INITIALIZE_PASS(SCoPDetection, "polly-detect",
                "Polly - Detect SCoPs in functions", true, true);

