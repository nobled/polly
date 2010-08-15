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
#include "polly/SCoPInfo.h"

#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/RegionPass.h"
#include "llvm/Analysis/RegionIterator.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/ScalarEvolutionExpander.h"

#define DEBUG_TYPE "polly-independent"
#include "llvm/Support/Debug.h"

#include <vector>

using namespace polly;
using namespace llvm;

namespace {
struct IndependentBlocks : public RegionPass {
  Region *region;
  ScalarEvolution *SE;
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
  bool createIndependentBlocks(BasicBlock *BB);
  bool createIndependentBlocks(Region *R);
  bool isIV(const Instruction *I) const;
  bool isIndependentBlock(const Region *R, BasicBlock *BB) const;
  bool areAllBlocksIndependent(Region *R) const;

  bool runOnRegion(Region *R, RGPassManager &RGM);
  void verifyAnalysis() const;
  void getAnalysisUsage(AnalysisUsage &AU) const;
};
}

bool IndependentBlocks::createIndependentBlocks(BasicBlock *BB) {
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
    SE->forgetValue(Inst);
  }

  // The BB changed if we replaced any operand.
  return !work.empty();
}

bool IndependentBlocks::createIndependentBlocks(Region *R) {
  bool Changed = false;

  for (Region::block_iterator SI = R->block_begin(), SE = R->block_end();
       SI != SE; ++SI)
    Changed |= createIndependentBlocks((*SI)->getNodeAs<BasicBlock>());

  return Changed;
}

bool IndependentBlocks::isIV(const Instruction *I) const {
  Loop *L = LI->getLoopFor(I->getParent());

  return L && I == L->getCanonicalInductionVariable();
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

bool IndependentBlocks::areAllBlocksIndependent(Region *R) const {
  for (Region::block_iterator SI = R->block_begin(), SE = R->block_end();
       SI != SE; ++SI)
    if (!isIndependentBlock(R, (*SI)->getNodeAs<BasicBlock>()))
      return false;

  return true;
}

bool IndependentBlocks::runOnRegion(Region *R, RGPassManager &RGM) {
  region = R;
  SE = &getAnalysis<ScalarEvolution>();
  LI = &getAnalysis<LoopInfo>();
  SCoP *S = getAnalysis<SCoPInfo>().getSCoP();

  if (!S) {
    region = 0;
    return false;
  }

  bool Changed = createIndependentBlocks(R);

  verifyAnalysis();

  return Changed;
}

void IndependentBlocks::verifyAnalysis() const {
  assert (areAllBlocksIndependent(region)
          && "Cannot generate independent blocks");
}

void IndependentBlocks::getAnalysisUsage(AnalysisUsage &AU) const {
  // FIXME: If we set preserves cfg, the cfg only passes do not need to
  // be "addPreserved"?
  AU.addRequired<LoopInfo>();
  AU.addPreserved<LoopInfo>();
  AU.addPreserved<DominatorTree>();
  AU.addRequired<ScalarEvolution>();
  AU.addPreserved<ScalarEvolution>();
  AU.addRequired<SCoPInfo>();
  AU.addPreserved<SCoPInfo>();
  AU.setPreservesCFG();
}

char IndependentBlocks::ID = 0;

static RegisterPass<IndependentBlocks>
Z("polly-independent", "Polly - Create independent blocks");

char &polly::IndependentBlocksID = IndependentBlocks::ID;

Pass* polly::createIndependentBlocksPass() {
       return new IndependentBlocks();
}
