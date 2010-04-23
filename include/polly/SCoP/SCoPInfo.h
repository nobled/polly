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
  //===-------------------------------------------------------------------===//
  /// Types
  typedef std::set<const SCEV*> ParamSetType;

  // { Variable, Coefficient }
  typedef std::map<const SCEV*, const SCEV*> AffFuncType;

  // TODO: We need a standalone file to translate llvm affine function to
  // isl object
  // { Lowerbound, Upperbound }
  typedef std::pair<AffFuncType, AffFuncType> AffBoundType;
  // Only map bounds to loops, we could construct the iterate domain of
  // a BB/Region by visit all its parent. We can also have some stack to
  // do that.
  typedef std::map<const Loop*, AffBoundType> BoundMapType;



  //===-------------------------------------------------------------------===//
  // SCoP represent with llvm objects.
  // A help class for finding SCoP,
  struct LLVMSCoP {
    // The Region.
    Region *R;
    ParamSetType Params;
    LLVMSCoP(Region *r) : R(r) {
      assert(R && "Region Cant be null!");
    }
    void print(raw_ostream &OS) const;
  };

  typedef std::vector<LLVMSCoP*> SCoPSetType;
  //===-------------------------------------------------------------------===//
  // DO NOT IMPLEMENT
  SCoPInfo(const SCoPInfo &);
  // DO NOT IMPLEMENT
  const SCoPInfo &operator=(const SCoPInfo &);

  // The ScalarEvolution to help building SCoP.
  ScalarEvolution* SE;

  LoopInfo *LI;

  RegionInfo *RI;

  BoundMapType LoopBounds;

  SCoPSetType SCoPs;

  // Temporary Hack for extended regiontree.
  // Cast the region to loop if there is a loop have the same header and exit.
  Loop *castToLoop(const Region *R) const {
    BasicBlock *entry = R->getEntry();

    if (!LI->isLoopHeader(entry))
      return 0;

    Loop *L = LI->getLoopFor(entry);

    BasicBlock *exit = L->getExitBlock();

    assert(exit && "Oop! we need the extended region tree :P");

    if (exit != R->getExit()) {
      assert((RI->getRegionFor(entry) != R ||
              R->getParent()->getEntry() == entry) &&
              "Expect the loop is the smaller or bigger region");
      return 0;
    }

    return L;
  }

  // Get the Loop containing all bbs of this region, for ScalarEvolution
  // "getSCEVAtScope"
  Loop *getScopeLoop(const Region *R) {
    while (R) {
      if (Loop *L = castToLoop(R))
        return L;

      R = R->getParent();
    }
    return 0;
  }

  bool buildAffineFunc(const SCEV *S, LLVMSCoP *SCoP, AffFuncType &FuncToBuild);

  void clear() {
    LoopBounds.clear();
    SCoPs.clear();
  }

  // If the Region not a valid part of a SCoP, return false, otherwise return true.
  LLVMSCoP *findSCoPs(Region* R, SCoPSetType &SCoPs);


  void mergeSubSCoPs(LLVMSCoP *MergeTo, SCoPSetType &SubSCoPs) {
    while (!SubSCoPs.empty()) {
      LLVMSCoP *SubSCoP = SubSCoPs.back();
      // merge the parameters.
      MergeTo->Params.insert(SubSCoP->Params.begin(),
                             SubSCoP->Params.end());
      // Discard the merged scop.
      SubSCoPs.pop_back();
      delete SubSCoP;
    }

    if (Loop *L = castToLoop(MergeTo->R))
      MergeTo->Params.erase(
        SE->getSCEV(L->getCanonicalInductionVariable()));
  }

  //
  void printBounds(raw_ostream &OS) const;

public:
  static char ID;
  explicit SCoPInfo() : FunctionPass(&ID) {}
  ~SCoPInfo();

  /// Pass interface
  virtual void getAnalysisUsage(AnalysisUsage &AU) const {
    AU.addRequired<LoopInfo>();
    // Make loop only have 1 back-edge?
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
