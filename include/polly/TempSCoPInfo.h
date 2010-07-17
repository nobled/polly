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
//===---------------------------------------------------------------------===//
/// @brief Affine function represent in llvm SCEV expressions.
///
/// A helper class for collect affine function information
class SCEVAffFunc {
  // Temporary hack
  friend class TempSCoPInfo;
  // The translation component
  const SCEV *TransComp;

  // { Variable, Coefficient }
  typedef std::map<const SCEV*, const SCEV*> LnrTransSet;
  LnrTransSet LnrTrans;

public:
  // The type of the scev affine function
  enum SCEVAffFuncType {
    None = 0,
    ReadMem, // Or we could call it "Use"
    WriteMem, // Or define
    Eq,       // == 0
    Ne,       // != 0
    GE        // >= 0
  };
  // Pair of {address, read/write}
  typedef std::pair<Value*, SCEVAffFuncType> MemAccTy;

private:
  // The base address of the address SCEV, if the Value is a pointer, this is
  // an array access, otherwise, this is a value access.
  // And the Write/Read modifier
  Value *BaseAddr;
  SCEVAffFuncType FuncType;

  // getCoeff - Get the Coefficient of a given variable.
  const SCEV *getCoeff(const SCEV *Var) const {
    LnrTransSet::const_iterator At = LnrTrans.find(Var);
    return At == LnrTrans.end() ? 0 : At->second;
  }

public:
  /// @brief Create a new SCEV affine function.
  explicit SCEVAffFunc() : TransComp(0), BaseAddr(0),
    FuncType(None) {}

  /// @brief Create a new SCEV affine function with memory access type or
  ///        condition type

  explicit SCEVAffFunc(SCEVAffFuncType Type, Value* baseAddr = 0)
    : TransComp(0), BaseAddr(baseAddr), FuncType(Type) {}

  SCEVAffFunc(const SCEV *S, SCEVAffFuncType Type, ScalarEvolution *SE);

  /// @brief Build a constraint from an affine condition
  ///       (affine function + inequality modifier ).
  ///
  /// @param ctx      The context of isl objects.
  /// @param dim      The dimension of the constraint.
  /// @param IndVars  The induction variable may appear in the affine function.
  /// @param Params   The parameters may appear in the affine function.
  ///
  /// @return         The isl_constrain represented by this affine function.
  polly_constraint *toConditionConstrain(polly_ctx *ctx, polly_dim *dim,
    const SmallVectorImpl<const SCEVAddRecExpr*> &IndVars,
    const SmallVectorImpl<const SCEV*> &Params) const;

  /// @brief Build a constraint from an affine condition
  ///       (affine function + inequality modifier ).
  ///
  /// @param ctx      The context of isl objects.
  /// @param dim      The dimension of the isl set.
  /// @param IndVars  The induction variable may appear in the affine function.
  /// @param Params   The parameters may appear in the affine function.
  ///
  /// @return         The isl_set represented by this affine function.
  polly_set *toConditionSet(polly_ctx *ctx, polly_dim *dim,
    const SmallVectorImpl<const SCEVAddRecExpr*> &IndVars,
    const SmallVectorImpl<const SCEV*> &Params) const;

  polly_constraint *toAccessFunction(polly_dim* dim,
    const SmallVectorImpl<Loop*> &NestLoops,
    const SmallVectorImpl<const SCEV*> &Params,
    ScalarEvolution &SE) const;

  bool isDataRef() const {
    return FuncType == ReadMem || FuncType == WriteMem;
  }

  enum SCEVAffFuncType getType() const { return FuncType; }

  bool isRead() const { return FuncType == ReadMem; }

  const Value *getBaseAddr() const { return BaseAddr; }

  /// @brief Print the affine function.
  ///
  /// @param OS The output stream the affine function is printed to.
  void print(raw_ostream &OS) const;
  void dump() const;
};

//===---------------------------------------------------------------------===//
/// Types
// The condition of a Basicblock, combine brcond with "And" operator.
typedef SmallVector<SCEVAffFunc, 4> BBCond;

/// Maps from a loop to the affine function expressing its backedge taken count.
/// The backedge taken count already enough to express iteration domain as we
/// only allow loops with canonical induction variable.
/// A canonical induction variable is:
/// an integer recurrence that starts at 0 and increments by one each time
/// through the loop.
typedef std::map<const Loop*, SCEVAffFunc> LoopBoundMapType;

