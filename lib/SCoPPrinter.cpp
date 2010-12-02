//===- SCoPPrinter.cpp - Print the CLAST of the SCoPs to stdout. ----------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Dump for each SCoP the clast to stdout.
//
// For each SCoP CLooG is called to generate a clast based on the current
// SCoPinfo. The textual representation of this clast is dumped to stdout.
// By calling -polly-import before -polly-print the SCoP can be modified and the
// updated clast will be printed.
//
//===----------------------------------------------------------------------===//

#include "polly/CLooG.h"
#include "polly/LinkAllPasses.h"
#include "polly/SCoPInfo.h"
#include "polly/SCoPPass.h"

using namespace polly;
using namespace llvm;

namespace {
  class ScopPrinter : public SCoPPass {
  public:
    static char ID;
    CLooG *C;

    ScopPrinter() : SCoPPass(ID), C(0) {}

    bool runOnSCoP(SCoP &S) {
      if (C)
        delete C;

      C = new CLooG(&S);

      return false;
    }

    void printSCoP(raw_ostream &OS) const {
      C->pprint(OS);
    }

    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
      // Get the Common analysis usage of SCoPPasses.
      SCoPPass::getAnalysisUsage(AU);
    }
  };
} //end anonymous namespace

char ScopPrinter::ID = 0;

static RegisterPass<ScopPrinter>
X("polly-print", "Polly - Print SCoP as C code to stdout");

Pass* polly::createSCoPPrinterPass() {
  return new ScopPrinter();
}
