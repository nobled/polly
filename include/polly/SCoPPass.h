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
#ifndef POLLY_SCOP_PASS_H
#define POLLY_SCOP_PASS_H

#include "llvm/Analysis/RegionPass.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Support/raw_ostream.h"
#include "polly/SCoPDetection.h"

struct isl_set;
struct isl_map;
struct isl_ctx;
struct isl_constraint;

#include <set>

using namespace llvm;

namespace polly {
class SCoPPass;

/// A statement is the smallest element of a SCoP.  It describes
/// a set of calculations that have to be executed.
class Statement {
  /// The statements to be exectuted.  At the moment only complete basic
  /// blocks are supported.
  /// TODO: Extend to arbitrary regions.
  BasicBlock *BB;

  /// The parent of this Statment.
  SCoPPass *Scop;

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

  /// Number iterators in Statment.
  unsigned NumIts;

  /// Iterator names(values)?

  void AllocateScattering(unsigned *value);
  void AllocateDomain();

  /// Create lower bound constraint for loop.
  struct isl_constraint *createLBConstraintForLoop(Loop *L);

  /// Create upper bound constraint for loop.
  struct isl_constraint *createUBConstraintForLoop(Loop *L);

public:
  // XXX: value needs a comment.
  Statement(SCoPPass *SCoP, BasicBlock *BB, unsigned *value);
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
class SCoPPass: public RegionPass {
  /// The underlying Region.
  Region *R;
  bool valid;

  /// Create statements in current SCoP.
  void createStmts();

  ///
  unsigned getLoopDepthImp(unsigned LD) const {
    if (Loop *SL = getSurroundingLoop())
      return LD - SL->getLoopDepth();
    else
      return LD;
  }

  typedef std::set<Statement*> StmtSet;
  /// The Statments in this SCoP.
  StmtSet Stmts;

  ///
public:

  /// iterator/begin/end
  typedef StmtSet::iterator iterator;
  typedef StmtSet::const_iterator const_iterator;

  iterator begin() { return Stmts.begin(); }
  iterator end()   { return Stmts.end(); }

  const_iterator begin() const { return Stmts.begin(); }
  const_iterator end()   const { return Stmts.end(); }

  /// The context of this SCoP.

  static char ID;
  ScalarEvolution *SE;
  LoopInfo *LI;
  SCoPDetection *SD;

  SCoPPass();
  ~SCoPPass();

  /// Context for ISL library?
  struct isl_ctx *isl_ctx;
  /// Scattering dimension number
  unsigned NumScatterDim;
  /// CloogNames
  /// Mapping LLVM value to parameter?
  /// Context
  // Is this constraints on parameters?
  struct isl_set *Context;
  /// Loops and block list

  /// @brief Get the Number of cloog params.
  unsigned getNumParams() const;

  /// @brief Get the underlying Region of this SCoP.
  Region *getRegion() const { return R; }
  bool isValid();

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
    // According to LoopInfo.h, "null" loop is in level 0.
    if (!L) return 0;

    unsigned LLD = L->getLoopDepth();
    return getLoopDepthImp(LLD);
  }

  unsigned getMaxLoopDepth() const;
  Loop *getSurroundingLoop() const;
};

/// Function for force linking.
llvm::RegionPass* createSCoPPass();
llvm::RegionPass* createScopPrinterPass();
llvm::RegionPass* createScopCodeGenPass();

} //end polly namespace.

#endif /* POLLY_SCOP_H */
