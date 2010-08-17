//===------ IndependentBlocks.cpp - Create Independent Blocks in Regions --===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Create independent blocks in the regions detected by SCoPDetection.
//
//===----------------------------------------------------------------------===//
//
#include "polly/LinkAllPasses.h"
#include "polly/SCoPDetection.h"

#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/RegionInfo.h"
#include "llvm/Analysis/RegionPass.h"
#include "llvm/Analysis/RegionIterator.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Transforms/Utils/Local.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/ADT/OwningPtr.h"

#define DEBUG_TYPE "polly-independent"
#include "llvm/Support/Debug.h"

#include <vector>

using namespace polly;
using namespace llvm;

namespace {
struct IndependentBlocks : public RegionPass {
  Region *CurR;
  ScalarEvolution *SE;
  SCoPDetection *SD;
  LoopInfo *LI;

  static char ID;

  IndependentBlocks() : RegionPass(ID) {}

  // Create new code for every instruction operator that can be expressed by a
  // SCEV.  Like this there are just two types of instructions left:
  //
  // 1. Instructions that only reference loop ivs or parameters outside the
  // region.
  //
  // 2. Instructions that are not used for any memory modification. (These
  //    will be ignored later on.)
  //
  // Blocks containing only these kind of instructions are called independent
  // blocks as they can be scheduled arbitrarily.
  bool createIndependentBlocks(BasicBlock *BB, const Region *R);
  bool createIndependentBlocks(const Region *R);

  // Elimination on the SCoP to eliminate the scalar dependences come with
  // trivially dead instructions.
  bool eliminateDeadCode(const Region *R);

  /// @brief Check if the instruction I is the induction variable of a loop.
  ///
  /// @param I The instruction to check.
  ///
  /// @return Return true if I is the induction variable of a loop, false
  ///         otherwise.
  bool isIV(const Instruction *I) const;

  //===--------------------------------------------------------------------===//
  /// Non trivial scalar dependences checking functions.
  /// Non trivial scalar dependences occur when the def and use are located in
  /// different BBs and we can not move them into the same one. This will
  /// prevent use from schedule BBs arbitrarily.
  ///
  /// @brief This function checks if a scalar value that is part of the
  ///        SCoP is used outside of the SCoP.
  ///
  /// @param Use  The use of the instruction.
  /// @param R    The maximum region in the SCoP.
  ///
  /// @return Return true if the Use of an instruction and the instruction
  ///         itself form a non trivial scalar dependence.
  static bool isEscapeUse(const Value *Use, const Region *R);

  /// @brief This function just checks if a Value is either defined in the same
  ///        basic block or outside the region, such that there are no scalar
  ///        dependences between basic blocks that are both part of the same
  ///        region.
  ///
  /// @param Operand  The operand of the instruction.
  /// @param CurBB    The BasicBlock that contains the instruction.
  /// @param R        The maximum region in the SCoP.
  ///
  /// @return Return true if the Operand of an instruction and the instruction
  ///         itself form a non trivial scalar (true) dependence.
  bool isEscapeOperand(const Value *Operand, const BasicBlock *CurBB,
                       const Region *R) const;

  //===--------------------------------------------------------------------===//
  /// Operand tree moving functions.
  /// Trivial scalar dependences can eliminate by move the def to the same BB
  /// that containing use.
  ///
  /// @brief Check if the instruction can be moved to another place safely.
  ///
  /// @param Inst The instruction.
  ///
  /// @return Return true if the instruction can be moved safely, false
  ///         otherwise. 
  static bool isSafeToMove(Instruction *Inst);

  typedef std::map<Instruction*, Instruction*> ReplacedMapType;

  /// @brief Move all safe to move instructions in the Operand Tree (DAG) to
  ///        eliminate trivial scalar dependences.
  ///
  /// @param Inst         The root of the operand Tree.
  /// @param R            The maximum region in the SCoP.
  /// @param ReplacedMap  The map that mapping original instruction to the moved
  ///                     instruction.
  /// @param InsertPos    The insert position of the moved instructions.
  void moveOperandTree(Instruction *Inst, const Region *R,
                       ReplacedMapType &ReplacedMap,
                       Instruction *InsertPos);

  bool isIndependentBlock(const Region *R, BasicBlock *BB) const;
  bool areAllBlocksIndependent(const Region *R) const;

  bool runOnRegion(Region *R, RGPassManager &RGM);
  void verifyAnalysis() const;
  void verifySCoP(const Region *R) const;
  void getAnalysisUsage(AnalysisUsage &AU) const;
};
}

bool IndependentBlocks::isSafeToMove(Instruction *Inst) {
  if (Inst->mayReadFromMemory() ||
      Inst->mayWriteToMemory())
    return false;

  return Inst->isSafeToSpeculativelyExecute();
}

