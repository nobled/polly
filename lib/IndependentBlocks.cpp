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

#define DEBUG_TYPE "polly-independent"

#include "polly/LinkAllPasses.h"
#include "polly/SCoPDetection.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/IRBuilder.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/RegionPass.h"
#include "llvm/Analysis/RegionIterator.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/ScalarEvolutionExpander.h"


#include <vector>

using namespace polly;
using namespace llvm;

namespace {
struct IndependentBlocks : public FunctionPass {
  ScalarEvolution *SE;
  LoopInfo *LI;
  SCoPDetection *SD;
  RegionInfo *RI;

  static char ID;

  IndependentBlocks() : FunctionPass(&ID) {}

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
  void createIndependentBlocks(BasicBlock *BB);
  void createIndependentBlocks(Region *R);
  
  bool isIV(Instruction *I) const;
  
  bool isIndependentBlock(const Region *R, BasicBlock *BB) const;
  bool areAllBlocksIndependent(const Region *R) const;
  
  bool runOnFunction(Function &F);
  void runOnRegion(Region *R);

  void verifyAnalysis() const;
  void verifyRegion(const Region *R) const;

  virtual void getAnalysisUsage(AnalysisUsage &AU) const;
};
}

void IndependentBlocks::createIndependentBlocks(BasicBlock *BB) {
  std::vector<Instruction*> work;
  SCEVExpander Rewriter(*SE);
  Rewriter.disableInstructionHoisting();

  for (BasicBlock::iterator II = BB->begin(), IE = BB->end();
       II != IE; ++II) {
      Instruction *Inst = &*II;

      if (!isa<PHINode>(Inst))
        work.push_back(Inst);
    }

  for (std::vector<Instruction*>::iterator II = work.begin(), IE = work.end();
       II != IE; ++II) {
    Instruction *Inst = *II;

    for (Instruction::op_iterator UI = Inst->op_begin(),
         UE = Inst->op_end(); UI != UE; ++UI) {
      if (!SE->isSCEVable(UI->get()->getType()))
        continue;

      const SCEV *Scev = SE->getSCEV(*UI);

      Value *V = Rewriter.expandCodeFor(Scev, UI->get()->getType(), Inst);
      UI->set(V);
    }
  }
}

void IndependentBlocks::createIndependentBlocks(Region *R) {
  for (Region::block_iterator SI = R->block_begin(), SE = R->block_end();
       SI != SE; ++SI)
    createIndependentBlocks((*SI)->getNodeAs<BasicBlock>());
}

bool IndependentBlocks::isIV(Instruction *I) const {
  Loop *L = LI->getLoopFor(I->getParent());

  return L && I == L->getCanonicalInductionVariable();
}

bool IndependentBlocks::isIndependentBlock(const Region *R,
                                           BasicBlock *BB) const {
  for (BasicBlock::iterator II = BB->begin(), IE = BB->end();
       II != IE; ++II) {
    Instruction *Inst = &*II;

    if (isIV(Inst))
      continue;

    // A value inside the SCoP is referenced outside.
    for (Instruction::use_iterator UI = Inst->use_begin(),
         UE = Inst->use_end(); UI != UE; ++UI) {
      Instruction *Use = dyn_cast<Instruction>(*UI);

      if (Use && !R->contains(Use)) {
        DEBUG(dbgs() << "Instruction not independent:\n");
        DEBUG(dbgs() << "Instruction used outside the SCoP!\n");
        DEBUG(Inst->print(dbgs()));
        DEBUG(dbgs() << "\n");
        return false;
      }
    }

    for (Instruction::op_iterator UI = Inst->op_begin(),
         UE = Inst->op_end(); UI != UE; ++UI) {
      Instruction *Op = dyn_cast<Instruction>(&(*UI));

      if (Op && R->contains(Op) && !(Op->getParent() == BB)
          && !isIV(Op)) {
        DEBUG(dbgs() << "Instruction in function '";
              WriteAsOperand(dbgs(), BB->getParent(), false);
              dbgs() << "' not independent:\n");
        DEBUG(dbgs() << "Uses invalid operator\n");
        DEBUG(Inst->print(dbgs()));
        DEBUG(dbgs() << "\n");
        DEBUG(dbgs() << "Invalid operator is: ";
              WriteAsOperand(dbgs(), Op, false);
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


void IndependentBlocks::verifyAnalysis() const {
  verifyRegion(RI->getTopLevelRegion());
}

void IndependentBlocks::verifyRegion(const Region *R) const {
  if (!SD->isMaxRegionInSCoP(*R)) {
    for (Region::const_iterator I = R->begin(), E = R->end(); I != E; ++I)
      verifyRegion(*I);

    // Only verify the maximal SCoPs.
    return;
  }

  assert (areAllBlocksIndependent(R) && "Independent block found!");
}

void IndependentBlocks::runOnRegion(Region *R) {
  if (!SD->isMaxRegionInSCoP(*R)) {
    for (Region::iterator I = R->begin(), E = R->end(); I != E; ++I)
      runOnRegion(*I);

    // Only handle the maximal SCoPs.
    return;
  }

  createIndependentBlocks(R);
}

bool IndependentBlocks::runOnFunction(Function &F) {
  SD = &getAnalysis<SCoPDetection>();
  SE = &getAnalysis<ScalarEvolution>();
  LI = &getAnalysis<LoopInfo>();
  RI = &getAnalysis<RegionInfo>();

  runOnRegion(RI->getTopLevelRegion());

  DEBUG(dbgs() << "After create indepnedent blocks:\n");
  DEBUG(F.dump());

  verifyAnalysis();
  // The function change after we create independent blocks.
  return true;
}

void IndependentBlocks::getAnalysisUsage(AnalysisUsage &AU) const {
  // FIXME: If we set preserves cfg, the cfg only passes do not need to
  // be "addPreserved"?
  AU.addRequired<LoopInfo>();
  AU.addPreserved<LoopInfo>();
  AU.addPreserved<DominatorTree>();
  // NOTE: ScalarEvolution not preserved.
  AU.addRequired<ScalarEvolution>();
  AU.addRequired<RegionInfo>();
  AU.addPreserved<RegionInfo>();
  AU.addRequired<SCoPDetection>();
  AU.addPreserved<SCoPDetection>();
  AU.setPreservesCFG();
}

char IndependentBlocks::ID = 0;

static RegisterPass<IndependentBlocks>
Z("polly-independent", "Polly - Create independent blocks");

const PassInfo *const polly::IndependentBlocksID = &Z;

Pass* polly::createIndependentBlocksPass() {
       return new IndependentBlocks();
}