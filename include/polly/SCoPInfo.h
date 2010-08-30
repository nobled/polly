//===------ polly/SCoPInfo.h - Create SCoPs from LLVM IR --------*- C++ -*-===//
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
#include "llvm/Analysis/ScalarEvolutionExpressions.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/ADT/PointerIntPair.h"
#include "llvm/ADT/PointerUnion.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Scalar.h"

#include <set>
#include <map>

using namespace llvm;

namespace polly {

//===----------------------------------------------------------------------===//
class SCoP;
class SCoPInfo;
class TempSCoP;

//===----------------------------------------------------------------------===//
/// @brief Represent memory accesses in statements.
class MemoryAccess {
  //===-------------------------------------------------------------------===//
  // DO NOT IMPLEMENT
  MemoryAccess(const MemoryAccess &);
  // DO NOT IMPLEMENT
  const MemoryAccess &operator=(const MemoryAccess &);

public:
  enum AccessType {
    Read, // Or we could call it "Use"
    Write // Or define
  };

private:
  // The access function map iteration domain to memory location.
  PointerIntPair<polly_map*, 1, AccessType> AccFunc;

  // Base address and access type.
  const Value* BaseAddr;

public:
  MemoryAccess(const Value *Base, AccessType AccType, polly_map *accFunc)
    : AccFunc(accFunc, AccType), BaseAddr(Base) {}

  ~MemoryAccess();

  bool isRead() const { return AccFunc.getInt() == MemoryAccess::Read; }

  polly_map *getAccessFunction() { return AccFunc.getPointer(); }
  polly_map *getAccessFunction() const { return AccFunc.getPointer(); }

  const Value *getBaseAddr() const {
    return BaseAddr;
  }

  /// @brief Print the MemoryAccess.
  ///
  /// @param OS The output stream the MemoryAccess is printed to.
  void print(raw_ostream &OS) const;

  /// @brief Print the MemoryAccess to stderr.
  void dump() const;

};

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
  BasicBlock *BB;

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

  typedef SmallVector<MemoryAccess*, 8> MemoryAccessVec;
  MemoryAccessVec MemAccs;

  typedef SmallVector<const SCEVAddRecExpr*, 8> IndVarVec;
  void addConditionsToDomain(TempSCoP &tempSCoP,
                             const Region &CurRegion,
                             IndVarVec &IndVars);
  void buildIterationDomainFromLoops(TempSCoP &tempSCoP,
                                     IndVarVec &IndVars);
  void buildIterationDomain(TempSCoP &tempSCoP,
                            const Region &CurRegion,
                            ScalarEvolution &SE);
  void buildScattering(SmallVectorImpl<unsigned> &Scatter,
                       unsigned CurLoopDepth);
  void buildAccesses(TempSCoP &tempSCoP, const Region &CurRegion,
                     ScalarEvolution &SE, SmallVectorImpl<Loop*> &NestLoops);
  /// Create the SCoPStmt from a BasicBlock.
  SCoPStmt(SCoP &parent, TempSCoP &tempSCoP,
          const Region &CurRegion, BasicBlock &bb,
          SmallVectorImpl<Loop*> &NestLoops,
          SmallVectorImpl<unsigned> &Scatter,
          ScalarEvolution &SE);

  friend class SCoP;
public:
  // The loop IVS.
  std::vector<PHINode*> IVS;

  ~SCoPStmt();

  /// @brief Get the iteration domain of this SCoPStmt.
  ///
  /// @return The iteration domain of this SCoPStmt.
  polly_set *getDomain() const { return Domain; }

  /// @brief Get the scattering function of this SCoPStmt.
  ///
  /// @return The scattering function of this SCoPStmt.
  polly_map *getScattering() const { return Scattering; }
  void setScattering(polly_map *scattering) { Scattering = scattering; }

  /// @brief Get the BasicBlock represented by this SCoPStmt.
  ///
  /// @return The BasicBlock represented by this SCoPStmt.
  BasicBlock *getBasicBlock() const { return BB; }

  void setBasicBlock(BasicBlock *Block) { BB = Block; }

  typedef MemoryAccessVec::iterator memacc_iterator;
  memacc_iterator memacc_begin() { return MemAccs.begin(); }
  memacc_iterator memacc_end() { return MemAccs.end(); }

  unsigned getNumParams();
  unsigned getNumIterators();
  unsigned getNumScattering();

  SCoP *getParent() { return &Parent; }

  /// @brief Get the induction variable of the loop a given level.
  ///
  /// @param L The level of loop for the induction variable.
  ///
  /// @return The induction variable of the Loop at level L.
  PHINode *getIVatLevel(unsigned L);

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

  typedef std::vector<SCoPStmt*> StmtSet;
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
  explicit SCoP(TempSCoP &TempSCoP, LoopInfo &LI, ScalarEvolution &SE);

  /// @brief Check if a basic block is trivial.
  ///
  /// A trivial basic block does not contain any useful calculation. Therefore,
  /// it does not need to be represented as a polyhedral statement.
  ///
  /// @param BB The basic block to check
  /// @param tempSCoP TempSCoP returning further information regarding the SCoP.
  ///
  /// @return True if the basic block is trivial, otherwise false.
  static bool isTrivialBB(BasicBlock *BB, TempSCoP &tempSCoP);

  /// Build the SCoP and Statement with precalculate scop information.
  void buildSCoP(TempSCoP &TempSCoP, const Region &CurRegion,
                  // Loops in SCoP containing CurRegion
                  SmallVectorImpl<Loop*> &NestLoops,
                  // The scattering numbers
                  SmallVectorImpl<unsigned> &Scatter,
                  LoopInfo &LI, ScalarEvolution &SE);

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
  inline unsigned getScatterDim() const {
    unsigned maxScatterDim = 0;

    for (const_iterator SI = begin(), SE = end(); SI != SE; ++SI)
      maxScatterDim = std::max(maxScatterDim, (*SI)->getNumScattering());

    return maxScatterDim;
  }


  /// @brief Get the constraint on parameter of this SCoP.
  ///
  /// @return The constraint on parameter of this SCoP.
  inline polly_set *getContext() const { return Context; }

  /// @name Statements Iterators
  ///
  /// These iterators iterate over all statements of this SCoP.
  //@{
  typedef StmtSet::iterator iterator;
  typedef StmtSet::const_iterator const_iterator;

  iterator begin() { return Stmts.begin(); }
  iterator end()   { return Stmts.end();   }
  const_iterator begin() const { return Stmts.begin(); }
  const_iterator end()   const { return Stmts.end();   }

  typedef StmtSet::reverse_iterator reverse_iterator;
  typedef StmtSet::const_reverse_iterator const_reverse_iterator;

  reverse_iterator rbegin() { return Stmts.rbegin(); }
  reverse_iterator rend()   { return Stmts.rend();   }
  const_reverse_iterator rbegin() const { return Stmts.rbegin(); }
  const_reverse_iterator rend()   const { return Stmts.rend();   }
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
  explicit SCoPInfo() : RegionPass(ID), scop(0) {}
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

} //end namespace polly

#endif
