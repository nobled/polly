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

using namespace polly;
using namespace llvm;

namespace {
  class ScopPrinter : public RegionPass {

    SCoP *S;

  public:
    static char ID;

    ScopPrinter() : RegionPass(ID), S(0) {}

    bool runOnRegion(Region *R, RGPassManager &RGM) {
      S = getAnalysis<SCoPInfo>().getSCoP();

      if (!S)
        return false;

      Function *F = S->getRegion().getEntry()->getParent();
      fflush(stdout);

      std::string output;
      raw_string_ostream OS(output);
      OS << "In function: '" << F->getNameStr() << "' SCoP: "
        << S->getRegion().getNameStr() << ":\n";
      fprintf(stdout, "%s", OS.str().c_str());

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
