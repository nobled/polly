//===- SCoPDetection.cpp - Detect the maximal SCoP regions ----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
// Detect maximal SCoPs regions.
//===----------------------------------------------------------------------===//

#include "polly/SCoPDetection.h"

#include "llvm/Analysis/RegionInfo.h"
#include "llvm/Analysis/RegionIterator.h"
#include "llvm/Analysis/ScalarEvolutionExpressions.h"


#define DEBUG_TYPE "polly-scopdet"
#include "llvm/Support/Debug.h"
#include "llvm/Instruction.h"

using namespace llvm;

namespace polly {
SCoPDetection::SCoPDetection() : FunctionPass(&ID) {}

bool SCoPDetection::isValid(Instruction *I) {
  if (I->isBinaryOp())
    return true;

  if (isa<LoadInst>(I) || isa<StoreInst>(I) || isa<AllocaInst>(I)
      || isa<ICmpInst>(I) || isa<FCmpInst>(I) || isa<GetElementPtrInst>(I))
    return true;

  if (isa<BranchInst>(I)) {
    BranchInst *branchInst = static_cast<BranchInst*>(I);
    if (branchInst->isUnconditional())
      return true;

    Loop *loop = LI->getLoopFor(I->getParent());
    // Only allow branches that are loop exits. This stops anything
    // except loops that have just one exit and are detected by our LoopInfo
    // analysis
    if (!loop || loop->getExitingBlock() == I->getParent())
      return true;

    DEBUG(dbgs() << "\tInvalid Instruction in BB: "
          << I->getParent()->getNameStr() << "\n\t\t"
          << (*I) << "\n");
    return false;
  }

  if (I->isCast()) {
    if (isa<IntToPtrInst>(I) || isa<BitCastInst>(I)) {
      DEBUG(dbgs() << "\tInvalid Instruction in BB: "
            << I->getParent()->getNameStr() << "\n\t\t"
            << (*I) << "\n");
      return false;
    } else {
      return true;
    }
  }

  if (isa<PHINode>(I))
    return true;

  DEBUG(dbgs() << "\tInvalid Instruction in BB: "
        << I->getParent()->getNameStr() << "\n\t\t"
        << (*I) << "\n");
  return false;
}

bool SCoPDetection::isValid(BasicBlock *BB) {
  for (BasicBlock::iterator II = BB->begin(), IE = BB->end();
       II != IE; ++II)
    if(!isValid(&(*II)))
      return false;

  return true;
}

bool SCoPDetection::isValidLoop(Loop *L) {
  const SCEV *scev = SE->getBackedgeTakenCount(L);

  if (scConstant != scev->getSCEVType())
    return false;

  return true;
}

static bool contains(Region *R, Loop *L) {
   return R->contains(L->getHeader()) && R->contains(L->getExitingBlock());
}

bool SCoPDetection::hasValidLoops(Region *R) {
  BasicBlock *BB = R->getEntry();
  Loop *L = LI->getLoopFor(BB);

  // Find the first Loop that is not contained in the region.
  while (L && L->getParentLoop() && contains(R, L))
    L = L->getParentLoop();

  std::vector<Loop*> todo;

  if (L)
    todo.push_back(L);
  else
    todo.insert(todo.end(), LI->begin(), LI->end());

  while (!todo.empty()) {
    L = todo.back();
    todo.pop_back();

    if (contains(R, L) && !isValidLoop(L)) {
      DEBUG(dbgs() << "\tInvalid loop starting at BB:"
            << L->getHeader()->getNameStr() << "\n");
      return false;
    }

    todo.insert(todo.end(), L->begin(), L->end());
  }

  return true;
}

bool SCoPDetection::isValid(Region *R) {

  DEBUG(dbgs() << "Checking Region: " << R->getNameStr() << "\n");

  for (Region::block_iterator BI = R->block_begin(), BE = R->block_end();
       BI != BE; ++BI)
    if (!isValid((*BI)->getNodeAs<BasicBlock>()))
      return false;


  if (!hasValidLoops(R))
    return false;

  DEBUG(dbgs() << "\tOK\n");
  return true;
}

void SCoPDetection::detectMaxSCoPs() {
  RegionVector RegionsToCheck;
  RegionsToCheck.push_back(RI->getTopLevelRegion());

  while (!RegionsToCheck.empty()) {
    Region *region = RegionsToCheck.back();
    RegionsToCheck.pop_back();

    if (isValid(region))
      SCoPRegions.insert(region);
    else
      RegionsToCheck.insert(RegionsToCheck.end(), region->begin(),
                            region->end());
  }
}

bool SCoPDetection::isMaxValid(Region *R) {
  return SCoPRegions.find(R) != SCoPRegions.end();
}

bool SCoPDetection::runOnFunction(Function &F) {
  releaseMemory();

  RI = &getAnalysis<RegionInfo>();
  LI = &getAnalysis<LoopInfo>();
  SE = &getAnalysis<ScalarEvolution>();

  detectMaxSCoPs();
  return false;
}

void SCoPDetection::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
  AU.addRequired<RegionInfo>();
  AU.addRequired<LoopInfo>();
  AU.addRequired<ScalarEvolution>();
}

void SCoPDetection::print(raw_ostream &OS, const Module *) const {

  for (RegionSet::const_iterator RI = SCoPRegions.begin(),
       RE = SCoPRegions.end(); RI != RE; ++RI)
    OS << (**RI) << "\n";
}

void SCoPDetection::verifyAnalysis() const {
}

char SCoPDetection::ID = 0;
RegisterPass<SCoPDetection> A("polly-scop-detection", "Detect maximal SCoPs",
                              true, true);

FunctionPass *createSCoPDetectionPass() {
  return new SCoPDetection();
}
} // End namespace polly
