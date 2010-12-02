//===--------- SCoPPass.h - Pass for Static Control Parts --------*-C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the SCoPPass class.  SCoPPasses are just RegionPasses,
// except they operate on Polly IR (SCoP and SCoPStmt) built by SCoPInfo Pass.
// Because they operate on Polly IR, not the LLVM IR, SCoPPasses are not allowed
// to modify the LLVM IR. Due to this limitation, the SCoPPass class takes
// care of declaring that no LLVM passes are invalidated.
//
//===----------------------------------------------------------------------===//

#ifndef POLLY_SCOP_PASS_H
#define POLLY_SCOP_PASS_H

#include "llvm/Analysis/RegionPass.h"

using namespace llvm;

namespace polly {
class SCoP;

/// SCoPPass - This class adapts the RegionPass interface to allow convenient
/// creation of passes that operate on the Polly IR. Instead of overriding
/// runOnRegion, subclasses override runOnSCoP.
class SCoPPass : public RegionPass {
  SCoP *S;
protected:
  explicit SCoPPass(char &ID) : RegionPass(ID), S(0) {}

  /// runOnMachineFunction - This method must be overloaded to perform the
  /// desired machine code transformation or analysis.
  ///
  virtual bool runOnSCoP(SCoP &S) = 0;

  /// getAnalysisUsage - Subclasses that override getAnalysisUsage
  /// must call this.
  ///
  virtual void getAnalysisUsage(AnalysisUsage &AU) const;

private:
  virtual bool runOnRegion(Region *R, RGPassManager &RGM);
  void print(raw_ostream &OS, const Module *) const;
  virtual void printSCoP(raw_ostream &OS) const {}
};

} // End llvm namespace

#endif
