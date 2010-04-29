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

#include "polly/PollyType.h"

#include "llvm/Analysis/RegionInfo.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Scalar.h"

using namespace llvm;

namespace polly {

class LLVMSCoP;
class SCoPDetection;

//===----------------------------------------------------------------------===//
//Temporary Hack for extended regiontree.
//
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
/// @brief Affine function represent in llvm SCEV expressions.
///
/// A helper class for collect affine function information
class SCEVAffFunc {
  // The translation component
  const SCEV *TransComp;

  // { Variable, Coefficient }
  typedef std::map<const SCEV*, const SCEV*> LnrTransSet;
  LnrTransSet LnrTrans;

  // The base address of the address SCEV.
  const Value *BaseAddr;

  // getCoeff - Get the Coefficient of a given variable.
  const SCEV *getCoeff(const SCEV *Var) {
    LnrTransSet::iterator At = LnrTrans.find(Var);
    return At == LnrTrans.end() ? 0 : At->second;
  }

public:
  /// @brief Create a new SCEV affine function.
  explicit SCEVAffFunc() : TransComp(0), BaseAddr(0) {}

  /// @brief Build an affine function from a SCEV expression.
  ///
  /// @param S            The SCEV expression to be converted to affine
  ///                     function.
  /// @param SCoP         The Scope of this expression.
  /// @param FuncToBuild  The SCEVAffFunc to hold the result.
  /// @param LI           The LoopInfo to help to build the affine function.
  /// @param SE           The ScalarEvolution to help to build the affine
  ///                     function.
  ///
  /// @return             Return true if S could be convert to affine function,
  ///                     false otherwise.
  static bool buildAffineFunc(const SCEV *S, LLVMSCoP &SCoP,
    SCEVAffFunc &FuncToBuild, LoopInfo &LI, ScalarEvolution &SE);

  /// @brief Build a loop bound constrain from an affine function.
  ///
  /// @param ctx      The context of isl objects.
  /// @param dim      The dimension of the the constrain.
  /// @param IndVars  The induction variable may appear in the affine function.
  /// @param Params   The parameters may appear in the affine funcion.
  /// @param isLower  Is this the lower bound?
  ///
  /// @return         The isl_constrain represent by this affine function.
  polly_constraint *toLoopBoundConstrain(polly_ctx *ctx, polly_dim *dim,
                                  const SmallVectorImpl<const SCEV*> &IndVars,
                                  const SmallVectorImpl<const SCEV*> &Params,
                                  bool isLower);

  /// @brief Print the affine function.
  ///
  /// @param OS The output stream the affine function is printed to.
  /// @param SE The ScalarEvolution that help printing the affine function.
  void print(raw_ostream &OS, ScalarEvolution *SE) const;
};

//===---------------------------------------------------------------------===//
/// Types

/// { Lower bound, Upper bound } of a loop
typedef std::pair<SCEVAffFunc, SCEVAffFunc> AffBoundType;

/// Mapping loops to its bounds.
typedef std::map<const Loop*, AffBoundType> BoundMapType;

// The access function of basic block (or region)
// { address, isStore }
typedef std::pair<SCEVAffFunc, bool> AccessFuncType;
typedef std::vector<AccessFuncType> AccFuncSetType;
typedef std::map<const BasicBlock*, AccFuncSetType> AccFuncMapType;

typedef std::set<const SCEV*> ParamSetType;

//===---------------------------------------------------------------------===//
/// @brief SCoP represent with llvm objects.
///
/// A helper class for remembering the parameter number and the max depth  in
/// this SCoP, and others context.
///
class LLVMSCoP {
  typedef std::vector<LLVMSCoP*> TempSCoPSetType;

  // The Region.
  Region &R;

  // Parameters used in this SCoP.
  ParamSetType Params;

  // TODO: Constraints on parameters?
  // Remember the bounds of loops, to help us build iterate domain of BBs.
  BoundMapType LoopBounds;

  // Access function of bbs.
  AccFuncMapType AccFuncMap;

  // The max loop depth of this SCoP
  unsigned MaxLoopDepth;

  // Merge the SCoP information of sub regions
  void mergeSubSCoPs(TempSCoPSetType &SubSCoPs,
    LoopInfo &LI, ScalarEvolution &SE);

  // TODO: Remove SCoPDetection from friend class list.
  friend class SCoPDetection;
public:
  explicit LLVMSCoP(Region &r) : R(r), MaxLoopDepth(0) {}

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
  AffBoundType *getLoopBound(const Loop *L) {
    BoundMapType::iterator at = LoopBounds.find(L);
    return at != LoopBounds.end()? &(at->second) : 0;
  }
  //@}

  /// @brief Print the Temporary SCoP information.
  ///
  /// @param OS The output stream the access functions is printed to.
  /// @param SE The ScalarEvolution that help printing Temporary SCoP
  ///           information.
  void print(raw_ostream &OS, ScalarEvolution *SE) const;

  /// @brief Print the loop bounds in this SCoP.
  ///
  /// @param OS The output stream the access functions is printed to.
  /// @param SE The ScalarEvolution that help printing the loop bounds.
  void printBounds(raw_ostream &OS, ScalarEvolution *SE) const;

  /// @brief Print the access functions in this SCoP.
  ///
  /// @param OS The output stream the access functions is printed to.
  /// @param SE The ScalarEvolution that help printing the access functions.
  void printAccFunc(raw_ostream &OS, ScalarEvolution *SE) const;

};

typedef std::map<const Region*, LLVMSCoP*> TempSCoPMapType;
typedef std::vector<LLVMSCoP*> TempSCoPSetType;

//===----------------------------------------------------------------------===//
/// @brief The Function Pass to detection Static control part in llvm function.
///
/// Please run "Canonicalize Induction Variables" pass(-indvars) before this
/// pass.
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

  // Clear the context.
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

  /// @brief Get the temporay SCoP information in LLVM IR represent
  ///        for Region R.
  ///
  /// @param R The Region to extract SCoP information.
  ///
  /// @return The SCoP information in LLVM IR represent.
  LLVMSCoP *getTempSCoPFor(const Region* R) const {
    // FIXME: Get the temporay scop info for the sub regions of scops.
    // Or we could create these information on the fly!
    TempSCoPMapType::const_iterator at = RegionToSCoPs.find(R);
    return at == RegionToSCoPs.end()?0:at->second;
  }

  /// @name FunctionPass interface
  //@{
  virtual void getAnalysisUsage(AnalysisUsage &AU) const {
    AU.addRequired<LoopInfo>();
    AU.addRequired<RegionInfo>();
    AU.addRequired<ScalarEvolution>();
    AU.setPreservesAll();
  }
  virtual void releaseMemory() { clear(); }
  virtual bool runOnFunction(Function &F);
  virtual void print(raw_ostream &OS, const Module *) const;
  //@}
};

} //end namespace polly

#endif
