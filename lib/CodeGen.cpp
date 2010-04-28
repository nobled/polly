//===- CodeGen.cpp - Recreate LLVM IR from the SCoP.  ---------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Recreate LLVM IR from the SCoP.
//
//===----------------------------------------------------------------------===//

#include "polly/CLooG.h"
#include "polly/Support/IRHelper.h"
#include "llvm/Analysis/RegionPass.h"
#include "llvm/Support/CFG.h"
#include "llvm/Support/Debug.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#define CLOOG_INT_GMP 1
#include "cloog/isl/domain.h"

#ifdef _WINDOWS
#define snprintf _snprintf
#endif

using namespace polly;
using namespace llvm;

namespace {

class ScopPrinter : public RegionPass {

  SCoP *S;

public:
  static char ID;

  ScopPrinter() : RegionPass(&ID), S(0) {}

  bool runOnRegion(Region *R, RGPassManager &RGM) {
    S = getAnalysis<SCoPInfo>().getSCoP();
    return false;
  }

  void print(raw_ostream &OS, const Module *) const {

    if (!S) {
      OS << "Invalid SCoP\n";
      return;
    }
    OS << "SCoP: " << S->getRegion().getNameStr() << "\n";
    CLooG C = CLooG(S);
    struct clast_stmt *clast = C.getClast();
    OS << "Generated CLAST '" << clast << "'\n";
    C.pprint();
  }

  virtual void releaseMemory() { S = 0; }

  virtual void getAnalysisUsage(AnalysisUsage &AU) const {
    // XXX: Cannot be removed, as otherwise LLVM crashes.
    AU.setPreservesAll();
    AU.addRequired<SCoPInfo>();
  }
};
} //end anonymous namespace

char ScopPrinter::ID = 0;

static RegisterPass<ScopPrinter>
X("polly-print-scop", "Polly - Print SCoP as C code");

Pass* polly::createScopPrinterPass() {
  return new ScopPrinter();
}

namespace {
class ScopCodeGen : public RegionPass {

  SCoP *S;
  DominatorTree *DT;

public:
  static char ID;

  ScopCodeGen() : RegionPass(&ID), S(0) {}

  succ_iterator insertNewCodeBranch() {
    Region *region = const_cast<Region*>(&S->getRegion());
    BasicBlock *SEntry = *succ_begin(region->getEntry());
    BasicBlock *SExit;

    for (pred_iterator PI = pred_begin(region->getExit()),
         PE = pred_end(region->getExit()); PI != PE; ++PI) {
      if (region->contains(*PI))
        SExit = *PI;
    }

    SplitEdge(region->getEntry(), SEntry, this);
    BasicBlock *branch = *succ_begin(region->getEntry());
    TerminatorInst *oldT = branch->getTerminator();
    BranchInst::Create(*succ_begin(branch), SExit,
                       ConstantInt::getFalse(branch->getContext()), branch);
    oldT->eraseFromParent();

    //TODO: Insert PHI nodes for every Value used outside of region

    for (succ_iterator SI = succ_begin(branch),
         SE = succ_end(branch); SI != SE; ++SI)
      if (*SI == SExit)
        return SI;
    return succ_end(branch);
  }

  void createLoop(succ_iterator edge) {
    BasicBlock *dest = *edge;
    BasicBlock *src = edge.getSource();
    BasicBlock *loop = SplitEdge(src, dest, this);
    TerminatorInst *oldT = loop->getTerminator();
    //IRBuilder LBuilder(loop);
    PHINode *NPN =
        PHINode::Create(IntegerType::getInt64Ty(loop->getContext()), "asph",
                        loop->begin());
    BranchInst::Create(dest, loop,
                       ConstantInt::getFalse(loop->getContext()), loop);
    NPN->addIncoming(NPN, loop);
    NPN->addIncoming(
      ConstantInt::get(IntegerType::getInt64Ty(loop->getContext()), 0, false),
      src);

    //BasicBlock *latch = SplitEdge(loop, loop, this);
    oldT->eraseFromParent();
   }

  bool runOnRegion(Region *R, RGPassManager &RGM) {
    S = getAnalysis<SCoPInfo>().getSCoP();
    DT = &getAnalysis<DominatorTree>();

    if (!S)
      return false;

    createSingleEntryEdge(R, this);
    createSingleExitEdge(R, this);
    succ_iterator edge = insertNewCodeBranch();
    createLoop(edge);

    return true;
  }

  void print(raw_ostream &OS, const Module *) const {
  }

  virtual void getAnalysisUsage(AnalysisUsage &AU) const {
    // XXX: Cannot be removed, as otherwise LLVM crashes.
    AU.addRequired<SCoPInfo>();
    AU.addRequired<DominatorTree>();
    AU.addPreserved<DominatorTree>();
  }

  virtual void releaseMemory() { S = 0; }

};
} //end anonymous namespace

char ScopCodeGen::ID = 0;

static RegisterPass<ScopCodeGen>
Y("polly-codegen-scop", "Polly - Code generation");

Pass* polly::createScopCodeGenPass() {
  return new ScopCodeGen();
}
