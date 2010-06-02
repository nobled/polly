//===-- polly/SCEVAffFunc.h - Express affine functions by SCEV --*- C++ -*-===//
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
#ifndef POLLY_SCEV_AFFINE_FUNCTION_H
#define POLLY_SCEV_AFFINE_FUNCTION_H


#include "llvm/Analysis/RegionInfo.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace polly {
typedef std::set<const SCEV*> ParamSetType;

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

public:
  enum AccessType {
    None = 0,
    Read = 1, // Or we could call it "Use"
    Write = 2 // Or define
  };
  // Pair of {address, read/write}
  typedef PointerIntPair<Value*, 2, AccessType> MemAccTy;
private:
  // The base address of the address SCEV, if the Value is a pointer, this is
  // an array access, otherwise, this is a value access.
  // And the Write/Read modifier
  MemAccTy BaseAddr;

  // getCoeff - Get the Coefficient of a given variable.
  const SCEV *getCoeff(const SCEV *Var) const {
    LnrTransSet::const_iterator At = LnrTrans.find(Var);
    return At == LnrTrans.end() ? 0 : At->second;
  }

public:
  /// @brief Create a new SCEV affine function.
  explicit SCEVAffFunc() : TransComp(0), BaseAddr(0, SCEVAffFunc::None) {}

  /// @brief Create a new SCEV affine function with memory access type.

  explicit SCEVAffFunc(AccessType Type, Value* baseAddr = 0)
    : TransComp(0), BaseAddr(baseAddr, Type) {}

  /// @brief Build an affine function from a SCEV expression.
  ///
  /// @param S            The SCEV expression to be converted to affine
  ///                     function.
  /// @param SCoP         The Scope of this expression.
  /// @param FuncToBuild  The SCEVAffFunc to hold the result, you can pass 0 if
  ///                     you only want to check if S is affine.
  /// @param LI           The LoopInfo to help to build the affine function.
  /// @param SE           The ScalarEvolution to help to build the affine
  ///                     function.
  /// @param AccType      If S is a pointer value, use AccType to indicate that
  ///                     is this a memory read or a memory write.
  ///
  /// @return             Return true if S could be convert to affine function,
  ///                     false otherwise.
  static bool buildAffineFunc(const SCEV *S, Region &R, ParamSetType &Params,
    SCEVAffFunc *FuncToBuild, LoopInfo &LI, ScalarEvolution &SE,
    AccessType AccType = SCEVAffFunc::None);

  static bool buildMemoryAccess(MemAccTy MemAcc, Region &R, ParamSetType &Params,
    SCEVAffFunc *FuncToBuild, LoopInfo &LI, ScalarEvolution &SE);

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
    bool isLower) const;

  polly_constraint *toAccessFunction(polly_ctx *ctx, polly_dim* dim,
    const SmallVectorImpl<Loop*> &NestLoops,
    const SmallVectorImpl<const SCEV*> &Params,
    ScalarEvolution &SE) const;


  bool isDataRef() const { return BaseAddr.getInt() != SCEVAffFunc::None; }

  bool isRead() const { return BaseAddr.getInt() == SCEVAffFunc::Read; }

  const Value *getBaseAddr() const { return BaseAddr.getPointer(); }

  /// @brief Print the affine function.
  ///
  /// @param OS The output stream the affine function is printed to.
  /// @param SE The ScalarEvolution that help printing the affine function.
  void print(raw_ostream &OS, ScalarEvolution *SE) const;
};

}

#endif
