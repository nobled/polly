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

#include "polly/SCoPDetection.h"

#include "llvm/Instruction.h"
#include "llvm/Pass.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/RegionInfo.h"
#include "llvm/Analysis/Dominators.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/PointerIntPair.h"


using namespace llvm;

namespace polly {
//===----------------------------------------------------------------------===//
/// @brief Scalar data reference - Analysis that provides scalar data
///        read/write reference in BasicBlocks or Regions.
///
/// All instructions that is read by other BB(statement, the schedule unit
/// of SCOP) should be consider as it have a "write" in its defining BB, and
/// have a "read" access in others BB that reading it, so we can preserve the
/// scalar data dependence between bbs.
///
/// But the dependence for instructions that only help to compute the affine
/// function of conditions or address is no need to consider because we
/// already express them in access functions and iteration domain, so what we
/// need to do is "kill" them and not expose them as scalar access functions.
///
/// The ScalarDataRef pass to compute which instructions should be expose as
/// scalar data access, and which should not.
///
/// ScalarDataRef will kill the unnecessary scalar data reference by:
/// 1. maintain a map that mapping a instruction to its use number.
/// 2. if we try to kill an instruction, it will reduce the use number in
/// the map by one.
/// 3. if the use number is zero after the reduction, then the instruction
/// is "kill" and will not expose as scalar data access, since the
/// instruction is killed, all its operand maybe useless, so we just perform
/// step 2 - step 3 on all of its instruction operand.
///
/// In fact, this pass preform per-region analyze, but all Scalar data
/// reference analysis must be preform before temporary SCoP information
///  extraction, so this pass is implemented as a FunctionPass.
///
class ScalarDataRef : public FunctionPass {
  // Mapping instruction to use number.
  typedef DenseMap<const Instruction*, int> SDRDataRefMapTy;

  typedef std::set<const Instruction*> SDRInstSetTy;

  typedef std::vector<const Instruction*> SDRInstVecTy;

  typedef std::set<const BasicBlock*> SDRBBSetTy;

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
  // SCoPDetection help the check the valid region for scops.
  SCoPDetection *SD;

  // Clear the context.
  void clear();

  // Get the use number of Inst
  int &getDataRefFor(const Instruction &Inst);

  // If Inst is killed as temporary value and should not be expose
  // as scalar data reference?
  bool isKilledAsTempVal(const Instruction &Inst) const;

  // If use dominates def, and use is a PHINode? (This means we found a
  // loop dependence pair?)
  bool isCyclicUse(const Instruction* def, const Instruction* use) const;


  // Kill all temporary value that can be rewrite by SCEV Expander.
  void killAllTempValFor(const Region &R);
  void killAllTempValFor(Loop &L);
  void killAllTempValFor(BasicBlock &BB);

  /// @brief Hide all instructions that only contribute to the computation of
  ///        Inst
  ///
  /// @param Inst The final result of all killed instruction.
  void killTempRefFor(const Instruction& Inst);

  /// @brief If an Instruction should be expose as a "write memory" access?
  ///
  /// @param I The Instruction to query.
  ///
  /// @return Return true if the instruction should be export
  //          as a "write memory" access.
  bool isDefExported(Instruction &I) const;
  bool isUseExported(Instruction &I) const;

  // Run On regions to preform Scalar data reference analysis
  void runOnRegion(Region &R);
public:
  static char ID;

  explicit ScalarDataRef() : FunctionPass(&ID) {}
  ~ScalarDataRef();

  /// @name FunctionPass interface
  //@{
  virtual void getAnalysisUsage(AnalysisUsage &AU) const;
  virtual void releaseMemory();
  virtual bool runOnFunction(Function &F);
  //@}

};

Pass *createScalarDataRefPass();

}

#endif
