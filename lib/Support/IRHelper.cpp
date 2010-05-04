//===- IRHelper.cpp - Recreate LLVM IR from the SCoP.  ---------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Small functions that help with LLVM-IR.
//
//===----------------------------------------------------------------------===//

#include "polly/Support/IRHelper.h"

#include "llvm/Analysis/RegionInfo.h"
#include "llvm/Support/CFG.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

using namespace llvm;

BasicBlock *polly::createSingleEntryEdge(Region *R, Pass *P) {
  BasicBlock *BB = R->getEntry();

  BasicBlock::iterator SplitIt = BB->begin();

  while (isa<PHINode>(SplitIt))
    ++SplitIt;

  BasicBlock *newBB = SplitBlock(BB, SplitIt, P);

  for (BasicBlock::iterator PI = BB->begin(); isa<PHINode>(PI); ++PI) {
    PHINode *PN = cast<PHINode>(PI);
    PHINode *NPN =
        PHINode::Create(PN->getType(), PN->getName()+".ph", newBB->begin());

    for (pred_iterator PI = pred_begin(BB), PE = pred_end(BB); PI != PE; ++PI) {
      if (R->contains(*PI)) {
        Value *V = PN->removeIncomingValue(*PI, false);
        NPN->addIncoming(V, *PI);
      }
    }
    PN->replaceAllUsesWith(NPN);
    NPN->addIncoming(PN,BB);
 }

 for (pred_iterator PI = pred_begin(BB), PE = pred_end(BB); PI != PE; ++PI)
   if (R->contains(*PI))
     (*PI)->getTerminator()->replaceUsesOfWith(BB, newBB);

 return newBB;
}

void polly::createSingleExitEdge(Region *R, Pass *P) {
  BasicBlock *BB = R->getExit();
  int num = 0, i = 0;

  for (pred_iterator PI = pred_begin(BB), PE = pred_end(BB); PI != PE; ++PI)
    if (R->contains(*PI))
      ++num;

  BasicBlock **Preds = new BasicBlock *[num];

  for (pred_iterator PI = pred_begin(BB), PE = pred_end(BB); PI != PE; ++PI) {
    if (R->contains(*PI)) {
      Preds[i] = *PI;
      ++i;
    }
  }

  SplitBlockPredecessors(BB, Preds, num, ".region", P);

  delete[] Preds;
}
