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

class SCoPStmt {
  //===-------------------------------------------------------------------===//
  // DO NOT IMPLEMENT
  SCoPStmt(const SCoPStmt &);
  // DO NOT IMPLEMENT
  const SCoPStmt &operator=(const SCoPStmt &);

  SCoP &Parent;
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

  SCoPStmt(SCoP &parent, BasicBlock &bb, polly_set *domain, polly_map *scat);

  friend class SCoP;
public:
  ~SCoPStmt();

  polly_set *getDomain() const { return Domain; }
  polly_map *getScattering() const { return Scattering; }

  BasicBlock *getBasicBlock() const { return &BB; }

  void print(raw_ostream &OS) const;
  void dump() const;
};

/// @brief Print SCoPStmt S to raw_ostream O.
static inline raw_ostream& operator<<(raw_ostream &O, const SCoPStmt &S) {
  S.print(O);
  return O;
}

class TempSCoP;

/// @brief Static Control Part in program tree.
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
  /// Context
  // Is this constraints on parameters?
  polly_set *Context;

  ///
  polly_ctx *ctx;

  template<class It>
  explicit SCoP(Region &r, unsigned maxLoopDepth, It ParamBegin, It ParamEnd);



  void buildSCoP(TempSCoP &TempSCoP, const Region &CurRegion,
                  SmallVectorImpl<Loop*> &NestLoops,
                  SmallVectorImpl<unsigned> &Scatter,
                  LoopInfo &LI, ScalarEvolution &SE);

  void buildStmt(TempSCoP &TempSCoP, BasicBlock &BB,
                  SmallVectorImpl<Loop*> &NestLoops,
                  SmallVectorImpl<unsigned> &Scatter,
                  ScalarEvolution &SE);

  friend class SCoPInfo;
public:

  ~SCoP();

  inline ParamVecType::size_type getNumParams() const {
    return Parameters.size();
  }
  inline const ParamVecType &getParams() const { return Parameters; }

  //void addStatement(SCoPStmt *stmt) { Stmts.insert(stmt); }

  typedef StmtSet::iterator iterator;
  typedef StmtSet::const_iterator const_iterator;

  iterator begin() { return Stmts.begin(); }
  iterator end()   { return Stmts.end();   }

  const_iterator begin() const { return Stmts.begin(); }
  const_iterator end()   const { return Stmts.end();   }

  typedef ParamVecType::iterator param_iterator;
  typedef ParamVecType::const_iterator const_param_iterator;

  param_iterator param_begin() { return Parameters.begin(); }
  param_iterator param_end()   { return Parameters.end(); }

  const_param_iterator param_begin() const { return Parameters.begin(); }
  const_param_iterator param_end()   const { return Parameters.end(); }

  inline const Region &getRegion() const { return R; }

  inline unsigned getMaxLoopDepth() const { return MaxLoopDepth; }
  /// Scattering dimension number
  inline unsigned getScatterDim() const { return 2 * MaxLoopDepth + 1; }

  inline polly_set *getContext() const { return Context; }

  void print(raw_ostream &OS) const;
  void printContext(raw_ostream &OS) const;
  void printStatements(raw_ostream &OS) const;
  void dump() const;

  polly_ctx *getCtx() const { return ctx; }
};

/// @brief Print SCoP scop to raw_ostream O.
static inline raw_ostream& operator<<(raw_ostream &O, const SCoP &scop) {
  scop.print(O);
  return O;
}

class SCoPInfo : public RegionPass {
  //===-------------------------------------------------------------------===//
  // DO NOT IMPLEMENT
  SCoPInfo(const SCoPInfo &);
  // DO NOT IMPLEMENT
  const SCoPInfo &operator=(const SCoPInfo &);

  // The SCoP
  SCoP *scop;

  /*__isl_give*/
  polly_basic_set *buildIterateDomain(SCoP &SCoP, TempSCoP &TempSCoP,
                                      SmallVectorImpl<Loop*> &NestLoops);

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

  SCoP *getSCoP() { return scop; }
  const SCoP *getSCoP() const { return scop; }

  virtual bool runOnRegion(Region *R, RGPassManager &RGM);
  virtual void getAnalysisUsage(AnalysisUsage &AU) const;
  virtual void releaseMemory() { clear(); }
  virtual void print(raw_ostream &OS, const Module *) const {
    if (scop)
      scop->print(OS);
    else
      OS << "Invalid SCoP!\n";
  }
};

/// Function for force linking.
Pass *createSCoPInfoPass();
Pass* createScopPrinterPass();
Pass* createScopCodeGenPass();

} //end namespace polly

#endif
