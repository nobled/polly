//===---- SCoPDetection.h - Detect SCoPs in LLVM Function --------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Detect SCoPs in LLVM Function and extract loop bounds, access functions and
// conditions while checking.
//
//===----------------------------------------------------------------------===//

#ifndef SCOP_DETECTION_H
#define SCOP_DETECTION_H

#include "llvm/Analysis/RegionInfo.h"

#include "llvm/Analysis/Passes.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Scalar.h"

using namespace llvm;

namespace polly {

struct LLVMSCoP;

//===----------------------------------------------------------------------===//
// Temporary Hack for extended regiontree.
// Cast the region to loop if there is a loop have the same header and exit.
Loop *castToLoop(const Region &R, LoopInfo &LI);
// Get the Loop containing all bbs of this region, for ScalarEvolution
// "getSCEVAtScope"
Loop *getScopeLoop(const Region &R, LoopInfo &LI);

//===---------------------------------------------------------------------===//
// Affine function represent with llvm objects.
// A helper class for collect affine function information
struct SCEVAffFunc {
  // The translation component
  const SCEV *TransComp;
  // { Variable, Coefficient }
  typedef std::map<const SCEV*, const SCEV*> LnrTransSet;
  LnrTransSet LnrTrans;
  // The base address of the address SCEV.
  const Value *BaseAddr;

  explicit SCEVAffFunc() : TransComp(0), BaseAddr(0) {}

  // getCoeff - Get the Coefficient of a given variable.
  const SCEV *getCoeff(const SCEV *Var) {
    LnrTransSet::iterator At = LnrTrans.find(Var);
    return At == LnrTrans.end() ? 0 : At->second;
  }

  static bool buildAffineFunc(const SCEV *S, LLVMSCoP &SCoP,
    SCEVAffFunc &FuncToBuild,
    LoopInfo &LI, ScalarEvolution &SE);

  //
  void print(raw_ostream &OS, ScalarEvolution *SE) const;
};

//===---------------------------------------------------------------------===//
/// Types
typedef std::set<const SCEV*> ParamSetType;
// TODO: We need a standalone file to translate llvm affine function to
// isl object
// { Lower bound, Upper bound }
typedef std::pair<SCEVAffFunc, SCEVAffFunc> AffBoundType;
// Only map bounds to loops, we could construct the iterate domain of
// a BB/Region by visit all its parent. We can also have some stack to
// do that.
typedef std::map<const Loop*, AffBoundType> BoundMapType;

// The access function of basic block (or region)
// { address, isStore }
typedef std::pair<SCEVAffFunc, bool> AccessFuncType;
typedef std::vector<AccessFuncType> AccFuncSetType;
typedef std::map<const BasicBlock*, AccFuncSetType> AccFuncMapType;

//===---------------------------------------------------------------------===//
// SCoP represent with llvm objects.
// A helper class for finding SCoP,

typedef std::vector<LLVMSCoP*> TempSCoPSetType;
typedef std::map<const Region*, LLVMSCoP*> TempSCoPMapType;

struct LLVMSCoP {
  // The Region.
  Region &R;
  ParamSetType Params;
  // TODO: Constraints on parameters?
  // Remember the bounds of loops, to help us build iterate domain of BBs.
  BoundMapType LoopBounds;
  // Access function of bbs.
  AccFuncMapType AccFuncMap;
  unsigned MaxLoopDepth;

  explicit LLVMSCoP(Region &r) : R(r), MaxLoopDepth(0) {}

  // Merge the SCoP information of sub regions into MergeTo.
  void mergeSubSCoPs(TempSCoPSetType &SubSCoPs,
                     LoopInfo &LI, ScalarEvolution &SE);

  AffBoundType *getLoopBound(const Loop *L) {
    BoundMapType::iterator at = LoopBounds.find(L);
    return at != LoopBounds.end()? &(at->second) : 0;
  }

  void print(raw_ostream &OS, ScalarEvolution *SE) const;
  //
  void printBounds(raw_ostream &OS, ScalarEvolution *SE) const;
  void printAccFunc(raw_ostream &OS, ScalarEvolution *SE) const;

};

typedef std::vector<LLVMSCoP*> TempSCoPSetType;

class SCoPDetection : public FunctionPass {
  //===-------------------------------------------------------------------===//
  // DO NOT IMPLEMENT
  SCoPDetection(const SCoPDetection &);
  // DO NOT IMPLEMENT
  const SCoPDetection &operator=(const SCoPDetection &);

  // The ScalarEvolution to help building SCoP.
  ScalarEvolution* SE;
  // LoopInfo for information about loops
  LoopInfo *LI;
  // RegionInfo for regiontrees
  RegionInfo *RI;
  // SCoPs in the function
  TempSCoPMapType RegionToSCoPs;

  void clear();

  // If the Region not a valid part of a SCoP,
  // return false, otherwise return true.
  LLVMSCoP *findSCoPs(Region &R, TempSCoPSetType &SCoPs);

  // Check if the BB is a valid part of SCoP, return true and extract the
  // corresponding information, return false otherwise.
  bool checkBasicBlock(BasicBlock &BB, LLVMSCoP &SCoP);

  bool checkCFG(BasicBlock &BB, Region &R);

public:
  static char ID;
  explicit SCoPDetection() : FunctionPass(&ID) {}
  ~SCoPDetection();

  LLVMSCoP *getTempSCoPFor(const Region* R) const {
    TempSCoPMapType::const_iterator at = RegionToSCoPs.find(R);
    return at == RegionToSCoPs.end()?0:at->second;
  }

  /// Pass interface
  virtual void getAnalysisUsage(AnalysisUsage &AU) const {
    AU.addRequired<LoopInfo>();
    // Grr! These are looppasses!
    //AU.addRequiredID(IndVarSimplifyID);
    // Make loop only have 1 back-edge?
    //AU.addRequiredID(LoopSimplifyID);
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

} //end namespace polly

#endif
