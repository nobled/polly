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

  Region *region;
  SCoP *S;

public:
  static char ID;

  ScopPrinter() : RegionPass(&ID) {}

  bool runOnRegion(Region *R, RGPassManager &RGM) {
    region = R;
    S = getAnalysis<SCoPInfo>().getSCoPFor(R);

    return false;
  }

  void print(raw_ostream &OS, const Module *) const {

    if (!S) {
      OS << "Invalid SCoP\n";
      return;
    }

    CLooG C = CLooG(S);
    struct clast_stmt *clast = C.getClast();
    OS << "Generated CLAST '" << clast << "'\n";
    C.pprint();
  }

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

  Region *region;
  SCoP *S;

public:
  static char ID;

  ScopCodeGen() : RegionPass(&ID) {}

  void insertNewCodeBranch() {
  }

  bool runOnRegion(Region *R, RGPassManager &RGM) {
    region = R;
    S = getAnalysis<SCoPInfo>().getSCoPFor(R);

    if (!S)
      return false;

    createSingleEntryEdge(R);
    createSingleExitEdge(R, this);
    insertNewCodeBranch();

    return true;
  }

  void print(raw_ostream &OS, const Module *) const {
  }

  virtual void getAnalysisUsage(AnalysisUsage &AU) const {
    // XXX: Cannot be removed, as otherwise LLVM crashes.
    AU.addRequired<SCoPInfo>();
  }
};
} //end anonymous namespace

char ScopCodeGen::ID = 0;

static RegisterPass<ScopCodeGen>
Y("polly-codegen-scop", "Polly - Code generation");

Pass* polly::createScopCodeGenPass() {
  return new ScopCodeGen();
}
