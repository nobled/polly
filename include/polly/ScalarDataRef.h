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
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/PointerIntPair.h"


using namespace llvm;

namespace polly {

typedef DenseMap<const Instruction*, int> SDRDataRefMapTy;

typedef std::set<const Instruction*> SDRInstSetTy;

typedef std::vector<const Instruction*> SDRInstVecTy;

typedef std::set<const BasicBlock*> SDRBBSetTy;

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

  // Data definition
  SDRDataRefMapTy DataRefs;

  // LoopInfo to compute canonical induction variable
  LoopInfo *LI;
  // DominatorTree to compute cyclic reference.
  DominatorTree *DT;

  Function *Func;

  void clear();

  int &getDataRefFor(const Instruction &Inst);

  bool killedAsTempVal(const Instruction &Inst) const;

public:
  static char ID;

  explicit ScalarDataRef() : FunctionPass(&ID) {}
  ~ScalarDataRef();

  bool isCyclicUse(const Instruction* def, const Instruction* use) const;

  bool isDefExported(Instruction &I) const;

  void getAllUsing(Instruction &Inst, SmallVectorImpl<const Value*> &Defs);

  void reduceTempRefFor(const Instruction& Inst);


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