/// Mapping BBs to its condition constrains
typedef std::map<const BasicBlock*, BBCond> BBCondMapType;

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

  // Remember the bounds of loops, to help us build iteration domain of BBs.
  const LoopBoundMapType &LoopBounds;
  const BBCondMapType &BBConds;

  // Access function of bbs.
  const AccFuncMapType &AccFuncMap;

  friend class TempSCoPInfo;

  explicit TempSCoP(Region &r, LoopBoundMapType &loopBounds,
    BBCondMapType &BBCnds, AccFuncMapType &accFuncMap)
    : R(r), MaxLoopDepth(0),
    LoopBounds(loopBounds), BBConds(BBCnds), AccFuncMap(accFuncMap) {}
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
  /// @return The bounds of the loop L in { Lower bound, Upper bound } form.
  ///
  const SCEVAffFunc &getLoopBound(const Loop *L) const {
    LoopBoundMapType::const_iterator at = LoopBounds.find(L);
    assert(at != LoopBounds.end() && "Only valid loop is allow!");
    return at->second;
  }

  /// @brief Get the condition from entry block of the SCoP to a BasicBlock
  ///
  /// @param BB The BasicBlock
  ///
  /// @return The condition from entry block of the SCoP to a BB
  ///
  const BBCond *getBBCond(const BasicBlock *BB) const {
    BBCondMapType::const_iterator at = BBConds.find(BB);
    return at != BBConds.end() ? &(at->second) : 0;
  }

  /// @brief Get all access functions in a BasicBlock
  ///
  /// @param  BB The BasicBlock that containing the access functions.
  ///
  /// @return All access functions in BB
  ///
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
/// @brief The Region Pass to extract temporary information for Static control
///        part in llvm function.
///
class TempSCoPInfo : public RegionPass {
  //===-------------------------------------------------------------------===//
  // DO NOT IMPLEMENT
  TempSCoPInfo(const TempSCoPInfo &);
  // DO NOT IMPLEMENT
  const TempSCoPInfo &operator=(const TempSCoPInfo &);

  // The ScalarEvolution to help building SCoP.
  ScalarEvolution* SE;

  // LoopInfo for information about loops
  LoopInfo *LI;

  // Current region
  Region *CurR;
  // Valid Regions for SCoP
  SCoPDetection *SD;

  // FIXME: This is only a temporary hack, we need a standalone condition
  // analysis and construction pass.
  // For simple condition extraction support
  DominatorTree *DT;
  PostDominatorTree *PDT;

  // Remember the bounds of loops, to help us build iteration domain of BBs.
  LoopBoundMapType LoopBounds;

  // And also Remember the constrains for BBs
  BBCondMapType BBConds;

  // Access function of bbs.
  AccFuncMapType AccFuncMap;

  // SCoP for the current region.
  TempSCoP *TSCoP;

  // Clear the context.
  void clear();

  // Check if all parameters in Params valid in Region R.
  void mergeParams(Region &R, ParamSetType &Params,
                   ParamSetType &SubParams) const;

  /// @brief Build an affine function from a SCEV expression.
  ///
  /// @param S            The SCEV expression to be converted to affine
  ///                     function.
  /// @param SCoP         The Scope of this expression.
  /// @param FuncToBuild  The SCEVAffFunc to hold the result.
  ///
  void buildAffineFunction(const SCEV *S, SCEVAffFunc &FuncToBuild,
                           Region &R, ParamSetType &Params) const;


  /// @brief Build condition constrains to BBs in a valid SCoP.
  ///
  /// @param BB           The BasicBlock to build condition constrains
  /// @param RegionEntry  The entry block of the Smallest Region that containing
  ///                     BB
  /// @param Cond         The built condition
  void buildCondition(BasicBlock *BB, BasicBlock *RegionEntry, BBCond &Cond,
                      TempSCoP &SCoP);

  // Build the affine function of the given condition
  void buildAffineCondition(Value &V, bool inverted,  SCEVAffFunc &FuncToBuild,
                            TempSCoP &SCoP) const;
  
  // Return the temporary SCoP information of Region R, where R must be a valid
  // part of SCoP
  TempSCoP *getTempSCoP(Region &R);

  // Build the temprory information of Region R, where R must be a valid part
  // of SCoP.
  TempSCoP *buildTempSCoP(Region &R);
  TempSCoP *buildTempSCoP(Region &R, Region &RefRegion);

  void buildAccessFunctions(Region &RefRegion, ParamSetType &Params,
                            BasicBlock &BB);

  // Build the bounds of loop that corresponding to the outer most region of 
  // a given SCoP
  void buildLoopBound(TempSCoP &SCoP);

public:
  static char ID;
  explicit TempSCoPInfo() : RegionPass(&ID) {TSCoP = 0;}
  ~TempSCoPInfo();

  /// @brief Get the temporay SCoP information in LLVM IR represent
  ///        for Region R.
  ///
  /// @return The SCoP information in LLVM IR represent.
  TempSCoP *getTempSCoP() const;

  /// @name FunctionPass interface
  //@{
  virtual void getAnalysisUsage(AnalysisUsage &AU) const;
  virtual void releaseMemory() { clear(); }
  virtual bool doFinalization() { clear(); return false; }
  virtual bool runOnRegion(Region *R, RGPassManager &RGM);
  virtual void print(raw_ostream &OS, const Module *) const;
  //@}
};

} // end namespace polly


#endif
