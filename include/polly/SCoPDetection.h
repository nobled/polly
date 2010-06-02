//===--- polly/SCoPDetection.h - Detect SCoPs in LLVM Function ---*- C++ -*-===//
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

#ifndef POLLY_SCOP_DETECTION_H
#define POLLY_SCOP_DETECTION_H

#include "polly/PollyType.h"
#include "polly/SCEVAffFunc.h"
#include "polly/ScalarDataRef.h"

#include "llvm/Analysis/RegionInfo.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/PointerIntPair.h"
#include "llvm/Transforms/Scalar.h"

using namespace llvm;

namespace polly {

class SCoPDetection;

//===----------------------------------------------------------------------===//
/// Temporary Hack for extended regiontree.
///
/// @brief Cast the region to loop.
///
/// @param R  The Region to be casted.
/// @param LI The LoopInfo to help the casting.
///
/// @return If there is a loop have the same entry and exit, return the loop,
///         otherwise, return null.
Loop *castToLoop(const Region &R, LoopInfo &LI);

/// @brief Get the Loop containing all bbs of this region,
///
/// This function is mainly use for get the loop for ScalarEvolution
/// "getSCEVAtScope"
///
/// @param R  The "Scope"
/// @param LI The LoopInfo to help the casting.
Loop *getScopeLoop(const Region &R, LoopInfo &LI);

//===---------------------------------------------------------------------===//
/// Types

/// { Lower bound, Upper bound } of a loop
typedef std::pair<SCEVAffFunc, SCEVAffFunc> AffBoundType;

/// Mapping loops to its bounds.
typedef std::map<const Loop*, AffBoundType> BoundMapType;

typedef std::vector<SCEVAffFunc> AccFuncSetType;
typedef std::map<const BasicBlock*, AccFuncSetType> AccFuncMapType;


//===---------------------------------------------------------------------===//
/// @brief SCoP represent with llvm objects.
///
/// A helper class for remembering the parameter number and the max depth  in
/// this SCoP, and others context.
///
class TempSCoP {
  // The Region.
  Region &R;

  // Parameters used in this SCoP.
  ParamSetType Params;

  // TODO: Constraints on parameters?

  // The max loop depth of this SCoP
  unsigned MaxLoopDepth;

  // Remember the bounds of loops, to help us build iterate domain of BBs.
  const BoundMapType &LoopBounds;

  // Access function of bbs.
  const AccFuncMapType &AccFuncMap;

  friend class SCoPDetection;

  explicit TempSCoP(Region &r, BoundMapType &loopBounds,
    AccFuncMapType &accFuncMap)
    : R(r), MaxLoopDepth(0),
    LoopBounds(loopBounds), AccFuncMap(accFuncMap) {}
public:

  /// @name Information about this Temporary SCoP.
  ///
  //@{
  /// @brief Get the parameters used in this SCoP.
  ///
  /// @return The parameters use in region.
  ParamSetType &getParamSet() { return Params; }

  /// @brief Get the maximum Region contained by this SCoP.
  ///
  /// @return The maximum Region contained by this SCoP.
  Region &getMaxRegion() const { return R; }

  /// @brief Get the maximum loop depth of Region R.
  ///
  /// @return The maximum loop depth of Region R.
  unsigned getMaxLoopDepth() const { return MaxLoopDepth; }

  /// @brief Get the loop bounds of the given loop.
  ///
  /// @param L The loop to get the bounds.
  ///
  // @return The bounds of the loop L in { Lower bound, Upper bound } form.
  const AffBoundType *getLoopBound(const Loop *L) const {
    BoundMapType::const_iterator at = LoopBounds.find(L);
    assert(at != LoopBounds.end() && "Only valid loop is allow!");
    return &(at->second);
  }

  const AccFuncSetType *getAccessFunctions(const BasicBlock* BB) const {
    AccFuncMapType::const_iterator at = AccFuncMap.find(BB);
    return at != AccFuncMap.end()? &(at->second) : 0;
  }
  //@}

  /// @brief Print the Temporary SCoP information.
  ///
  /// @param OS The output stream the access functions is printed to.
  /// @param SE The ScalarEvolution that help printing Temporary SCoP
  ///           information.
  /// @param LI The LoopInfo that help printing the access functions.
  void print(raw_ostream &OS, ScalarEvolution *SE, LoopInfo *LI) const;

