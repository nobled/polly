//===- SCoPPass.cpp - The base class of Passes that operate on Polly IR ---===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the definitions of the SCoPPass members.
//
//===----------------------------------------------------------------------===//

#include "polly/SCoPPass.h"
#include "polly/SCoPInfo.h"

using namespace llvm;
using namespace polly;

bool SCoPPass::runOnRegion(Region *R, RGPassManager &RGM) {
  if (SCoP *S = getAnalysis<SCoPInfo>().getSCoP())
    return runOnSCoP(*S);

  return false;
}

void SCoPPass::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<SCoPInfo>();
  AU.setPreservesAll();
}
