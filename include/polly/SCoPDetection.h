//===--- polly/SCoPDetection.h - Detect SCoPs -------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Pass to detect the maximal static control parts (SCoPs) of a function.
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
  class ScalarEvolution;
  class SCEV;
  class SCEVAddRecExpr;
  class CallInst;
  class Instruction;
}

namespace polly {
typedef std::set<const SCEV*> ParamSetType;

//===----------------------------------------------------------------------===//
/// @brief Pass to detect the maximal static control parts (SCoPs) of a
/// function.
class SCoPDetection : public FunctionPass {
  //===--------------------------------------------------------------------===//
  // DO NOT IMPLEMENT
  SCoPDetection(const SCoPDetection &);
  // DO NOT IMPLEMENT
  const SCoPDetection &operator=(const SCoPDetection &);

  /// @brief Analysis passes used.
  //@{
  ScalarEvolution* SE;
  LoopInfo *LI;
  RegionInfo *RI;
  //@}

  // Remember the valid regions
  typedef std::set<const Region*> RegionSet;
  RegionSet ValidRegions;

  // Find the SCoPs in this region tree.
  void findSCoPs(Region &R);

  /// @brief Check if all basic block in the region are valid.
  ///
  /// @param R The region to check.
  /// @return True if all blocks in R are valid, false otherwise.
  bool allBlocksValid(Region &R) const;

  /// @brief Check the exit block of a region is valid.
  ///
  /// @param R The region to check.
  /// @return True if the exit of R is valid, false otherwise.
  bool isValidExit(Region &R) const;

  /// @brief Check if a region is a SCoP.
  ///
  /// @param R The region to check.
  /// @return True if R is a SCoP, false otherwise.
  bool isValidRegion(Region &R) const;

  /// @brief Check if a call instruction can be part of a SCoP.
  static bool isValidCallInst(CallInst &CI);

  /// @brief Check if a memory access can be part of a SCoP.
  bool isValidMemoryAccess(Instruction &Inst, Region &RefRegion) const;

  /// @brief Check if an instruction has any non trivial scalar dependencies
  ///        as part of a SCoP.
  bool hasScalarDependency(Instruction &Inst, Region &RefRegion) const;

  /// @brief Check if an instruction can be part of a SCoP.
  bool isValidInstruction(Instruction &I, Region &RefRegion) const;

  /// @brief Check if the BB can be part of a SCoP.
  bool isValidBasicBlock(BasicBlock &BB, Region &RefRegion) const;

  /// @brief Check if the control flow in a basic block is valid.
  ///
  /// @param BB The BB to check the control flow.
  /// @param RefRegion The region in respect to which we check the control
  ///                  flow.
  ///
  /// @return True if the BB contains only valid control flow.
  bool isValidCFG(BasicBlock &BB, Region &RefRegion) const;

  /// @brief Check if the SCEV expression is a valid affine function
  ///
  /// @param S          The SCEV expression to be checked
  /// @param RefRegion  The reference scope to check SCEV, it help to find out
  ///                   induction variables and parameters
  /// @param isMemoryAccess Does S represent a memory access. In this case one
  ///                       base pointer is allowed.
  ///
  /// @return True if the SCEV expression is affine, false otherwise
  ///
  bool isValidAffineFunction(const SCEV *S, Region &RefRegion,
                             bool isMemoryAccess = false) const;

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
  virtual void releaseMemory();
  virtual bool runOnFunction(Function &F);
  virtual void print(raw_ostream &OS, const Module *) const;
  //@}
};

} //end namespace polly

#endif
