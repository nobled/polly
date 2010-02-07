
//===- RegionPrinter.cpp - SESE region detection analysis ---------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
// print out the region information of a function
//===----------------------------------------------------------------------===//

#include "llvm/Analysis/RegionInfo.h"
#include "llvm/Analysis/RegionPass.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {
class PollyPass : public RegionPass {
  raw_ostream *Out;       // raw_ostream to print on
  bool DeleteStream;      // Delete the ostream in our dtor?
public:
  static char ID;

  PollyPass() : RegionPass(&ID), Out(&dbgs()), DeleteStream(false){}
  PollyPass(raw_ostream *o, bool DS) : RegionPass(&ID), Out(o), DeleteStream(DS){}

  ~PollyPass() {
      if (DeleteStream) delete Out;
  }

  bool runOnRegion(Region *R, RGPassManager &RGM) {
    //(*Out) << "printing regions:\n";
    //dont print the tree out
    R->print(*Out, false);
    //(*Out) << "End\n";
    return false;
  }

  virtual void getAnalysisUsage(AnalysisUsage &AU) const {
    AU.setPreservesAll();
  }
};

} //end anonymous namespace

char PollyPass::ID = 0;

static RegisterPass<PollyPass>
X("polly", "Apply polyhedral transformations");


RegionPass* createPollyPass() {
  return new PollyPass();
}
