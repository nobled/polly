//===- Pocc.cpp - Pocc interface ----------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Pocc[1] interface.
//
// Pocc, the polyhedral compilation collection is a collection of polyhedral
// tools. It is used as an optimizer in polly
//
// [1] http://www-roc.inria.fr/~pouchet/software/pocc/
//
//===----------------------------------------------------------------------===//

#include "polly/Cloog.h"
#include "polly/LinkAllPasses.h"
#include "polly/ScopInfo.h"

using namespace llvm;
using namespace polly;

namespace {

  struct Pocc : public ScopPass {
    static char ID;
    explicit Pocc() : ScopPass(ID) {}

    std::string getFileName(Region *R) const;
    virtual bool runOnScop(Scop &S);
    void getAnalysisUsage(AnalysisUsage &AU) const;
  };

}
char Pocc::ID = 0;
bool Pocc::runOnScop(Scop &S) {
  return false;
}

void Pocc::getAnalysisUsage(AnalysisUsage &AU) const {
  ScopPass::getAnalysisUsage(AU);
  AU.addRequired<ScopInfo>();
}

static RegisterPass<Pocc> A("polly-pocc",
                            "Polly - Optimize the scop using pocc");

Pass* polly::createPoccPass() {
  return new Pocc();
}
