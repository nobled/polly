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
#include "polly/ScalarDataRef.h"
#include "polly/Support/AffineSCEVIterator.h"

#include "llvm/Analysis/RegionInfo.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/PointerIntPair.h"
#include "llvm/Transforms/Scalar.h"

using namespace llvm;

namespace polly {
typedef std::set<const SCEV*> ParamSetType;
typedef std::pair<const SCEV*, const SCEV*> AffCmptType;

class TempSCoP;
class SCoPDetection;
class SCEVAffFunc;

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
///
/// @return If there is a loop have the same entry and exit with R or its parent,
///          return the loop, otherwise, return null.
Loop *getScopeLoop(const Region &R, LoopInfo &LI);

//===---------------------------------------------------------------------===//
/// @brief Affine function represent in llvm SCEV expressions.
///
/// A helper class for collect affine function information
class SCEVAffFunc {
  // Temporary hack
  friend class SCoPDetection;
  // The translation component
  const SCEV *TransComp;

  // { Variable, Coefficient }
  typedef std::map<const SCEV*, const SCEV*> LnrTransSet;
  LnrTransSet LnrTrans;

public:
  // FIXME: This class doing too much jobs?
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
    FuncType(SCEVAffFunc::None) {}

  /// @brief Create a new SCEV affine function with memory access type or
  ///        condition type

  explicit SCEVAffFunc(SCEVAffFuncType Type, Value* baseAddr = 0)
    : TransComp(0), BaseAddr(baseAddr), FuncType(Type) {}

  /// @brief Build a loop bound constrain from an affine function.
  ///
  /// @param ctx      The context of isl objects.
  /// @param dim      The dimension of the the constrain.
  /// @param IndVars  The induction variable may appear in the affine function.
  /// @param Params   The parameters may appear in the affine funcion.
  /// @param isLower  Is this the lower bound?
  ///
  /// @return         The isl_constrain represent by this affine function.
  polly_set *toConditionConstrain(polly_ctx *ctx, polly_dim *dim,
    const SmallVectorImpl<const SCEVAddRecExpr*> &IndVars,
    const SmallVectorImpl<const SCEV*> &Params) const;

  polly_constraint *toAccessFunction(polly_ctx *ctx, polly_dim* dim,
    const SmallVectorImpl<Loop*> &NestLoops,
    const SmallVectorImpl<const SCEV*> &Params,
    ScalarEvolution &SE) const;


  bool isDataRef() const {
    return FuncType == SCEVAffFunc::ReadMem
           || FuncType == SCEVAffFunc::WriteMem;
  }

  enum SCEVAffFuncType getType() const { return FuncType; }

  bool isRead() const { return FuncType == SCEVAffFunc::ReadMem; }

  const Value *getBaseAddr() const { return BaseAddr; }

  /// @brief Print the affine function.
  ///
  /// @param OS The output stream the affine function is printed to.
  /// @param SE The ScalarEvolution that help printing the affine function.
  void print(raw_ostream &OS, ScalarEvolution *SE) const;
};

//===---------------------------------------------------------------------===//
/// Types
// The condition of a Basicblock, combine brcond with "And" operator.
typedef SmallVector<SCEVAffFunc, 4> BBCond;

/// Mapping loops to its bounds.
typedef std::map<const Loop*, BBCond> LoopBoundMapType;
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

  // Remember the bounds of loops, to help us build iterate domain of BBs.
  const LoopBoundMapType &LoopBounds;
  const BBCondMapType &BBConds;

  // Access function of bbs.
  const AccFuncMapType &AccFuncMap;

  friend class SCoPDetection;

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
  // @return The bounds of the loop L in { Lower bound, Upper bound } form.
  const BBCond &getLoopBound(const Loop *L) const {
    LoopBoundMapType::const_iterator at = LoopBounds.find(L);
    assert(at != LoopBounds.end() && "Only valid loop is allow!");
    return at->second;
  }

