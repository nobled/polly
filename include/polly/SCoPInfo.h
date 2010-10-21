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
// The pass creates a polyhedral description of the SCoPs detected by the SCoP
// detection derived from their LLVM-IR code.
//
// This represantation is shared among several tools in the polyhedral
// community, which are e.g. CLooG, Pluto, Loopo, Graphite.
//
//===----------------------------------------------------------------------===//

#ifndef POLLY_SCOP_INFO_H
#define POLLY_SCOP_INFO_H

#include "llvm/Analysis/RegionPass.h"

using namespace llvm;

namespace llvm {
  class SCEV;
  class ScalarEvolution;
  class SCEVAddRecExpr;
  class Loop;
  class LoopInfo;
}

struct isl_map;
struct isl_set;
struct isl_ctx;

namespace polly {

class SCoP;
class SCoPInfo;
class TempSCoP;

//===----------------------------------------------------------------------===//
/// @brief Represent memory accesses in statements.
class MemoryAccess {
  // DO NOT IMPLEMENT
  MemoryAccess(const MemoryAccess &);
  // DO NOT IMPLEMENT
  const MemoryAccess &operator=(const MemoryAccess &);

public:
  /// @brief The access type of a memory access
  ///
  /// There are three kind of access types:
  ///
  /// * A read access
  ///
  /// A certain set of memory locations are read and may be used for internal
  /// calculations.
  ///
  /// * A write access
  ///
  /// A certain set of memory locactions is definitely written. The old value is
  /// replaced by a newly calculated value. The old value is not read or used at
  /// all.
  ///
  /// * A may write access
  ///
  /// A certain set of memory locactions may be written. The memory location may
  /// contain a new value if there is actually a write or the old value may
  /// remain, if no write happens.
  enum AccessType {
    Read,
    Write,
    MayWrite
  };

private:
  isl_map *AccessRelation;
  enum AccessType Type;

  const Value* BaseAddr;

public:
  MemoryAccess(const Value *Base, AccessType AccType, isl_map *accRel)
    : AccessRelation(accRel), Type(AccType), BaseAddr(Base) {}

  ~MemoryAccess();

  /// @brief Is this a read memory access?
  bool isRead() const { return Type == MemoryAccess::Read; }

  isl_map *getAccessFunction() { return AccessRelation; }
  isl_map *getAccessFunction() const { return AccessRelation; }

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
/// @brief Statement of the SCoP
///
/// A SCoP statement represents an instruction in the SCoP.
///
/// It is further described by its iteration domain, its schedule and its data
/// accesses.
/// At the moment every statement represents a single basic block of LLVM-IR.
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
  ///       for (j = 0; j < 50; ++j)
  ///         S(i,j);
  ///
  /// 'S' is executed for different values of i and j. A vector of all
  /// induction variables around S (i, j) is called iteration vector.
  /// The domain describes the set of possible iteration vectors.
  ///
  /// In this case it is:
  ///
  ///     Domain: 0 <= i <= 100 + b
  ///             0 <= i <= j
  ///
  /// A pair of statment and iteration vector (S, (5,3)) is called statment
  /// iteration.
  isl_set *Domain;

  /// The scattering map describes the execution order of the statement.
  ///
  /// A statement and its iteration domain do not give any information about the
  /// order in time in which the different statement iterations are executed.
  /// This information is provided by the Scattering.
  ///
  /// The scattering maps every iteration of every statement into a multi
  /// dimensional scattering space.
  ///
  /// Example:
  ///
  /// <S,(5,4)>  may be mapped to (5,4) by this scattering:
  ///
  /// s0 = i
  /// s1 = j
  ///
  /// or to (9, 20) by this scattering:
  ///
  /// s0 = i + j
  /// s1 = 20
  ///
  /// The order statement iterations are executed is now defined by the
  /// scattering vectors they are mapped to. A statement iteration
  /// <A, (i, j, ..)> is executed before a statement iteration <B, (i', ..)>, if
  /// the scattering vector of A is lexicographic smaller than the scattering
  /// vector of B.
  isl_map *Scattering;

  /// The memory accesses of this statement.
  ///
  /// The only side effects models for a SCoP statement are its accesses to
  /// memory. Each statement may have an arbitrary large set of memory accesses.
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
  isl_set *getDomain() const { return Domain; }

  /// @brief Get the scattering function of this SCoPStmt.
  ///
  /// @return The scattering function of this SCoPStmt.
  isl_map *getScattering() const { return Scattering; }
  void setScattering(isl_map *scattering) { Scattering = scattering; }

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
/// @brief Static Control Part
///
/// A SCoP is the polyhedral representation of a control flow region detected
/// by the SCoP detection. It is generated by translating the LLVM-IR and
/// abstracting its effects.
///
/// A SCoP consists of a set of:
///
///   * A set of statements executed in the SCoP.
///
///   * A set of global parameters
///   Those parameters are scalar integer values, which are constant during
///   execution.
///
///   * A context
///   This context contains information about the values the parameters
///   can take and relations between different parameters.
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
  isl_set *Context;

  ///
  isl_ctx *ctx;

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
  inline isl_set *getContext() const { return Context; }

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
  isl_ctx *getCtx() const { return ctx; }
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