void IndependentBlocks::moveOperandTree(Instruction *Inst, const Region *R,
                                        ReplacedMapType &ReplacedMap,
                                        Instruction *InsertPos) {
  BasicBlock *CurBB = Inst->getParent();

  // Depth first traverse the operand tree (or operand dag, because we will
  // stop at PHINodes, so there are no cycle).
  typedef Instruction::op_iterator ChildIt;
  std::vector<std::pair<Instruction*, ChildIt> > WorkStack;

  WorkStack.push_back(std::make_pair(Inst, Inst->op_begin()));

  while (!WorkStack.empty()) {
    Instruction *CurInst = WorkStack.back().first;
    ChildIt It = WorkStack.back().second;
    DEBUG(dbgs() << "Checking Operand of Node:\n" << *CurInst << "\n------>\n");
    if (It == CurInst->op_end()) {
      // Insert the new instructions in topological order.
      if (!CurInst->getParent())
        CurInst->insertBefore(InsertPos);

      WorkStack.pop_back();
    } else {
      // for each node N,
      Instruction *Operand = dyn_cast<Instruction>(*It);
      ++WorkStack.back().second;

      // Can not move no instruction value.
      if (Operand == 0) continue;

      DEBUG(dbgs() << "For Operand:\n" << *Operand << "\n--->");

      // If the SCoP Region does not contain N, skip it and all its operand and
      // continue. because we reach a "parameter".
      if (!R->contains(Operand)) {
        DEBUG(dbgs() << "Out of region.\n");
        continue;
      }

      // No need to move induction variable.
      if (isIV(Operand)) {
        DEBUG(dbgs() << "is IV.\n");
        continue;
      }

      // We can not move the operand, a non trivial scalar dependence found!
      if (!isSafeToMove(Operand)) {
        DEBUG(dbgs() << "Can not move!\n");
        continue;
      }

      // Do not need to move instruction if it contained in the same BB with
      // the root instruction.
      // FIXME: Remember this in visited Map.
      if (Operand->getParent() == CurBB) {
        DEBUG(dbgs() << "No need to move.\n");
        // Try to move its operand.
        WorkStack.push_back(std::make_pair(Operand, Operand->op_begin()));
        continue;
      }

      // Now we need to move Operand to CurBB.
      // Check if we already moved it.
      ReplacedMapType::iterator At = ReplacedMap.find(Operand);
      if (At != ReplacedMap.end()) {
        DEBUG(dbgs() << "Moved.\n");
        Instruction *MovedOp = At->second;
        It->set(MovedOp);
        // Skip all its children as we already processed them.
        continue;
      } else {
        // Note that NewOp is not inserted in any BB now, we will insert it when
        // it popped form the work stack, so it will be inserted in topological
        // order.
        Instruction *NewOp = Operand->clone();
        NewOp->setName(Operand->getName() + ".moved.to." + CurBB->getName());
        DEBUG(dbgs() << "Move to " << *NewOp << "\n");
        It->set(NewOp);
        ReplacedMap.insert(std::make_pair(Operand, NewOp));
        // Process its operands.
        WorkStack.push_back(std::make_pair(NewOp, NewOp->op_begin()));
      }
    }
  }

  SE->forgetValue(Inst);
}

bool IndependentBlocks::createIndependentBlocks(BasicBlock *BB,
                                                const Region *R) {
  std::vector<Instruction*> WorkList;
  for (BasicBlock::iterator II = BB->begin(), IE = BB->end(); II != IE; ++II)
    if (!isSafeToMove(II) && !isIV(II))
      WorkList.push_back(II);

  ReplacedMapType ReplacedMap;
  Instruction *InsertPos = BB->getFirstNonPHIOrDbg();

  for (std::vector<Instruction*>::iterator I = WorkList.begin(),
      E = WorkList.end(); I != E; ++I)
    moveOperandTree(*I, R, ReplacedMap, InsertPos);

  // The BB was changed if we replaced any operand.
  return !ReplacedMap.empty();
}

bool IndependentBlocks::createIndependentBlocks(const Region *R) {
  bool Changed = false;

  for (Region::const_block_iterator SI = R->block_begin(), SE = R->block_end();
       SI != SE; ++SI)
    Changed |= createIndependentBlocks((*SI)->getNodeAs<BasicBlock>(), R);

  return Changed;
}

bool IndependentBlocks::eliminateDeadCode(const Region *R) {
  std::vector<Instruction*> WorkList;

  // Find all trivially dead instructions.
  for (Region::const_block_iterator SI = R->block_begin(), SE = R->block_end();
      SI != SE; ++SI) {
    BasicBlock *BB = (*SI)->getNodeAs<BasicBlock>();
    for (BasicBlock::iterator I = BB->begin(), E = BB->end(); I != E; ++I)
      if (isInstructionTriviallyDead(I))
        WorkList.push_back(I);
  }

  if (WorkList.empty()) return false;

  // Delete them so the cross BB scalar dependences come with them will
  // also be eliminated.
  while (!WorkList.empty()) {
    RecursivelyDeleteTriviallyDeadInstructions(WorkList.back());
    WorkList.pop_back();
  }

  return true;
}

