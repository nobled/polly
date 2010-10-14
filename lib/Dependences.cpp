//===- Dependency.cpp - Recreate LLVM IR from the SCoP.  ------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Calculate dependency information for a SCoP.
//
//===----------------------------------------------------------------------===//

#include "polly/LinkAllPasses.h"
#include "polly/SCoPInfo.h"

#include "llvm/Analysis/RegionPass.h"

#include <isl_flow.h>

using namespace polly;
using namespace llvm;

namespace {

class Dependences : public RegionPass {

  SCoP *S;

public:
  static char ID;

  Dependences() : RegionPass(ID), S(0) {}

  bool runOnRegion(Region *R, RGPassManager &RGM) {
    S = getAnalysis<SCoPInfo>().getSCoP();

    if (!S)
      return false;

    return false;
  }

  void print(raw_ostream &OS, const Module *) const {}

  virtual void releaseMemory() { S = 0; }

  virtual void getAnalysisUsage(AnalysisUsage &AU) const {
    AU.addRequired<SCoPInfo>();
    AU.setPreservesAll();
  }
};
} //end anonymous namespace

char Dependences::ID = 0;

static RegisterPass<Dependences>
X("polly-dependences", "Polly - Calculate dependences for SCoP");

Pass* polly::createDependencesPass() {
  return new Dependences();
}
