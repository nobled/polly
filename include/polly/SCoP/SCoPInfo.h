//===- polly/SCoP/SCoPInfo.h - Create SCoPs from LLVM IR --------*- C++ -*-===//
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
#ifndef POLLY_SCOP_INFO_H
#define POLLY_SCOP_INFO_H

#include "llvm/Analysis/RegionPass.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Scalar.h"

#include <set>
#include <map>

using namespace llvm;

namespace polly {

class SCoPInfo : public FunctionPass {
  // DO NOT IMPLEMENT
  SCoPInfo(const SCoPInfo &);
  // DO NOT IMPLEMENT
  const SCoPInfo &operator=(const SCoPInfo &);

  // The ScalarEvolution to help building SCoP.
  ScalarEvolution* SE;

  std::set<const Loop*> BadLoops;

  typedef std::set<const SCEV*> ParamSet;
  typedef std::map<const Loop*, ParamSet > ParamMap;
  // We will operate on SCEV instead of value.
  ParamMap ParamsAtScope;

  void clear() {
    BadLoops.clear();
    ParamsAtScope.clear();
  }

  void AddBadLoop(const Loop* L) {
      BadLoops.insert(L);
      ParamsAtScope.erase(L);
  }

  // Exract the parameter from a SCEV expression.
  // Return false if the SCEV is not a affine expression.
  bool extractParam(const SCEV *S, const Loop *Scope);

  // If the loop not a valid part of a SCoP, return false, otherwise return true.
  //
  // Then we can find the SCoP by find the biggest regions do not contain any
  // bad Loop.
  bool visitLoop(const Loop* L);

  // Visit all loops in function
  void visitLoopsInFunction(Function &F, LoopInfo &LI);

  //
  void printParams(raw_ostream &OS) const;
public:
  static char ID;
  explicit SCoPInfo() : FunctionPass(&ID) {}
  ~SCoPInfo();

  /// Pass interface
  virtual void getAnalysisUsage(AnalysisUsage &AU) const {
    AU.addRequired<LoopInfo>();
    //AU.addPreservedID(LoopSimplifyID);
    AU.addRequired<RegionInfo>();
    AU.addRequired<ScalarEvolution>();
    AU.setPreservesAll();
  }

  virtual void releaseMemory() {
    clear();
  }

  virtual bool runOnFunction(Function &F);

  virtual void print(raw_ostream &OS, const Module *) const;
};

/// Function for force linking.
FunctionPass *createSCoPInfoPass();

} //end namespace polly

#endif
