//===- CodeGen.cpp - Recreate LLVM-IR from the SCoP.  ---------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Recreate LLVM IR from the polyhedral description using CLooG.
//
//===----------------------------------------------------------------------===//

#include "polly/SCoP.h"

using namespace polly;

namespace {
class CodeGenPass: public RegionPass {

  Region *region;

public:
  static char ID;

  CodeGenPass() : RegionPass(&ID) {}

  bool runOnRegion(Region *R, RGPassManager &RGM) {
    region = R;
    return true;
  }

  virtual void getAnalysisUsage(AnalysisUsage &AU) const {
    // TODO: Cannot be removed, as otherwise LLVM crashes.
    AU.setPreservesAll();
    AU.addRequired<SCoP>();
  }
};
} //end anonymous namespace

char CodeGenPass::ID = 0;

static RegisterPass<CodeGenPass>
X("polly-codegen", "Polly - Code generation");


RegionPass* createCodeGenPass() {
  return new CodeGenPass();
}
