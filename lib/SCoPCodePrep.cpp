//===---- SCoPCodePrep.cpp - Code preparation for SCoP Detect ---*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implement the code preparation for SCoP detect, which will:
//    1. Translate all PHINodes that not induction variable to memory access,
//       this will easier parameter and scalar dependencies checking.
//
//===----------------------------------------------------------------------===//
#include "polly/LinkAllPasses.h"
#include "polly/Support/SCoPHelper.h"

#include "llvm/Instruction.h"
#include "llvm/Pass.h"
#include "llvm/Assembly/Writer.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/RegionInfo.h"
#include "llvm/Analysis/Dominators.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/PointerIntPair.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/Transforms/Utils/Local.h"

#define DEBUG_TYPE "polly-code-prep"
#include "llvm/Support/Debug.h"


using namespace llvm;
using namespace polly;

namespace {
//===----------------------------------------------------------------------===//
/// @brief SCoP Code Preparation - Perform some transforms to make scop detect
/// easier.
///
class SCoPCodePrep : public FunctionPass {
  // DO NOT IMPLEMENT.
  SCoPCodePrep(const SCoPCodePrep &);
  // DO NOT IMPLEMENT.
  const SCoPCodePrep &operator=(const SCoPCodePrep &);

  // LoopInfo to compute canonical induction variable.
  LoopInfo *LI;

  // Clear the context.
  void clear();

  bool eliminatePHINodes(Function &F);

public:
  static char ID;

  explicit SCoPCodePrep() : FunctionPass(ID) {}
  ~SCoPCodePrep();

  /// @name FunctionPass interface.
  //@{
  virtual void getAnalysisUsage(AnalysisUsage &AU) const;
  virtual void releaseMemory();
  virtual bool runOnFunction(Function &F);
  virtual void print(raw_ostream &OS, const Module *) const;
  //@}

};
}

//===----------------------------------------------------------------------===//
/// SCoPCodePrep implement.

void SCoPCodePrep::clear() {
}

SCoPCodePrep::~SCoPCodePrep() {
  clear();
}

bool SCoPCodePrep::eliminatePHINodes(Function &F) {
  // The PHINodes that will be deleted.
  std::vector<PHINode*> PNtoDel;
  // The PHINodes that will be preserved.
  std::vector<PHINode*> PreservedPNs;

  // Scan the PHINodes in this function.
  for (Function::iterator ibb = F.begin(), ibe = F.end();
      ibb != ibe; ++ibb)
    for (BasicBlock::iterator iib = ibb->begin(), iie = ibb->getFirstNonPHI();
        iib != iie; ++iib)
      if (PHINode *PN = cast<PHINode>(iib)) {
        if (Loop *L = LI->getLoopFor(ibb)) {
          // Induction variable will be preserved.
          if (L->getCanonicalInductionVariable() == PN) {
            PreservedPNs.push_back(PN);
            continue;
          }
        }

        // As DemotePHIToStack does not support invoke edges, we preserve
        // PHINodes that have invoke edges.
        if (hasInvokeEdge(PN))
          PreservedPNs.push_back(PN);
        else
          PNtoDel.push_back(PN);
      }

  if (PNtoDel.empty())
    return false;

  // Eliminate the PHINodes that not an Induction variable.
  while (!PNtoDel.empty()) {
    PHINode *PN = PNtoDel.back();
    PNtoDel.pop_back();

    DemotePHIToStack(PN);
  }

  // Move all preserved PHINodes to the beginning of the BasicBlock.
  while (!PreservedPNs.empty()) {
    PHINode *PN = PreservedPNs.back();
    PreservedPNs.pop_back();

    BasicBlock *BB = PN->getParent();
    if (PN == BB->begin())
      continue;

    PN->moveBefore(BB->begin());
  }

  return true;
}

void SCoPCodePrep::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<LoopInfo>();
  AU.setPreservesAll();
}

bool SCoPCodePrep::runOnFunction(Function &F) {
  LI = &getAnalysis<LoopInfo>();

  eliminatePHINodes(F);

  return false;
}

void SCoPCodePrep::releaseMemory() {
  clear();
}

void SCoPCodePrep::print(raw_ostream &OS, const Module *) const {
}

char SCoPCodePrep::ID = 0;

RegisterPass<SCoPCodePrep> X("polly-prepare",
                              "Polly - Prepare code for polly.",
                              false, true);

char &polly::SCoPCodePrepID = SCoPCodePrep::ID;

Pass *polly::createSCoPCodePrepPass() {
  return new SCoPCodePrep();
}
