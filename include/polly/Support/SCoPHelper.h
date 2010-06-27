//===------ Support/SCoPHelper.h -- Some Helper Functions for SCoP. --------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Small functions that help with LLVM-IR.
//
//===----------------------------------------------------------------------===//

#ifndef POLLY_SUPPORT_IRHELPER_H
#define POLLY_SUPPORT_IRHELPER_H

namespace llvm {
  class Instruction;
  class ScalarEvolution;
  class LoopInfo;
  class Loop;
  class SCEV;
  class Value;
  class Region;
  class Pass;
  class BasicBlock;
}

namespace polly {
  // Helper function for SCoP
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
  llvm::Loop *castToLoop(const llvm::Region &R, llvm::LoopInfo &LI);

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
  llvm::Loop *getScopeLoop(const llvm::Region &R, llvm::LoopInfo &LI);

  //===----------------------------------------------------------------------===//
  // Functions for checking affine functions

  bool isParameter(const llvm::SCEV *Var, llvm::Region &RefRegion,
    llvm::BasicBlock *CurBB, llvm::LoopInfo &LI, llvm::ScalarEvolution &SE);

  bool isIndVar(const llvm::SCEV *Var, llvm::Region &RefRegion,
    llvm::BasicBlock *CurBB, llvm::LoopInfo &LI, llvm::ScalarEvolution &SE);

  llvm::Value *getPointerOperand(llvm::Instruction &Inst);

  // Helper function for LLVM-IR about SCoP
  llvm::BasicBlock *createSingleEntryEdge(llvm::Region *R, llvm::Pass *P);
  void createSingleExitEdge(llvm::Region *R, llvm::Pass *P);
}
#endif
