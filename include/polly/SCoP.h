//===- SCoP.h - Create a SCoP. ----------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Create a polyhedral description for a static control flow region.
//
//===----------------------------------------------------------------------===//
//
#ifndef POLLY_SCOP_H
#define POLLY_SCOP_H

#include "llvm/Analysis/RegionPass.h"
#include "llvm/Analysis/Passes.h"

using namespace llvm;

namespace polly {
  class SCoP: public RegionPass {
  public:
    static char ID;

    SCoP();

    bool runOnRegion(Region *R, RGPassManager &RGM);

    virtual void getAnalysisUsage(AnalysisUsage &AU) const;
  };
}

#endif /* POLLY_SCOP_H */