  /// @brief Print the access functions and loop bounds in this SCoP.
  ///
  /// @param OS The output stream the access functions is printed to.
  /// @param SE The ScalarEvolution that help printing the access functions.
  /// @param LI The LoopInfo that help printing the access functions.
  void printDetail(raw_ostream &OS, ScalarEvolution *SE,
                   LoopInfo *LI, const Region *Reg, unsigned ind) const;

};

typedef std::map<const Region*, TempSCoP*> TempSCoPMapType;

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

  // Capture scalar data reference.
  ScalarDataRef *SDR;

  // Remember the bounds of loops, to help us build iterate domain of BBs.
  BoundMapType LoopBounds;

  // Access function of bbs.
  AccFuncMapType AccFuncMap;

  // SCoPs in the function
  TempSCoPMapType RegionToSCoPs;

  // Clear the context.
  void clear();

  // Run on region to:
  // 1. check if the region is a valid part of SCoP,
  // 2. remember valid regions
  // 3. kill all temporary instructions that can be rewrite
  //    in the codegen phase.
  void runOnRegion(Region &R);

  /////////////////////////////////////////////////////////////////////////////
  // We need to check if valid parameters from child SCoP also valid in
  // parent SCoP. So all the isValidXXX functions will extract parameters
  // to Params.
  //
  // Check if the max region of SCoP is valid, return true if it is valid
  // false otherwise.
  //
  // NOTE: All this function will increase the statistic counters.
  bool isValidRegion(Region &R, ParamSetType &Params) const;

  bool isValidRegion(Region &R) const;

  // Check if the instruction is a valid function call.
  static bool isValidCallInst(CallInst &CI);

  // Check is a memory access is valid.
  bool isValidMemoryAccess(Instruction &Inst, Region &R,
                           ParamSetType &Params) const;

  // Check if all parameters in Params valid in Region R.
  bool tryMergeParams(Region &R, ParamSetType &Params,
                      ParamSetType &SubParams) const;

  // Check if the Instruction is a valid part of SCoP, return true and extract
  // the corresponding information, return false otherwise.
  bool isValidInstruction(Instruction &I, Region &R,
                          ParamSetType &Params) const;

  // Check if the BB is a valid part of SCoP, return true and extract the
  // corresponding information, return false otherwise.
  bool isValidBasicBlock(BasicBlock &BB, Region &R,
                         ParamSetType &Params) const;

  // Check if the CFG is valid for SCoP.
  bool isValidCFG(BasicBlock &BB, Region &R) const;

  // Check if the loop bounds in SCoP is valid.
  bool hasValidLoopBounds(Region &R, ParamSetType &Params) const;

  /////////////////////////////////////////////////////////////////////////////
  // If the Region not a valid part of a SCoP,
  // return null if Region R is a valid part of a SCoP,
  // otherwise return the temporary SCoP information of Region R.
  TempSCoP *getTempSCoP(Region &R);

  TempSCoP *buildTempSCoP(Region &R);

  // Extract the access functions from a BasicBlock to ScalarAccs
  void extractAccessFunctions(TempSCoP &SCoP, BasicBlock &BB,
                              AccFuncSetType &AccessFunctions);

  void extractLoopBounds(TempSCoP &SCoP);

  // Capture scalar data reference. Fill the scalar "memory access" to the
  // access function map.
  void captureScalarDataRef(Instruction &I, AccFuncSetType &ScalarAccs);

  // Kill all temporary value that can be rewrite by SCEV Expander.
  void killAllTempValFor(const Region &R);

  void killAllTempValFor(Loop &L);

  void killAllTempValFor(BasicBlock &BB);

public:
  static char ID;
  explicit SCoPDetection() : FunctionPass(&ID) {}
  ~SCoPDetection();

  /// @brief Get the temporay SCoP information in LLVM IR represent
  ///        for Region R.
  ///
  /// @param R            The Region to extract SCoP information.
  /// @param subSCoPAllow Should this function return information about
  ///                     the subSCoPs?
  ///
  /// @return The SCoP information in LLVM IR represent.
  TempSCoP *getTempSCoPFor(const Region* R) const;

  /// @brief Is the region is the maximum region of a SCoP?
  ///
  /// @param R The Region to test if it is maximum.
  ///
  /// @return Return true if R is the maximum Region in a SCoP, false otherwise.
  bool isMaxRegionInSCoP(const Region &R) const {
    assert(RegionToSCoPs.count(&R) && "R must be a valid SCoP!");
    return (R.getParent() == 0) || !RegionToSCoPs.count(R.getParent());
  }

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