//===-------- polly/TempSCoPExtraction.h - Extract TempSCoPs -----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Detect SCoPs in LLVM Function;
//
//===----------------------------------------------------------------------===//

#ifndef POLLY_TEMP_SCOP_EXTRACTION_H
#define POLLY_TEMP_SCOP_EXTRACTION_H

#include "polly/PollyType.h"
#include "polly/SCoPDetection.h"
#include "polly/Support/AffineSCEVIterator.h"

#include "llvm/Analysis/RegionInfo.h"
#include "llvm/Analysis/RegionPass.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/PointerIntPair.h"
#include "llvm/Transforms/Scalar.h"

using namespace llvm;

namespace polly {

class IndependentBlocks : public RegionPass {
  Region *region;
  ScalarEvolution *SE;
  LoopInfo *LI;
  SCoPDetection *SD;

  public:
  static char ID;

  IndependentBlocks() : RegionPass(&ID) {}

  // Create new code for every instruction operator that can be expressed by a
  // SCEV.  Like this there are just two types of instructions left:
  //
  // 1. Instructions that only reference loop ivs or parameters outside the
  // region.
  //
  // 2. Instructions that are not used for any memory modification. (These
  //    will be ignored later on.)
  //
  // Blocks containing only these kind of instructions are called independent
  // blocks as they can be scheduled arbitrarily.
  void createIndependentBlocks(BasicBlock *BB);
  void createIndependentBlocks(Region *R);
  bool isIV(Instruction *I);
  bool isIndependentBlock(const Region *R, BasicBlock *BB);
  bool areAllBlocksIndependent(Region *R);
  bool runOnRegion(Region *R, RGPassManager &RGM);
  virtual void getAnalysisUsage(AnalysisUsage &AU) const;
};
} // end namespace polly


#endif
