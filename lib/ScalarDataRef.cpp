//===----- ScalarDataRef.cpp - Capture scalar data reference ----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implement the scalar data reference analysis, which will capture
// scalar reads/writes(use/def) in SCoP statement(BB or Region).
//
//===----------------------------------------------------------------------===//
#include "polly/LinkAllPasses.h"

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

#define DEBUG_TYPE "polly-scalar-data-ref"
#include "llvm/Support/Debug.h"


using namespace llvm;
using namespace polly;

namespace {
//===----------------------------------------------------------------------===//
/// @brief Scalar data reference - Translat scalar data dependencies to one
/// element array access.
///
class ScalarDataRef : public FunctionPass {
  // DO NOT IMPLEMENT.
  ScalarDataRef(const ScalarDataRef &);
  // DO NOT IMPLEMENT.
  const ScalarDataRef &operator=(const ScalarDataRef &);

  // LoopInfo to compute canonical induction variable.
  LoopInfo *LI;
  //
  ScalarEvolution *SE;

  // Clear the context.
  void clear();

  bool eliminatePHINodes(Function &F);

public:
  static char ID;

  explicit ScalarDataRef() : FunctionPass(&ID) {}
  ~ScalarDataRef();

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
/// ScalarDataRef implement.

void ScalarDataRef::clear() {
}

ScalarDataRef::~ScalarDataRef() {
  clear();
}

bool ScalarDataRef::eliminatePHINodes(Function &F) {
  std::vector<PHINode*> PNtoDel;
  std::vector<PHINode*> IVs;


  for (Function::iterator ibb = F.begin(), ibe = F.end();
      ibb != ibe; ++ibb)
    for (BasicBlock::iterator iib = ibb->begin(), iie = ibb->end();
        iib != iie; ++iib)
      if (PHINode *PN = dyn_cast<PHINode>(iib)) {
        if (Loop *L = LI->getLoopFor(ibb)) {
          if (L->getCanonicalInductionVariable() == PN) {
            IVs.push_back(PN);
            continue;
          }
        }

        PNtoDel.push_back(PN);
      }

  if (PNtoDel.empty())
    return false;

  // Eliminate PHI Node.
  while (!PNtoDel.empty()) {
    PHINode *PN = PNtoDel.back();
    PNtoDel.pop_back();

    DemotePHIToStack(PN);
  }

  // Move all left PHINodes (IVs) to the beginning of the BasicBlock.
  while (!IVs.empty()) {
    PHINode *PN = IVs.back();
    IVs.pop_back();

    BasicBlock *BB = PN->getParent();
    if (PN == BB->begin())
      continue;

    PN->removeFromParent();
    BB->getInstList().push_front(PN);

  }

  return true;
}

void ScalarDataRef::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<RegionInfo>();
  AU.addRequired<LoopInfo>();
  AU.setPreservesAll();
}

bool ScalarDataRef::runOnFunction(Function &F) {
  LI = &getAnalysis<LoopInfo>();

  eliminatePHINodes(F);

  return false;
}

void ScalarDataRef::releaseMemory() {
  clear();
}

void ScalarDataRef::print(raw_ostream &OS, const Module *) const {
}

char ScalarDataRef::ID = 0;

RegisterPass<ScalarDataRef> X("polly-scalar-data-ref",
                              "Polly - Scalar Data Reference Analysis",
                              false, true);

Pass *polly::createScalarDataRefPass() {
  return new ScalarDataRef();
}
