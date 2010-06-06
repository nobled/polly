//===------ polly/SCoPCondition.h - Extract conditons in CFG -----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Extract conditions in CFG, so we could apply them to iterate domain.
//
//===----------------------------------------------------------------------===//
#ifndef POLLY_SCOP_CONDITION_H
#define POLLY_SCOP_CONDITION_H

#include "llvm/Pass.h"
#include "llvm/Instructions.h"
#include "llvm/Analysis/Dominators.h"
#include "llvm/ADT/PointerIntPair.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/Allocator.h"
using namespace llvm;

namespace polly {
class SCoPCondition;

//===----------------------------------------------------------------------===//
/// Condition types
enum SCoPCndTypes {
  scopFalseCnd, scopTrueCnd, scopBrCnd, scopSWCnd, scopOrCnd, scopAndCnd
};

//===----------------------------------------------------------------------===//
// @brief Class to express and help to evaluate conditions in SCoPs.
class SCoPCnd {
  SCoPCnd(const SCoPCnd &);
  void operator=(const SCoPCnd &);

  // The SCoPCnd baseclass this node corresponds to
  const unsigned short SCoPCndType;

protected:
  SCoPCnd(unsigned SCoPCndTy) : SCoPCndType(SCoPCndTy) {}

public:
  unsigned getSCoPCndType() const { return SCoPCndType; }

  virtual void print(raw_ostream &OS) const = 0;
};

inline raw_ostream &operator<<(raw_ostream &OS, const SCoPCnd &C) {
  C.print(OS);
  return OS;
}

template<unsigned CndType>
class SCoPConstCnd : public SCoPCnd {
  friend class SCoPCondition;
protected:
  explicit SCoPConstCnd() : SCoPCnd(CndType) {}
public:
  void print(raw_ostream &OS) const {
    OS << (isa<SCoPConstCnd<scopTrueCnd> >(this) ? "True" : "False");
  }

  static inline bool classof(const SCoPCnd *C) {
    return C->getSCoPCndType() == CndType;
  }
  template<unsigned OtherType>
  static inline bool classof(const SCoPConstCnd<OtherType> *C) {
    return OtherType == CndType;
  }
};

// Instance Type of the Always True and Always False Condition
typedef SCoPConstCnd<scopTrueCnd> SCoPTrueCnd;
typedef SCoPConstCnd<scopFalseCnd> SCoPFalseCnd;

//===----------------------------------------------------------------------===//
// @brief Class to express Branch condition for BB.
class SCoPBrCnd : public SCoPCnd {
  friend class SCoPCondition;

  // The compare instruction and if this Comparison true of false
  // TODO: how to express Switch?
  PointerIntPair<Value *, 1, bool> Cond;

protected:
  explicit SCoPBrCnd(Value *Cmp, bool WhichSide)
    : SCoPCnd(scopBrCnd), Cond(Cmp, WhichSide) {
    assert(Cmp && "Cmp can not be null!");
  }

public:
  // What the branching condition
  const Value *getCnd() const { return Cond.getPointer(); }
  // Branch to which side?
  bool getSide() const { return Cond.getInt(); }

  void print(raw_ostream &OS) const;

  /// Methods for support type inquiry through isa, cast, and dyn_cast:
  static inline bool classof(const SCoPBrCnd *C) { return true; }
  static inline bool classof(const SCoPCnd *C) {
    return C->getSCoPCndType() == scopBrCnd;
  }
};

//===----------------------------------------------------------------------===//
// @brief Class to provide common functionality for n'ary operators. e.g. Or
//        operator for condtions or And operator for conditions. Note that
//        Or and And is also commutative.
template<unsigned CndType>
class SCoPNAryCnd : public SCoPCnd {
  friend class SCoPCondition;
  const SCoPCnd *const *Operands;
  size_t NumOperands;
protected:
  explicit SCoPNAryCnd(const SCoPCnd *const *O, size_t N)
    : SCoPCnd(CndType), Operands(O), NumOperands(N) {
    assert(NumOperands > 1 && "Expect more than 1 operands!");
  }
public:
  /// @name Operands of this Condition
  //{
  typedef const SCoPCnd *const *op_iterator;
  op_iterator op_begin() const { return Operands; }
  op_iterator op_end() const { return Operands + NumOperands; }

  size_t getNumOperands() const { return NumOperands; }
  const SCoPCnd *getOperand(unsigned i) const {
    assert(i < NumOperands && "Operand index out of range!");
    return Operands[i];
  }
  //}

  void print(raw_ostream &OS) const {
    OS << "(" << *Operands[0];
    for (unsigned i = 1; i < NumOperands; ++i)
      OS << (CndType == scopOrCnd? ",||," : ",&&,") << *Operands[i];
    OS << ")";
  }

