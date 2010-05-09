//===- polly/ScalarDataRef.h - Capture scalar data reference ----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the scalar data reference analysis, whice will capture
// scalar reads/writes(use/def) in SCoP statement(BB or Region).
//
//===----------------------------------------------------------------------===//
//

#ifndef POLLY_SCALAR_DATA_REF
#define POLLY_SCALAR_DATA_REF

#include "llvm/Instruction.h"
#include "llvm/Pass.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/Dominators.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/PointerIntPair.h"


using namespace llvm;

namespace polly {
//===----------------------------------------------------------------------===//
/// @brief Scalar data reference - Analysis that provides scalar data
///        read/write reference in BasicBlocks or Regions.
///
///
class ScalarDataRef : public FunctionPass {
  // DO NOT IMPLEMENT
  ScalarDataRef(const ScalarDataRef &);
  // DO NOT IMPLEMENT
  const ScalarDataRef &operator=(const ScalarDataRef &);

  /// Reference Record
  class RefRec {
    // { Value, isCyclic }
    PointerIntPair<const Value *, 1, bool> Val;
    //
    unsigned UseNum;

  public:
    RefRec(const Value *v, bool cyclic, unsigned useNum)
      : Val(v, cyclic), UseNum(useNum) {
        assert(getValue() && "Value can not be null!");
    }

    const Value *getValue() const { return Val.getPointer(); }

    bool isCyclic() const { return Val.getInt(); }

    bool isDef() const { return UseNum != 0; }

    void print(raw_ostream &OS) const;
  };

  typedef DenseMap<const BasicBlock*, SmallVector<RefRec, 4> > DataRefMapTy;

  // Data referenced in each BasicBlock.
  DataRefMapTy DataRefs;

  // LoopInfo to compute canonical induction variable
  LoopInfo *LI;
  // DominatorTree to compute cyclic reference.
  DominatorTree *DT;

  Function *Func;

  void clear();

public:
  static char ID;

  ScalarDataRef() : FunctionPass(&ID) {}
  ~ScalarDataRef();

  /// @brief Compute the data reference for scalar define by Instruction I
  ///
  /// @param I The Instruction to compute data Reference.
  ///
  /// @return True if there is any valid data reference, false otherwise.
  bool computeDataRefForScalar(Instruction &I);

  /// @brief Remove the data reference for scalar use directly or indirectly used
  ///        by Instruction I
  ///
  /// @param I The Instruction.
  ///
  void killAllUseOf(Instruction &I);

  /// @name FunctionPass interface
  //@{
  virtual void getAnalysisUsage(AnalysisUsage &AU) const;
  virtual void releaseMemory();
  virtual bool runOnFunction(Function &F);
  virtual void print(raw_ostream &OS, const Module *) const;
  //@}

};

Pass *createScalarDataRefPass();

}

#endif
