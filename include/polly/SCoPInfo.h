//===- polly/SCoP/SCoPInfo.h - Create SCoPs from LLVM IR --------*- C++ -*-===//
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
#ifndef POLLY_SCOP_INFO_H
#define POLLY_SCOP_INFO_H

#include "polly/PollyType.h"

#include "llvm/Analysis/RegionInfo.h"
#include "llvm/Analysis/RegionPass.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Scalar.h"

#include <set>
#include <map>

using namespace llvm;

namespace polly {

//===----------------------------------------------------------------------===//
class SCoP;
class SCoPInfo;

//===----------------------------------------------------------------------===//
/// @brief Statement in Static control part.
///
/// The Polly IR of BasicBlock contains:
///
/// Iteration domain, the iteration bounds of this BasicBlock.
///
/// Read/Write accesses in this BasicBlock, a set of pairs (P, f) where P is a
/// pointer value that written(stored)/read (load) in this BasicBlock and f is
/// an affine function mapping iterations in iteration domain to the read/write
/// offset of P.
///
/// Scattering function, an affine function specifying the execution schedule
/// of this BasicBlock in SCoP .
///
class SCoPStmt {
  //===-------------------------------------------------------------------===//
  // DO NOT IMPLEMENT
  SCoPStmt(const SCoPStmt &);
  // DO NOT IMPLEMENT
  const SCoPStmt &operator=(const SCoPStmt &);

  /// The SCoP containing this SCoPStmt
  SCoP &Parent;

  /// The BasicBlock represented by this SCoPStmt.
  BasicBlock &BB;

  /// The iteration domain describes the set of iterations for which this
  /// statement is executed.
  ///
  /// Example:
  ///     for (i = 0; i < 100 + b; ++i)
  ///       S(i);
  ///
  ///     Domain: 0 <= i <= 100 + b
  polly_set *Domain;
  /// The scattering map describes the order in which the different statements
  /// executed. The scattering is closely related to the one of CLooG. So have
  /// a look at cloog.org to find a complete description.
  polly_map *Scattering;

  /// TODO: access functions.

  /// Create the SCoPStmt from a BasicBlock.
  SCoPStmt(SCoP &parent, BasicBlock &bb, polly_set *domain, polly_map *scat);

  friend class SCoP;
public:
  ~SCoPStmt();

  /// @brief Get the iterate domain of this SCoPStmt.
  ///
  /// @return The iterate domain of this SCoPStmt.
  polly_set *getDomain() const { return Domain; }

  /// @brief Get the scattering function of this SCoPStmt.
  ///
  /// @return The scattering function of this SCoPStmt.
  polly_map *getScattering() const { return Scattering; }


  /// @brief Get the BasicBlock represented by this SCoPStmt.
  ///
  /// @return The BasicBlock represented by this SCoPStmt.
  BasicBlock *getBasicBlock() const { return &BB; }

  /// @brief Print the SCoPStmt.
  ///
  /// @param OS The output stream the SCoPStmt is printed to.
  void print(raw_ostream &OS) const;

  /// @brief Print the SCoPStmt to stderr.
  void dump() const;
};

/// @brief Print SCoPStmt S to raw_ostream O.
static inline raw_ostream& operator<<(raw_ostream &O, const SCoPStmt &S) {
  S.print(O);
  return O;
}

class TempSCoP;

//===----------------------------------------------------------------------===//
/// @brief Static Control Part in program tree.
///
/// The Polly IR of SCoPs in LLVM IR contains:
///
/// A set of BasicBlocks in their polyhedral intermediate representation.
///
/// Global parameters. A set of a LLVM Values with integer type defined outside
/// the SCoP and used inside the SCoP. The value of a global parameter does not
/// change during the execution of the SCoP, but we cannot know its value at
/// compile time.
///
class SCoP {
  //===-------------------------------------------------------------------===//
  // DO NOT IMPLEMENT
  SCoP(const SCoP &);
  // DO NOT IMPLEMENT
  const SCoP &operator=(const SCoP &);

  /// The underlying Region.
  Region &R;

  /// Max loop depth.
  unsigned MaxLoopDepth;

  typedef std::set<SCoPStmt*> StmtSet;
  /// The Statments in this SCoP.
  StmtSet Stmts;

  /// Parameters of this SCoP
  typedef SmallVector<const SCEV*, 8> ParamVecType;
  ParamVecType Parameters;

  /// Constraints on parameters.
  polly_set *Context;

  ///
  polly_ctx *ctx;

  /// Create the static control part with a region, max loop depth of this region
  /// and parameters used in this region.
  template<class It>
  explicit SCoP(Region &r, unsigned maxLoopDepth, It ParamBegin, It ParamEnd);

