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
#include "llvm/ADT/FoldingSet.h"
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
class SCoPCnd : public FoldingSetNode {
  SCoPCnd(const SCoPCnd &);
  void operator=(const SCoPCnd &);
  /// FastID - A reference to an Interned FoldingSetNodeID for this node.
  /// The ScalarEvolution's BumpPtrAllocator holds the data.
  FoldingSetNodeIDRef FastID;

  // The SCoPCnd baseclass this node corresponds to
  const unsigned short SCoPCndType;

protected:
  SCoPCnd(const FoldingSetNodeIDRef ID, unsigned SCoPCndTy)
    : FastID(ID), SCoPCndType(SCoPCndTy) {}

public:
  /// Profile - FoldingSet support.
  void Profile(FoldingSetNodeID &ID) { ID = FastID; }

  unsigned getSCoPCndType() const { return SCoPCndType; }

  virtual void print(raw_ostream &OS) const = 0;
};

inline raw_ostream &operator<<(raw_ostream &OS, const SCoPCnd &C) {
  C.print(OS);
  return OS;
}

template<enum SCoPCndTypes CndType>
class SCoPConstCnd : public SCoPCnd {
  friend class SCoPCondition;
protected:
  explicit SCoPConstCnd() : SCoPCnd(FoldingSetNodeIDRef(), CndType) {}
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
  explicit SCoPBrCnd(const FoldingSetNodeIDRef ID, Value *Cmp, bool WhichSide)
    : SCoPCnd(ID, scopBrCnd), Cond(Cmp, WhichSide) {
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
template<enum SCoPCndTypes CndType>
class SCoPNAryCnd : public SCoPCnd {
  friend class SCoPCondition;
  const SCoPCnd *const *Operands;
  size_t NumOperands;
protected:
  explicit SCoPNAryCnd(const FoldingSetNodeIDRef ID,
    const SCoPCnd *const *O, size_t N)
    : SCoPCnd(ID, CndType), Operands(O), NumOperands(N) {
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
///
/// where
///   IDom(BB) is the immediately dominator of BB
///   InDomCond(BB, DomBB) is the the condition that from DomBB to BB.
///   and DomBB must dominate BB.
///
/// we could evaluate InDomCond(BB, DomBB) with the following rules:
///   InDomCond(BB, IDom(BB)) & InDomCond(IDom(BB), DomBB) and
///   InDomCond(BB, BB) = Always True
///
/// InDomCond(BB, IDom(BB)) could be evaluate by:
/// for each predBB in predecessor set of BB
///     Union(InDomPathCond(BB, predBB, IDomBB))
///
/// InDomPathCond(BB, predBB, IDomBB) means the condition construct by a path
/// of CFG that start from BB, to IDom(BB), and passing predBB, e.g.
///
/// BB0:
///     if(a > 0) goto BB1, else goto BB2
/// BB1:
///     if(b > 0) goto BB2, else goto BB3
/// in this example, we know IDom(BB2) = BB0, so we have
///   InDomPathCond(BB2, BB1, BB0) = (a > 0, true) & (b > 0, true) and
///   InDomPathCond(BB2, BB0, BB0) = (a > 0, false)
///
/// and InDomPathCond(BB, predBB, IDomBB) could be evaluate by:
///     CondOfEdge(BB, predBB) & InDomCond(predBB, IDom(BB))
///
/// CondOfEdge(BB, pred(BB) means the condition from pred(BB) to BB, e.g.
/// BB0:
///     if(a > 0) goto BB1, else goto BB2
/// in this example,
///     CondOfEdge(BB1, BB0) = (a > 0, true)
///     CondOfEdge(BB2, BB0) = (a > 0, false)
///
class SCoPCondition : public FunctionPass {
  typedef DenseMap<const BasicBlock*, const SCoPCnd*> CndMapTy;

  // Mapping BBs to its immediately condition
  CndMapTy BBtoInDomCond;
  // Allocator for SCoP conditions.
  BumpPtrAllocator SCoPCndAllocator;

  FoldingSet<SCoPCnd> UniqueSCoPCnds;

  SCoPTrueCnd TrueCnd;
  SCoPFalseCnd FalseCnd;

  /// Return the SCoPBrCnd corresponding to the Side of Br.
  ///
  const SCoPCnd *getBrCnd(BranchInst *Br, bool Side);
  const SCoPCnd *createBrCnd(Value *Cnd, bool Side) {
    // Do not create a new condition when we already have one
    FoldingSetNodeID ID;
    ID.AddInteger(scopBrCnd);
    ID.AddPointer(Cnd);
    ID.AddBoolean(Side);

    void *IP = 0;
    if (const SCoPCnd *C = UniqueSCoPCnds.FindNodeOrInsertPos(ID, IP)) {
      assert(0 && "Why we create the same br condition more than once?");
      return C;
    }
   // If there are no the same br condition exist, just create one
   SCoPCnd *C = new (SCoPCndAllocator) SCoPBrCnd(ID.Intern(SCoPCndAllocator),
                                                 Cnd, Side);
   UniqueSCoPCnds.InsertNode(C, IP);
   return C;
  }

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

  template<enum SCoPCndTypes CndType>
  const SCoPCnd *createNAryCnd(SmallVectorImpl<const SCoPCnd *> &Ops) {
    // Do not create a new condition when we already have one
    FoldingSetNodeID ID;
    ID.AddInteger(CndType);
    ID.AddInteger(Ops.size());
    for (unsigned i = 0, e = Ops.size(); i != e; ++i)
      ID.AddPointer(Ops[i]);

    void *IP = 0;
    if (const SCoPCnd *C = UniqueSCoPCnds.FindNodeOrInsertPos(ID, IP))
      return C;

    // If there are no the same nary condition exist, just create one
    const SCoPCnd **O = SCoPCndAllocator.Allocate<const SCoPCnd*>(Ops.size());
    std::uninitialized_copy(Ops.begin(), Ops.end(), O);
    SCoPCnd *C =
      new (SCoPCndAllocator) SCoPNAryCnd<CndType>(ID.Intern(SCoPCndAllocator),
                                                  O, Ops.size());
    UniqueSCoPCnds.InsertNode(C, IP);
    return C;
  }

  /// Try to reduce LHS and RHS to a one condition
  /// (not by create a new node to combining them)
  ///
  /// If LHS and RHS the opposite branch conditon?
  bool isOppBrCnd(const SCoPBrCnd *LHS, const SCoPBrCnd *RHS) const;

  //===--------------------------------------------------------------------===//
  DominatorTree *DT;

  const SCoPCnd *lookUpInDomCond(BasicBlock *BB) const {
    CndMapTy::const_iterator at = BBtoInDomCond.find(BB);
    // If the InDomCond already computed
    return at == BBtoInDomCond.end() ? 0 : at->second;
  }

  // Compute InDomcnd(BB, DomBB)
  const SCoPCnd *getInDomCnd(DomTreeNode *BB, DomTreeNode *DomBB);

  // Compute InDomcnd(BB, IDom(BB))
  const SCoPCnd *getInDomCnd(DomTreeNode *BB);

  // Compute CondOfEdge(BB, pred(BB)) & InDomCond(pred(BB), IDom(BB))
  const SCoPCnd *getInDomPathCnd(DomTreeNode *BB, DomTreeNode *PredBB);
  // Handle the backedge
  const SCoPCnd *getBEPathCnd(DomTreeNode *BB, DomTreeNode *PredBB);

  // Compute CondofEdge(BB, pred(BB))
  const SCoPCnd *getEdgeCnd(BasicBlock *SrcBB, BasicBlock *DstBB);

  const SCoPCnd *getCndForBB(BasicBlock *BB) {
    DomTreeNode *Node = DT->getNode(BB);
    return getInDomCnd(Node, DT->getRootNode());
  }

  void clear() {
    BBtoInDomCond.clear();
    UniqueSCoPCnds.clear();
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