  static inline bool classof(const SCoPCnd *C) {
    return C->getSCoPCndType() == CndType;
  }
  template<unsigned OtherType>
  static inline bool classof(const SCoPConstCnd<OtherType> *C) {
    return OtherType == CndType;
  }
};

// Instance Type of the And Condition and Or Condition
typedef SCoPNAryCnd<scopOrCnd> SCoPOrCnd;
typedef SCoPNAryCnd<scopAndCnd> SCoPAndCnd;

//===----------------------------------------------------------------------===//
/// @brief The pass to extract all condition constraints for BBs.
///
/// The condition of a given BB Cond(BB) could be evaluate by:
///   InDomCond(BB, entry) & Cond(entry)
/// and we have Cond(entry) = Always True
/// where
///   IDom(BB) is the immediately dominator of BB
///   InDomCond(BB, DomBB) is the the condition that from DomBB to BB.
///   and DomBB must dominate BB.
/// we could evaluate it by:
///   InDomCond(BB, IDom(BB)) & InDomCond(IDom(BB), DomBB) and
///   InDomCond(BB, BB) = Always True
/// InDomCond(BB, IDom(BB)) could be evaluate by:
///   Union(CondOfEdge(BB, pred(BB)) & InDomCond(pred(BB), IDom(BB)))
///
class SCoPCondition : public FunctionPass {
  typedef DenseMap<const BasicBlock*, const SCoPCnd*> CndMapTy;

  // Mapping BBs to its immediately condition
  CndMapTy BBtoInDomCond;
  // Allocator for SCoP conditions.
  BumpPtrAllocator SCoPCndAllocator;

  SCoPTrueCnd TrueCnd;
  SCoPFalseCnd FalseCnd;

  /// Return the SCoPBrCnd corresponding to the Side of Br.
  ///
  const SCoPCnd *getBrCnd(BranchInst *Br, bool Side);

  /// Return the Constant conditions
  ///
  const SCoPCnd *getAlwaysTrue() const { return &TrueCnd; }
  const SCoPCnd *getAlwaysFalse() const { return &FalseCnd; }

  /// Return the combine conditions
  ///
  const SCoPCnd *getOrCnd(const SCoPCnd *LHS, const SCoPCnd *RHS);
  const SCoPCnd *getOrCnd(SmallVectorImpl<const SCoPCnd *> &Ops);
  const SCoPCnd *getAndCnd(const SCoPCnd *LHS, const SCoPCnd *RHS);
  const SCoPCnd *getAndCnd(SmallVectorImpl<const SCoPCnd *> &Ops);

  /// Try to reduce LHS and RHS to a one condition
  /// (not by create a new node to combining them)
  ///
  /// If LHS and RHS the opposite branch conditon?
  bool isOppBrCnd(const SCoPBrCnd *LHS, const SCoPBrCnd *RHS) const;

  /// Get the immediately condition for BB
  const SCoPCnd *getImmCndFor(BasicBlock *BB) const {
    CndMapTy::const_iterator at = BBtoInDomCond.find(BB);
    return at == BBtoInDomCond.end() ? 0 : at->second;
  }

  const SCoPCnd *getOrCreateImmCndFor(BasicBlock *BB) {
    if (const SCoPCnd *C = getImmCndFor(BB))
      return C;

    // The BB is unreachable by default
    BBtoInDomCond[BB] = getAlwaysFalse();
    return getAlwaysFalse();
  }

  //===--------------------------------------------------------------------===//
  DominatorTree *DT;

  const SCoPCnd *lookUpInDomCond(BasicBlock *BB) const {
    CndMapTy::const_iterator at = BBtoInDomCond.find(BB);
    // If the InDomCond already computed
    return at == BBtoInDomCond.end() ? 0 : at->second;
  }
  const SCoPCnd *getInDomCnd(DomTreeNode *BB, DomTreeNode *DomBB);
  const SCoPCnd *getInDomCnd(DomTreeNode *BB);

  const SCoPCnd *getEdgeCnd(BasicBlock *SrcBB, BasicBlock *DstBB);

  const SCoPCnd *getCndForBB(BasicBlock *BB) {
    DomTreeNode *Node = DT->getNode(BB);
    return getInDomCnd(Node, DT->getRootNode());
  }

  void clear() {
    BBtoInDomCond.clear();
    SCoPCndAllocator.Reset();
  }

public:
  static char ID;
  explicit SCoPCondition() : FunctionPass(&ID), DT(0) {}
  ~SCoPCondition() { clear(); }
  /// @name FunctionPass interface
  //@{
  virtual void getAnalysisUsage(AnalysisUsage &AU) const;
  virtual void releaseMemory() { clear(); }
  virtual bool runOnFunction(Function &F);
  virtual void print(raw_ostream &OS, const Module *) const;
  //@}
};

Pass *createSCoPConditionPass();

} // end namespace polly

#endif