bool IndependentBlocks::isIV(const Instruction *I) const {
  Loop *L = LI->getLoopFor(I->getParent());

  return L && I == L->getCanonicalInductionVariable();
}

bool IndependentBlocks::isEscapeUse(const Value *Use, const Region *R) {
  // Non-instruction user will never escape.
  if (!isa<Instruction>(Use)) return false;

  return !R->contains(cast<Instruction>(Use));
}

bool IndependentBlocks::isEscapeOperand(const Value *Operand,
                                        const BasicBlock *CurBB,
                                        const Region *R) const {
  const Instruction *OpInst = dyn_cast<Instruction>(Operand);

  // Non-instruction operand will never escape.
  if (OpInst == 0) return false;

  // Induction variables are valid operands.
  if (isIV(OpInst)) return false;

  // A value from a different BB is used in the same region.
  return R->contains(OpInst) && (OpInst->getParent() != CurBB);
}

bool IndependentBlocks::isIndependentBlock(const Region *R,
                                           BasicBlock *BB) const {
  for (BasicBlock::iterator II = BB->begin(), IE = --BB->end();
       II != IE; ++II) {
    Instruction *Inst = &*II;

    if (isIV(Inst))
      continue;

    // A value inside the SCoP is referenced outside.
    for (Instruction::use_iterator UI = Inst->use_begin(),
         UE = Inst->use_end(); UI != UE; ++UI) {
      if (isEscapeUse(*UI, R)) {
        DEBUG(dbgs() << "Instruction not independent:\n");
        DEBUG(dbgs() << "Instruction used outside the SCoP!\n");
        DEBUG(Inst->print(dbgs()));
        DEBUG(dbgs() << "\n");
        return false;
      }
    }

    for (Instruction::op_iterator OI = Inst->op_begin(),
         OE = Inst->op_end(); OI != OE; ++OI) {
      if (isEscapeOperand(*OI, BB, R)) {
        DEBUG(dbgs() << "Instruction in function '";
              WriteAsOperand(dbgs(), BB->getParent(), false);
              dbgs() << "' not independent:\n");
        DEBUG(dbgs() << "Uses invalid operator\n");
        DEBUG(Inst->print(dbgs()));
        DEBUG(dbgs() << "\n");
        DEBUG(dbgs() << "Invalid operator is: ";
              WriteAsOperand(dbgs(), *OI, false);
              dbgs() << "\n");
        return false;
      }
    }
  }

  return true;
}

bool IndependentBlocks::areAllBlocksIndependent(const Region *R) const {
  for (Region::const_block_iterator SI = R->block_begin(), SE = R->block_end();
       SI != SE; ++SI)
    if (!isIndependentBlock(R, (*SI)->getNodeAs<BasicBlock>()))
      return false;

  return true;
}

void IndependentBlocks::getAnalysisUsage(AnalysisUsage &AU) const {
  // FIXME: If we set preserves cfg, the cfg only passes do not need to
  // be "addPreserved"?
  AU.addPreserved<DominatorTree>();
  AU.addRequired<LoopInfo>();
  AU.addPreserved<LoopInfo>();
  AU.addRequired<ScalarEvolution>();
  AU.addPreserved<ScalarEvolution>();
  AU.addRequired<SCoPDetection>();
  AU.addPreserved<SCoPDetection>();
  AU.setPreservesCFG();
}

bool IndependentBlocks::runOnRegion(Region *R, RGPassManager &RGM) {
  LI = &getAnalysis<LoopInfo>();
  SD = &getAnalysis<SCoPDetection>();
  SE = &getAnalysis<ScalarEvolution>();

  if (!SD->isMaxRegionInSCoP(*R)) return false;
  
  CurR = R;
  bool Changed = createIndependentBlocks(CurR);

  DEBUG(dbgs() << "Before Independent Blocks clean up------->\n");
  DEBUG(CurR->getEntry()->getParent()->dump());

  Changed |= eliminateDeadCode(CurR);

  DEBUG(dbgs() << "After Independent Blocks------------->\n");
  DEBUG(CurR->getEntry()->getParent()->dump());
  verifySCoP(CurR);

  return Changed;
}

void IndependentBlocks::verifyAnalysis() const {
  verifySCoP(CurR);
}

void IndependentBlocks::verifySCoP(const Region *R) const {
  assert (areAllBlocksIndependent(R) && "Cannot generate independent blocks");
}

char IndependentBlocks::ID = 0;

static RegisterPass<IndependentBlocks>
Z("polly-independent", "Polly - Create independent blocks");

char &polly::IndependentBlocksID = IndependentBlocks::ID;

Pass* polly::createIndependentBlocksPass() {
  return new IndependentBlocks();
}
