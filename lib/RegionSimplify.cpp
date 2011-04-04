//===- RegionSimplify.cpp -------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file converts refined regions detected by the RegionInfo analysis
// into simple regions.
//
//===----------------------------------------------------------------------===//

#include "polly/LinkAllPasses.h"

#include "llvm/Instructions.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/Dominators.h"
#include "llvm/Analysis/RegionPass.h"
#include "llvm/Analysis/RegionInfo.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#define DEBUG_TYPE "region-simplify"

using namespace llvm;

STATISTIC(NumEntries, "The # of created entry edges");
STATISTIC(NumExits, "The # of created exit edges");

namespace {
class RegionSimplify: public RegionPass {
  bool modified;
  Region *CR;
  void createSingleEntryEdge(Region *R);
  void createSingleExitEdge(Region *R);
public:
  static char ID;
  explicit RegionSimplify() :
    RegionPass(ID) {
  }

  virtual void print(raw_ostream &O, const Module *M) const;

  virtual bool runOnRegion(Region *R, RGPassManager &RGM);
  virtual void getAnalysisUsage(AnalysisUsage &AU) const;
};
}

static RegisterPass<RegionSimplify>
X("polly-region-simplify", "Transform refined regions into simple regions");

char RegionSimplify::ID = 0;
namespace polly {
  Pass *createRegionSimplifyPass() {
    return new RegionSimplify();
  }
}

void RegionSimplify::print(raw_ostream &O, const Module *M) const {
  if (!modified)
    return;

  BasicBlock *enteringBlock = CR->getEnteringBlock();
  BasicBlock *exitingBlock = CR->getExitingBlock();

  O << "\nRegion: [" << CR->getNameStr() << "] Edges:\n";
  if (enteringBlock)
    O << "  Entry: ]" << enteringBlock->getNameStr() << " => "
        << enteringBlock->getNameStr() << "]\n";
  if (exitingBlock)
    O << "  Exit: [" << exitingBlock->getNameStr() << " => "
        << exitingBlock->getNameStr() << "[\n";

  O << "\n";
}

void RegionSimplify::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addPreserved<DominatorTree> ();
  AU.addPreserved<RegionInfo> ();
  AU.addRequired<RegionInfo> ();
}

// createSingleEntryEdge - Split the entry basic block of the given
// region after the last PHINode to form a single entry edge.
void RegionSimplify::createSingleEntryEdge(Region *R) {
  BasicBlock *oldEntry = R->getEntry();
  SmallVector<BasicBlock*, 4> Preds;
  for (pred_iterator PI = pred_begin(oldEntry), PE = pred_end(oldEntry);
       PI != PE; ++PI)
    if (!R->contains(*PI))
      Preds.push_back(*PI);

  assert(Preds.size() && "This region has already a single entry edge");

  BasicBlock *newEntry = SplitBlockPredecessors(oldEntry,
                                                Preds.data(), Preds.size(),
                                                ".single_entry", this);

  RegionInfo *RI = &getAnalysis<RegionInfo> ();
  // We do not update entry node for children of this region.
  // This make it easier to extract children regions because they do not share
  // the entry node with their parents.
  // all parent regions whose entry is oldEntry are updated with newEntry
  Region *r = R->getParent();
  while (r->getEntry() == oldEntry && !r->isTopLevelRegion()) {
    r->replaceEntry(newEntry);
    r = r->getParent();
  }

  // We do not update exit node for children of this region for the same reason
  // of not updating entry node.
  // All other regions whose exit is oldEntry are updated with new exit node
  r = RI->getTopLevelRegion();
  std::vector<Region *> RQ;
  RQ.push_back(r);

  while (!RQ.empty()){
    r = RQ.back();
    RQ.pop_back();

    for (Region::const_iterator RI = r->begin(), RE = r->end(); RI!=RE; ++RI)
      RQ.push_back(*RI);

    if (r->getExit() == oldEntry && !R->contains(r))
      r->replaceExit(newEntry);
  }

}

// createSingleExitEdge - Split the exit basic of the given region
// to form a single exit edge.
void RegionSimplify::createSingleExitEdge(Region *R) {
  BasicBlock *oldExit = R->getExit();

  SmallVector<BasicBlock*, 4> Preds;
  for (pred_iterator PI = pred_begin(oldExit), PE = pred_end(oldExit);
      PI != PE; ++PI)
    if (R->contains(*PI))
      Preds.push_back(*PI);

  BasicBlock *newExit =  SplitBlockPredecessors(oldExit,
                                                Preds.data(), Preds.size(),
                                                ".single_exit", this);
  RegionInfo *RI = &getAnalysis<RegionInfo>();

  // We do not need to update entry nodes because this split happens inside
  // this region and it affects only this region and all of its children.
  // The new split node belongs to this region
  RI->setRegionFor(newExit,R);

  // all children of this region whose exit is oldExit is changed to newExit
  std::vector<Region *> RQ;
  for (Region::const_iterator RI = R->begin(), RE = R->end(); RI!=RE; ++RI)
    RQ.push_back(*RI);

  while (!RQ.empty()){
    R = RQ.back();
    RQ.pop_back();

    if (R->getExit() != oldExit)
      continue;

    for (Region::const_iterator RI = R->begin(), RE = R->end(); RI!=RE; ++RI)
      RQ.push_back(*RI);

    R->replaceExit(newExit);
  }

}

bool RegionSimplify::runOnRegion(Region *R, RGPassManager &RGM) {
  modified = false;

  if (!R->isTopLevelRegion()) {

    // split entry node if the region has multiple entry edges
    if (!(R->getEnteringBlock())
        && (pred_begin(R->getEntry()) != pred_end(R->getEntry()))) {
      createSingleEntryEdge(R);
      modified = true;
      ++NumEntries;
    }

    // split exit node if the region has multiple exit edges
    if (!(R->getExitingBlock())) {
      createSingleExitEdge(R);
      modified = true;
      ++NumExits;
    }
  }

  return modified;
}
