//===----- SCoPDetection.cpp  - Detect SCoPs --------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Detect the maximal SCoPs of a function.
//
// A static control part (SCoP) is a subgraph of the control flow graph (CFG)
// that only has statically known control flow and can therefore be described
// within the polyhedral model.
//
// Every SCoP fullfills these restrictions:
//
// * It is a single entry single exit region
//
// * Only affine linear bounds in the loops
//
// Every natural loop in a SCoP must have a number of loop iterations that can
// be described as an affine linear function in surrounding loop iterators or
// parameters. (A parameter is a scalar that does not change its value during
// execution of the SCoP).
//
// * Only comparisons of affine linear expressions in conditions
//
// * All loops and conditions perfectly nested
//
// The control flow needs to be structured such that it could be written using
// just 'for' and 'if' statements, without the need for any 'goto', 'break' or
// 'continue'.
//
// * Side effect free functions call
//
// Only function calls and intrinsics that do not have side effects are allowed
// (readnone).
//
// The SCoP detection finds the largest SCoPs by checking if the largest
// region is a SCoP. If this is not the case, its canonical subregions are
// checked until a region is a SCoP. It is now tried to extend this SCoP by
// creating a larger non canonical region.
//
//===----------------------------------------------------------------------===//

#include "polly/SCoPDetection.h"

#include "polly/LinkAllPasses.h"
#include "polly/Support/SCoPHelper.h"
#include "polly/Support/AffineSCEVIterator.h"

#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/RegionIterator.h"
#include "llvm/Support/CommandLine.h"

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

      if (!BaseAddr || isa<UndefValue>(BaseAddr->getValue())) return false;

      // BaseAddr must be invariant in SCoP.
      if (!isParameter(BaseAddr, RefRegion, *LI, *SE)) return false;

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

bool SCoPDetection::isValidCFG(BasicBlock &BB, Region &RefRegion,
                               bool verifying) const {
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
                                        Region &RefRegion,
                                        bool verifying) const {
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
  for (Instruction::use_iterator UI = Inst.use_begin(), UE = Inst.use_end();
       UI != UE; ++UI)
    if (Instruction *Use = dyn_cast<Instruction>(*UI))
      if (!RefRegion.contains(Use->getParent())) {
        // DirtyHack 1: PHINode user outside the SCoP is not allow, if this
        // PHINode is induction variable, the scalar to array transform may
        // break it and introduce a non-indvar PHINode, which is not allow in
        // SCoP.
        // This can be fix by:
        // Introduce a IndependentBlockPrepare pass, which translate all
        // PHINodes not in SCoP to array.
        // The IndependentBlockPrepare pass can also split the entry block of
        // the function to hold the alloca instruction created by scalar to
        // array.  and split the exit block of the SCoP so the new create load
        // instruction for escape users will not break other SCoPs.
        if (isa<PHINode>(Use))
          return true;
      }

  return false;
}

bool SCoPDetection::isValidInstruction(Instruction &Inst,
                                       Region &RefRegion,
                                       bool verifying) const {
  // Only canonical IVs are allowed.
  if (PHINode *PN = dyn_cast<PHINode>(&Inst))
    if (!isIndVar(PN, LI)) {
      DEBUG(dbgs() << "Non canonical PHI node found: ";
            WriteAsOperand(dbgs(), &Inst, false);
            dbgs() << "\n");
      return false;
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

    if (isa<AllocaInst>(Inst)) {
      DEBUG(dbgs() << "AllocaInst is not allowed!!\n");
      STATSCOP(Other);
      return false;
    }

    return true;
  }

  // Check the access function.
  if (isa<LoadInst>(Inst) || isa<StoreInst>(Inst))
    return isValidMemoryAccess(Inst, RefRegion, verifying);

  // We do not know this instruction, therefore we assume it is invalid.
  DEBUG(dbgs() << "Bad instruction found: ";
        WriteAsOperand(dbgs(), &Inst, false);
        dbgs() << "\n");
  STATSCOP(Other);
  return false;
}

bool SCoPDetection::isValidBasicBlock(BasicBlock &BB,
                                      Region &RefRegion,
                                      bool verifying) const {
  if (!isValidCFG(BB, RefRegion, verifying))
    return false;

  // Check all instructions, except the terminator instruction.
  for (BasicBlock::iterator I = BB.begin(), E = --BB.end(); I != E; ++I)
    if (!isValidInstruction(*I, RefRegion, verifying))
      return false;

  Loop *L = LI->getLoopFor(&BB);
  if (L && L->getHeader() == &BB && !isValidLoop(L, RefRegion, verifying))
    return false;

  return true;
}

bool SCoPDetection::isValidLoop(Loop *L, Region &RefRegion,
                                bool verifying) const {
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
    DEBUG(dbgs() << "Non affine loop bound for loop: ";
          WriteAsOperand(dbgs(), L->getHeader(), false);
          dbgs() << "\n");
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

    if (!allBlocksValid(*TmpRegion, false /*verifying*/))
      break;

    if (isValidExit(*TmpRegion, false /*verifying*/)) {
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

  if (isValidRegion(R, false /*verifying*/)) {
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

bool SCoPDetection::allBlocksValid(Region &R, bool verifying) const {
  for (Region::block_iterator I = R.block_begin(), E = R.block_end(); I != E;
       ++I)
    if (!isValidBasicBlock(*(I->getNodeAs<BasicBlock>()), R, verifying))
      return false;

  return true;
}

bool SCoPDetection::isValidExit(Region &R, bool verifying) const {
  // PHI nodes are not allowed in the exit basic block.
  if (BasicBlock *Exit = R.getExit()) {
    BasicBlock::iterator I = Exit->begin();
    if (I != Exit->end() && isa<PHINode> (*I)) {
      DEBUG(dbgs() << "PHI node in exit";
            dbgs() << "\n");
      STATSCOP(Other);
      return false;
    }
  }

  return true;
}

bool SCoPDetection::isValidRegion(Region &R, bool verifying) const {
  DEBUG(dbgs() << "Checking region: " << R.getNameStr() << "\n\t");

  // The toplevel region is no valid region.
  if (!R.getParent()) {
    DEBUG(dbgs() << "Top level region is invalid";
          dbgs() << "\n");
    return false;
  }

  if (!allBlocksValid(R, verifying))
    return false;

  if (!isValidExit(R, verifying))
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
  isValidRegion(const_cast<Region&>(R), true /*verifying*/);
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

static RegisterPass<SCoPDetection>
X("polly-detect", "Polly - Detect SCoPs in functions");

