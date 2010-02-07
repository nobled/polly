//===- SCoP.cpp - Create a SCoP -------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Create a polyhedral description of the region.
//
//===----------------------------------------------------------------------===//

#include "polly/SCoP.h"

using namespace llvm;
using namespace polly;

namespace polly {
SCoP::SCoP() : RegionPass(&ID) {}

bool SCoP::runOnRegion(Region *R, RGPassManager &RGM) {
    return false;
}

void SCoP::getAnalysisUsage(AnalysisUsage &AU) const {
    AU.setPreservesAll();
}
} //end polly namespace

char polly::SCoP::ID = 0;

static RegisterPass<SCoP>
X("polly-scop", "Polly - Create polyhedral SCoPs");

RegionPass* createSCoPPass() {
  return new SCoP();
}