  /// Build the SCoP and Statement with precalculate scop information.
  void buildSCoP(TempSCoP &TempSCoP, const Region &CurRegion,
                  SmallVectorImpl<Loop*> &NestLoops,
                  SmallVectorImpl<unsigned> &Scatter,
                  LoopInfo &LI, ScalarEvolution &SE);
  void buildStmt(TempSCoP &TempSCoP, BasicBlock &BB,
                  SmallVectorImpl<Loop*> &NestLoops,
                  SmallVectorImpl<unsigned> &Scatter,
                  ScalarEvolution &SE);

  /// Helper function for printing the SCoP.
  void printContext(raw_ostream &OS) const;
  void printStatements(raw_ostream &OS) const;

  friend class SCoPInfo;
public:

  ~SCoP();

  /// @brief Get the count of parameters used in this SCoP.
  ///
  /// @return The count of parameters used in this SCoP.
  inline ParamVecType::size_type getNumParams() const {
    return Parameters.size();
  }

  /// @brief Get a set containing the parameters used in this SCoP
  ///
  /// @return The set containing the parameters used in this SCoP.
  inline const ParamVecType &getParams() const { return Parameters; }

  /// @name Parameter Iterators
  ///
  /// These iterators iterate over all parameters of this SCoP.
  //@{
  typedef ParamVecType::iterator param_iterator;
  typedef ParamVecType::const_iterator const_param_iterator;

  param_iterator param_begin() { return Parameters.begin(); }
  param_iterator param_end()   { return Parameters.end(); }
  const_param_iterator param_begin() const { return Parameters.begin(); }
  const_param_iterator param_end()   const { return Parameters.end(); }
  //@}

  /// @brief Get the maximum region of this static control part.
  ///
  /// @return The maximum region of this static control part.
  inline const Region &getRegion() const { return R; }

  /// @brief Get the maximum region of this static control part.
  ///
  /// @return The maximum region of this static control part.
  inline unsigned getMaxLoopDepth() const { return MaxLoopDepth; }

  /// @brief Get the scattering dimension number of this SCoP.
  ///
  /// @return The scattering dimension number of this SCoP.
  inline unsigned getScatterDim() const { return 2 * MaxLoopDepth + 1; }

  /// @brief Get the constraint on parameter of this SCoP.
  ///
  /// @return The constraint on parameter of this SCoP.
  inline polly_set *getContext() const { return Context; }

  /// @name Statments Iterators
  ///
  /// These iterators iterate over all statements of this SCoP.
  //@{
  typedef StmtSet::iterator iterator;
  typedef StmtSet::const_iterator const_iterator;

  iterator begin() { return Stmts.begin(); }
  iterator end()   { return Stmts.end();   }
  const_iterator begin() const { return Stmts.begin(); }
  const_iterator end()   const { return Stmts.end();   }
  //@}

  /// @brief Print the static control part.
  ///
  /// @param OS The output stream the static control part is printed to.
  void print(raw_ostream &OS) const;

  /// @brief Print the SCoPStmt to stderr.
  void dump() const;

  /// @brief Get the isl context of this static control part.
  ///
  /// @return The isl context of this static control part.
  polly_ctx *getCtx() const { return ctx; }
};

/// @brief Print SCoP scop to raw_ostream O.
static inline raw_ostream& operator<<(raw_ostream &O, const SCoP &scop) {
  scop.print(O);
  return O;
}

//===---------------------------------------------------------------------===//
/// @brief Build the Polly IR (SCoP and SCoPStmt) on a Region.
///
class SCoPInfo : public RegionPass {
  //===-------------------------------------------------------------------===//
  // DO NOT IMPLEMENT
  SCoPInfo(const SCoPInfo &);
  // DO NOT IMPLEMENT
  const SCoPInfo &operator=(const SCoPInfo &);

  // The SCoP
  SCoP *scop;

  void clear() {
    if (scop) {
      delete scop;
      scop = 0;
    }
  }

public:
  static char ID;
  explicit SCoPInfo() : RegionPass(&ID), scop(0) {}
  ~SCoPInfo() { clear(); }

  /// @brief Try to build the Polly IR of static control part on the current
  ///        SESE-Region.
  ///
  /// @return If the current region is a valid for a static control part,
  ///         return the Polly IR representing this static control part,
  ///         return null otherwise.
  SCoP *getSCoP() { return scop; }
  const SCoP *getSCoP() const { return scop; }

  /// @name RegionPass interface
  //@{
  virtual bool runOnRegion(Region *R, RGPassManager &RGM);
  virtual void getAnalysisUsage(AnalysisUsage &AU) const;
  virtual void releaseMemory() { clear(); }
  virtual void print(raw_ostream &OS, const Module *) const {
    if (scop)
      scop->print(OS);
    else
      OS << "Invalid SCoP!\n";
  }
  //@}
};

/// Function for force linking.
Pass *createSCoPInfoPass();
Pass *createScopPrinterPass();
Pass *createScopCodeGenPass();

} //end namespace polly

#endif
