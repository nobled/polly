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
#include "polly/LinkAllPasses.h"
#include "polly/Support/SCoPHelper.h"
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

    if (!S)
      return false;

    Function *F = S->getRegion().getEntry()->getParent();
    fflush(stdout);
    outs() << "\nIn function: '" << F->getNameStr() << "' SCoP: "
      << S->getRegion().getNameStr() << ":\n";

    CLooG C = CLooG(S);
    C.pprint();

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

char ScopPrinter::ID = 0;

static RegisterPass<ScopPrinter>
X("polly-print", "Polly - Print SCoP as C code to stdout");

Pass* polly::createScopPrinterPass() {
  return new ScopPrinter();
}