  const BBCond *getBBCond(const BasicBlock *BB) const {
    BBCondMapType::const_iterator at = BBConds.find(BB);
    return at != BBConds.end() ? &(at->second) : 0;
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

  // FIXME: This is only a temporary hack, we need a standalone condition
  // analysis and construction pass.
  // For simple condition extraction support
  DominatorTree *DT;
  PostDominatorTree *PDT;

  // Remember the bounds of loops, to help us build iterate domain of BBs.
  LoopBoundMapType LoopBounds;

  // And also Remember the constrains for BBs
  BBCondMapType BBConds;

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
  // Check if the max region of SCoP is valid, return true if it is valid
  // false otherwise.
  //
  // NOTE: All this function will increase the statistic counters.

  /// @brief Check if a Region is a valid element of a SCoP.
  ///
  ///
  /// @param RefRegion The region in respect to which the correctness is
  ///                        checked.
  /// @param CurRegion The region that is checked to be a valid element of
  ///                      the RefRegion.
  ///
  /// @return Return true if R is a valid subregion of R.
  bool isValidRegion(Region &RefRegion, Region &CurRegion) const;

  // Check if the instruction is a valid function call.
  static bool isValidCallInst(CallInst &CI);

  // Check is a memory access is valid.
  bool isValidMemoryAccess(Instruction &Inst, Region &RefRegion) const;

  // Check if all parameters in Params valid in Region R.
  void mergeParams(Region &R, ParamSetType &Params,
                   ParamSetType &SubParams) const;

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

  /// @brief Is a loop valid with respect to a given region.
  ///
  /// @param L The loop to check.
  /// @param RefRegion The region we analyse the loop in.
  ///
  /// @return True if the loop is valid in the region.
  bool isValidLoop(Loop *L, Region &RefRegion) const;

  /// @brief Build an affine function from a SCEV expression.
  ///
  /// @param S            The SCEV expression to be converted to affine
  ///                     function.
  /// @param SCoP         The Scope of this expression.
  /// @param FuncToBuild  The SCEVAffFunc to hold the result.
  ///
  void buildAffineFunction(const SCEV *S, SCEVAffFunc &FuncToBuild,
                           TempSCoP &SCoP) const;

  bool isValidAffineFunction(const SCEV *S, Region &RefRegion, BasicBlock *CurBB,
                             bool isMemAcc) const;



  /// @brief Build condition constrains to BBs in a valid SCoP.
  ///
  /// @param BB           The BasicBlock to build condition constrains
  /// @param RegionEntry  The entry block of the Smallest Region that containing
  ///                     BB
  /// @param Cond         The built condition
  void buildCondition(BasicBlock *BB, BasicBlock *RegionEntry, BBCond &Cond,
                      TempSCoP &SCoP);

  void buildAffineCondition(Value &V, bool inverted,  SCEVAffFunc &FuncToBuild,
                            TempSCoP &SCoP) const;
  /////////////////////////////////////////////////////////////////////////////
  // If the Region not a valid part of a SCoP,
  // return null if Region R is a valid part of a SCoP,
  // otherwise return the temporary SCoP information of Region R.
  TempSCoP *getTempSCoP(Region &R);

  TempSCoP *buildTempSCoP(Region &R);

  // Extract the access functions from a BasicBlock to ScalarAccs
  void buildAccessFunctions(TempSCoP &SCoP, BasicBlock &BB,
                            AccFuncSetType &AccessFunctions);

  void buildLoopBounds(TempSCoP &SCoP);

  // Capture scalar data reference. Fill the scalar "memory access" to the
  // access function map.
  void buildScalarDataRef(Instruction &I, AccFuncSetType &ScalarAccs);

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
    return isSCoP(R)
      && ((R.getParent() == 0) || !isSCoP(*R.getParent()));
  }

  /// @brief Is the region is a valid SCoP?
  ///
  /// @param R The Region to test if it is a valid SCoP.
  ///
  /// @return Return true if R is a valid SCoP, false otherwise.
  bool isSCoP(const Region &R) const {
    return RegionToSCoPs.count(&R);
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
