//===- SCoP.h - Create a SCoP. ----------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Create a polyhedral description for a static control flow region.
//
//===----------------------------------------------------------------------===//
//
#ifndef POLLY_SCOP_H
#define POLLY_SCOP_H

#include "llvm/Analysis/RegionPass.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Support/raw_ostream.h"

struct isl_set;
struct isl_map;
struct isl_ctx;
struct isl_constraint;

#include <set>

using namespace llvm;

namespace polly {
class SCoP;

/// A statement is the smallest element of a SCoP.  It describes
/// a set of calculations that have to be executed.
class Statement {
  /// The statements to be exectuted.  At the moment only complete basic
  /// blocks are supported.
  /// TODO: Extend to arbitrary regions.
  BasicBlock *BB;

  /// The parent of this Statment.
  SCoP *Scop;

  /// The iteration domain describes the set of iterations for which this
  /// statement is executed.
  ///
  /// Example:
  ///     for (i = 0; i < 100 + b; ++i)
  ///       S(i);
  ///
  ///     Domain: 0 <= i <= 100 + b
  struct isl_set *Domain;

  /// The scattering map describes the order in which the different statements
  /// executed. The scattering is closely related to the one of CLooG. So have
  /// a look at cloog.org to find a complete description.
  struct isl_map *Scattering;

  /// Number iterators in SCoP.
  unsigned NumIts;

  void AllocateScattering(unsigned *value);
  void AllocateDomain();

  /// Create Lower bound constraint for loop.
  struct isl_constraint *createLBConstraintForLoop(Loop *L);

  /// Create Upper bound constraint for loop.
  struct isl_constraint *createUBConstraintForLoop(Loop *L);

public:
  // XXX: value needs a comment.
  Statement(SCoP *SCoP, BasicBlock *BB, unsigned *value);
  ~Statement();

  /// FIXME: This is only a dirty hack. If a statement can hold a BB or Region,
  /// maybe we can use a RegionNode to hold it.
  BasicBlock *getBasicBlock() {
    return BB;
  }

  struct isl_map *getScattering() {
    return Scattering;
  }

  struct isl_set *getDomain() {
    return Domain;
  }

  /// @brief Print the Statement to OS.
  ///
  /// @param OS The output stream to print this Statememt
  void print(raw_ostream &OS) const;
};

/// @brief Print Statment S to raw_ostream O.
static inline raw_ostream& operator<<(raw_ostream &O, const Statement *S) {
    S->print(O);
    return O;
}

/// @brief Static Control Part in program tree.
class SCoP: public RegionPass {
  /// The underlying Region.
  Region *R;

  void findBlackBoxes();

  ///
  unsigned getLoopDepthImp(unsigned LD) const {
    if (Loop *SL = getSurroundingLoop())
      return LD - SL->getLoopDepth();
    else
      return LD;
  }

public:
  typedef std::set<Statement*> StmtSet;
  /// The Statments in this SCoP.
  StmtSet Stmts;

  /// The context of this SCoP.
  struct isl_set *Context;
  struct isl_ctx *isl_ctx;
  static char ID;
  unsigned NbScatteringDimensions;
  ScalarEvolution *SE;
  LoopInfo *LI;

  SCoP();
  ~SCoP();

  /// @brief Get the Number of cloog params.
  unsigned getNumParams() const;

  /// @brief Get the underlying Region of this SCoP.
  Region *getRegion() const { return R; }

  bool runOnRegion(Region *R, RGPassManager &RGM);
  virtual void getAnalysisUsage(AnalysisUsage &AU) const;

  void printContext(raw_ostream &OS) const;
  void printStatements(raw_ostream &OS) const;
  virtual void print(raw_ostream &OS, const Module *) const;

  ///
  unsigned getLoopDepth(BasicBlock *BB) const {
    unsigned BBLD = LI->getLoopDepth(BB);
    return getLoopDepthImp(BBLD);
  }

  unsigned getLoopDepth(Loop *L) const {
    unsigned LLD = L->getLoopDepth();
    return getLoopDepthImp(LLD);
  }

  unsigned getMaxLoopDepth() const;
  Loop *getSurroundingLoop() const;
};

/// Function for force linking.
llvm::RegionPass* createSCoPPass();
llvm::RegionPass* createScopPrinterPass();

} //end polly namespace.

#endif /* POLLY_SCOP_H */
