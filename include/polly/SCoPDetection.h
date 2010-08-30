//===--- polly/SCoPDetection.h - Detect SCoPs in LLVM Function ---*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Detect SCoPs in LLVM Function.
//
//===----------------------------------------------------------------------===//

#ifndef POLLY_SCOP_DETECTION_H
#define POLLY_SCOP_DETECTION_H

#include "llvm/Pass.h"

#include <set>

using namespace llvm;

namespace llvm {
  class RegionInfo;
  class Region;
  class LoopInfo;
  class Loop;
  class PostDominatorTree;
  class DominatorTree;
  class ScalarEvolution;
  class SCEV;
  class SCEVAddRecExpr;
  class CallInst;
  class Instruction;
}

namespace polly {
typedef std::set<const SCEV*> ParamSetType;
typedef std::pair<const SCEV*, const SCEV*> AffCmptType;

class TempSCoP;
class SCoPDetection;
class SCEVAffFunc;

//===----------------------------------------------------------------------===//
/// @brief The Function Pass to detection Static control part in llvm function.
///
/// Please run "Canonicalize Induction Variables" pass(-indvars) before this
/// pass.
///
/// TODO: Provide interface to update the temporary SCoP information.
///
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

  // FIXME: This is only a temporary hack, we need a standalone condition
  // analysis and construction pass.
  // For simple condition extraction support
  DominatorTree *DT;
  PostDominatorTree *PDT;

  // Remember the valid regions
  typedef std::set<const Region*> RegionSet;
  RegionSet ValidRegions;

  // Clear the context.
  void clear();

  // Find the SCoPs in this region tree.
  void findSCoPs(Region &R);

  /////////////////////////////////////////////////////////////////////////////
  // Check if the max region of SCoP is valid, return true if it is valid
  // false otherwise.
  //
  // NOTE: All this function will increase the statistic counters.

  /// @brief Check if a region is a SCoP.
  ///
  /// @param R The region to check.
  /// @return True it R is a SCoP, false otherwise.
  bool isValidRegion(Region &R) const;

  // Check if the instruction is a valid function call.
  static bool isValidCallInst(CallInst &CI);

  // Check is a memory access is valid.
  bool isValidMemoryAccess(Instruction &Inst, Region &RefRegion) const;

  // Check if all parameters in Params valid in Region R.
  void mergeParams(Region &R, ParamSetType &Params,
                   ParamSetType &SubParams) const;

  bool hasScalarDependency(Instruction &Inst, Region &RefRegion) const;

  // Check if the Instruction is a valid part of SCoP, return true and extract
  // the corresponding information, return false otherwise.
  bool isValidInstruction(Instruction &I, Region &RefRegion) const;

  // Check if the BB is a valid part of SCoP, return true and extract the
  // corresponding information, return false otherwise.
  bool isValidBasicBlock(BasicBlock &BB, Region &RefRegion) const;

  /// @brief Check if the control flow in a basic block is valid.
  ///
  /// @param BB The BB to check the control flow.
  /// @param RefRegion The region in respect to which we check the control
  ///                        flow.
  /// @return True if the BB contains only valid control flow.
  ///
  bool isValidCFG(BasicBlock &BB, Region &RefRegion) const;

  /// @brief Check if the SCEV expression is a valid affine function
  ///
  /// @param S          The SCEV expression to be checked
  /// @param RefRegion  The reference scope to check SCEV, it help to find out
  ///                   induction variables and parameters
  /// @param CurBB      The BasicBlock that containing this expression, it help
  ///                   to find out induction variables and parameters, too 
  ///
  /// @return True if the SCEV expression is affine, false otherwise
  ///
  bool isValidAffineFunction(const SCEV *S, Region &RefRegion, BasicBlock *CurBB,
    bool isMemAcc) const;

  /// @brief Is a loop valid with respect to a given region.
  ///
  /// @param L The loop to check.
  /// @param RefRegion The region we analyse the loop in.
  ///
  /// @return True if the loop is valid in the region.
  bool isValidLoop(Loop *L, Region &RefRegion) const;

  /// This variable will point out the fail reason when we verifying SCoPs. 
  mutable bool verifying;

public:
  static char ID;
  explicit SCoPDetection() : FunctionPass(ID), verifying(false) {}
  ~SCoPDetection();

  /// @brief Is the region is the maximum region of a SCoP?
  ///
  /// @param R The Region to test if it is maximum.
  ///
  /// @return Return true if R is the maximum Region in a SCoP, false otherwise.
  bool isMaxRegionInSCoP(const Region &R) const {
    // The Region is valid only if it could be found in the set.
    return ValidRegions.count(&R);
  }

  /// @name Maximum Region In SCoPs Iterators
  ///
  /// These iterators iterator over all maximum region in SCoPs of this
  /// function.
  //@{
  typedef RegionSet::iterator iterator;
  typedef RegionSet::const_iterator const_iterator;

  iterator begin()  { return ValidRegions.begin(); }
  iterator end()    { return ValidRegions.end();   }

  const_iterator begin() const { return ValidRegions.begin(); }
  const_iterator end()   const { return ValidRegions.end();   }
  //@}

  /// @brief Remove a region and its children from valid region set.
  ///
  /// @param R The region to remove.
  void forgetSCoP(const Region &R) {
    assert(isMaxRegionInSCoP(R) && "R is not a SCoP!");
    ValidRegions.erase(&R);
  }

  /// @brief Verify if all valid Regions in this Function are still valid
  /// after some transformations.
  void verifyAnalysis() const;

  /// @brief Verify if R is still a valid part of SCoP after some
  /// transformations.
  ///
  /// @param R The Region to verify.
  void verifyRegion(const Region &R) const;

  /// @name FunctionPass interface
  //@{
  virtual void getAnalysisUsage(AnalysisUsage &AU) const;
  virtual void releaseMemory() { clear(); }
  virtual bool runOnFunction(Function &F);
  virtual void print(raw_ostream &OS, const Module *) const;
  //@}
};

} //end namespace polly

#endif
