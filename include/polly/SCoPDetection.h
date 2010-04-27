//===- SCoPDetection.h - Detect maximal SCoP regions ------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//
// Detect the maximal regions that are valid SCoPs.
//
//===----------------------------------------------------------------------===//

#ifndef POLLY_SCOP_DETECTION_H
#define POLLY_SCOP_DETECTION_H

#include "llvm/Pass.h"
#include "llvm/Support/CFG.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Analysis/RegionInfo.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/ScalarEvolution.h"

#include <vector>

using namespace llvm;

namespace polly {

//===----------------------------------------------------------------------===//
/// @brief Analysis that detects all canonical Regions.
///
/// The RegionInfo pass detects all canonical regions in a function. The Regions
/// are connected using the parent relation. This builds a Program Structure
/// Tree.
class SCoPDetection : public FunctionPass {
  RegionInfo *RI;
  LoopInfo *LI;
  ScalarEvolution *SE;
  typedef std::vector<Region*> RegionVector;
  typedef std::set<Region*> RegionSet;
  RegionSet SCoPRegions;

  bool isValid(Region *R, Instruction *I);
  bool isValid(Region *R, BasicBlock *BB);
  bool isValid(Region *R);
  bool isValidLoop(Loop *L);
  bool hasValidLoops(Region *R);
  void detectMaxSCoPs();
public:
  static char ID;
  SCoPDetection();

  /// @name FunctionPass interface
  //@{
  virtual bool runOnFunction(Function &F);
  virtual void getAnalysisUsage(AnalysisUsage &AU) const;
  virtual void print(raw_ostream &OS, const Module *) const;
  virtual void verifyAnalysis() const;
  //@}

  /// Check if R is a maximal valid SCOP.
  ///
  /// @param The Region to check.
  bool isMaxSCoPRegion(const Region *R) const;
  bool isMaxValid(Region *R);
};

FunctionPass* createSCoPDetectionPass();
} // End polly namespace
#endif
