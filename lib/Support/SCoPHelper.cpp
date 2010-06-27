//===- SCoPHelper.cpp - Some Helper Functions for SCoP.  -------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Small functions that help with SCoP and LLVM-IR.
//
//===----------------------------------------------------------------------===//

#include "polly/Support/SCoPHelper.h"

#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/RegionInfo.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/ScalarEvolutionExpressions.h"
#include "llvm/Support/CFG.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#define DEBUG_TYPE "polly-scop-helper"
#include "llvm/Support/Debug.h"

using namespace llvm;

// Helper function for SCoP
// TODO: Add assertion to not allow parameter to be null
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

Value *polly::getPointerOperand(Instruction &Inst) {
  if (LoadInst *load = dyn_cast<LoadInst>(&Inst))
    return load->getPointerOperand();
  else if (StoreInst *store = dyn_cast<StoreInst>(&Inst))
    return store->getPointerOperand();
  // TODO: GEP also have a pointer operand

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

// Helper function for LLVM-IR about SCoP
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
