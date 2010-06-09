//===---------- SCoPConditon.cpp  - Extract conditons in CFG ------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Implement a pass that extrct the conditon information of CFGs.
//
//===----------------------------------------------------------------------===//

#include "polly/SCoPCondition.h"
#include "llvm/Support/CFG.h"
#include "llvm/Analysis/Dominators.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Assembly/Writer.h"
#include "llvm/Support/ErrorHandling.h"
#define DEBUG_TYPE "polly-scop-cond"
#include "llvm/Support/Debug.h"

using namespace llvm;
using namespace polly;

/// Some complexity sorting code from ScalarEvolution.cpp
namespace {
/// SCoPCndComplexityCompare - Return true if the complexity of the LHS is less
/// than the complexity of the RHS.  This comparator is used to canonicalize
/// expressions.
struct SCoPCndComplexityCompare {
  explicit SCoPCndComplexityCompare() {}
  bool operator()(const SCoPCnd *LHS, const SCoPCnd *RHS) const {
    if (LHS == RHS)
      return false;
    if (LHS->getSCoPCndType() != RHS->getSCoPCndType())
      return LHS->getSCoPCndType() < RHS->getSCoPCndType();

    // Now LHS and RHS have the same type
    // Check Branching condition.
    if (const SCoPBrCnd *LBr = dyn_cast<SCoPBrCnd>(LHS)) {
      const SCoPBrCnd *RBr = cast<SCoPBrCnd>(RHS);
      if (LBr->getCnd() != RBr->getCnd()) {
        // FIXME: How to compare the conditions?
        return LBr->getCnd() < RBr->getCnd();
      }
      DEBUG(LBr->print(dbgs()));
      DEBUG(dbgs()<< "\n");
      DEBUG(LBr->print(dbgs()));
      DEBUG(dbgs()<< "\n");
      // Branching with the same condition
      assert(LBr->getSide() != RBr->getSide()
        && "Why we get two same branching condition?");
      return LBr->getSide();
    }

    if (const SCoPLoopCnd *LLC = dyn_cast<SCoPLoopCnd>(LHS)) {
      const SCoPLoopCnd *RLC = cast<SCoPLoopCnd>(RHS);
      // Compare the entry condition
      if (operator()(LLC->getEntryCnd(), RLC->getEntryCnd()))
        return true;
      if (operator()(RLC->getEntryCnd(), LLC->getEntryCnd()))
        return false;
      // And the loop condition
      if (operator()(LLC->getLoopCnd(), RLC->getLoopCnd()))
        return true;
      if (operator()(RLC->getLoopCnd(), LLC->getLoopCnd()))
        return false;
      // Compare the loop
      return RLC->getLoop()->contains(LLC->getLoop());
    }


    // Or condition
    if (const SCoPOrCnd *LOr = dyn_cast<SCoPOrCnd>(LHS)) {
      const SCoPOrCnd *ROr = cast<SCoPOrCnd>(RHS);
      return SortNAryCnd(LOr, ROr);
    }
    // And condition
    if (const SCoPAndCnd *LAnd = dyn_cast<SCoPAndCnd>(LHS)) {
      const SCoPAndCnd *RAnd = cast<SCoPAndCnd>(RHS);
      return SortNAryCnd(LAnd, RAnd);
    }

    llvm_unreachable("UnSupport SCoPCond kind!");
    return false;
  }

  template<class CndType>
  bool SortNAryCnd(const CndType *LHS, const CndType *RHS) const {
    for (unsigned i = 0, e = LHS->getNumOperands(); i != e; ++i) {
      if (i >= RHS->getNumOperands())
        return false;
      if (operator()(LHS->getOperand(i), RHS->getOperand(i)))
        return true;
      if (operator()(RHS->getOperand(i), LHS->getOperand(i)))
        return false;
    }
    return LHS->getNumOperands() < RHS->getNumOperands();
  }
};
} // end namespace

static void GroupByComplexity(SmallVectorImpl<const SCoPCnd*> &Ops) {
  if (Ops.size() < 2) return;  // Noop
  if (Ops.size() == 2) {
    // This is the common case, which also happens to be trivially simple.
    // Special case it.
    if (SCoPCndComplexityCompare()(Ops[1], Ops[0]))
      std::swap(Ops[0], Ops[1]);
    return;
  }

  // Do the rough sort by complexity.
  std::stable_sort(Ops.begin(), Ops.end(), SCoPCndComplexityCompare());

  // Now that we are sorted by complexity, group elements of the same
  // complexity.  Note that this is, at worst, N^2, but the vector is likely to
  // be extremely short in practice.  Note that we take this approach because we
  // do not want to depend on the addresses of the objects we are grouping.
  for (unsigned i = 0; i < Ops.size() - 1; ++i) {
    const SCoPCnd *S = Ops[i];
    unsigned Complexity = S->getSCoPCndType();
    // If there are any objects of the same complexity and same value as this
    // one, group them.
    unsigned j = i + 1;
    while(j < Ops.size() && Ops[j]->getSCoPCndType() == Complexity) {
      if (Ops[j] == S) {
        // Remove the duplicate condition, because A & A = A, A | A = A
        Ops.erase(Ops.begin() + j);
        // Do not increase j since we had removed something
        continue;
      }
      ++j;
    }
  }
}

//===----------------------------------------------------------------------===//
void SCoPBrCnd::print(raw_ostream &OS) const {
  if (!Cond.getInt())
    OS << "!";
  WriteAsOperand(OS, Cond.getPointer(), false);
}

void SCoPLoopCnd::print(raw_ostream &OS) const {
  OS << "{" << *EntryCnd << "," << *LoopCnd << "}<";
  WriteAsOperand(OS, L->getHeader(), false);
  OS << ">";
}

const SCoPCnd *SCoPLoopCnd::evalAt(BasicBlock *BB, SCoPCondition &SC) const {
  if (!L->contains(BB))
    return getEntryCnd();

  return this;
}

//===----------------------------------------------------------------------===//
// Implementation of SCoPConditions

const SCoPCnd *SCoPCondition::getBrCnd(BranchInst *Br, bool Side) {
  // Return always true for unconditional branch.
  if (!Br->isConditional()) {
    assert(Side && "What is the false side of UnConditional Branch?");
    return getAlwaysTrue();
  }

  // Get the condition of Conditional Branch
  Value *Cnd = Br->getCondition();
  if (ConstantInt *ConstCnd = dyn_cast<ConstantInt>(Cnd)) {
    // Evaluate the constant condition.
    if (ConstCnd->isOne() == Side)
      return getAlwaysTrue();
    else
      return getAlwaysFalse();
  }

  return createBrCnd(Cnd, Side);
}

const SCoPCnd *SCoPCondition::createBrCnd(Value *Cnd, bool Side) {
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

const SCoPCnd *SCoPCondition::getOrCnd(const SCoPCnd *LHS,
                                       const SCoPCnd *RHS) {
  SmallVector<const SCoPCnd*, 2> Ops;
  Ops.push_back(LHS);
  Ops.push_back(RHS);
  return getOrCnd(Ops);
}

const SCoPCnd *SCoPCondition::getOrCnd(SmallVectorImpl<const SCoPCnd *> &Ops) {
  assert(!Ops.empty() && "Why ops is empty?");
  DEBUG(
    dbgs() << "dump or ops:\n";
    for (unsigned i = 0, e = Ops.size(); i != e; ++i) {
      dbgs() << "Cnd in or cnd: ";
      Ops[i]->print(dbgs());
      dbgs() << "\n";
    }
  );
  // Handle the trival condition
  if (Ops.size() == 1)
    return Ops[0];

  // Sort Ops by similarity
  GroupByComplexity(Ops);
  unsigned idx = 0;
  // False || Whatever = Whatever
  if (isa<SCoPFalseCnd>(Ops[idx]))
    Ops.erase(Ops.begin());

  // True || Whatever = True
  if (isa<SCoPTrueCnd>(Ops[idx]))
    return getAlwaysTrue();

  // Find the first br condition
  for (unsigned op_size = Ops.size();
      idx < op_size &&  Ops[idx]->getSCoPCndType() < scopBrCnd; ++idx)
    ;

  // Could us find any reducible branch conditions?
  while (idx < Ops.size() - 1 && Ops[idx + 1]->getSCoPCndType() < scopSWCnd) {
    if (isOppBrCnd(cast<SCoPBrCnd>(Ops[idx]), cast<SCoPBrCnd>(Ops[idx+1]))) {
      // Erase both conditions
      Ops.erase(Ops.begin() + idx, Ops.begin() + idx + 2);
      if (Ops.empty())
        // If all ops reduced, this is always true
        return getAlwaysTrue();
      // Do not increase idx;
      continue;
    }
    ++idx;
  }

  // Flatten the expression tree of Or Condition?
  for (unsigned i = 0, e = Ops.size(); i != e; ++i)
    // A || !A = True
    if (const SCoPOrCnd *Or = dyn_cast<SCoPOrCnd>(Ops[i])) {
      Ops.erase(Ops.begin() + i);
      Ops.append(Or->op_begin(), Or->op_end());
      return getOrCnd(Ops);
    }

  // ... How should us handle the And Condition?
  //

  // Dirty Hack:
  if (Ops.size() == 1)
    return Ops[0];


  // Create the condition
  return createNAryCnd<scopOrCnd>(Ops);
}

const SCoPCnd *SCoPCondition::getAndCnd(const SCoPCnd *LHS,
                                       const SCoPCnd *RHS) {
  SmallVector<const SCoPCnd*, 2> Ops;
  Ops.push_back(LHS);
  Ops.push_back(RHS);
  return getAndCnd(Ops);
}


const SCoPCnd *SCoPCondition::getAndCnd(SmallVectorImpl<const SCoPCnd *> &Ops) {
  assert(!Ops.empty() && "Why ops is empty?");
  // Handle the trival condition
  if (Ops.size() == 1)
    return Ops[0];
  // Sort Ops by similarity
  GroupByComplexity(Ops);
  unsigned idx = 0;
  // False && Whatever = False
  if (isa<SCoPFalseCnd>(Ops[idx]))
    return getAlwaysFalse();

  // True && Whatever = Whatever
  if (isa<SCoPTrueCnd>(Ops[idx]))
    Ops.erase(Ops.begin());

  // Find the first br condition
  for (unsigned op_size = Ops.size();
    idx < op_size &&  Ops[idx]->getSCoPCndType() < scopBrCnd; ++idx)
    ;

  // Could us find any reducible branch conditions?
  while (idx < Ops.size() - 1 && Ops[idx + 1]->getSCoPCndType() < scopSWCnd) {
    // A && !A = False
    if (isOppBrCnd(cast<SCoPBrCnd>(Ops[idx]), cast<SCoPBrCnd>(Ops[idx+1])))
      return getAlwaysFalse();

    ++idx;
  }

  // ... How should us handle the Or Condition?

  // Flatten the expression tree of and Condition?
  for (unsigned i = 0, e = Ops.size(); i != e; ++i)
    if (const SCoPAndCnd *Or = dyn_cast<SCoPAndCnd>(Ops[i])) {
      Ops.erase(Ops.begin() + i);
      Ops.append(Or->op_begin(), Or->op_end());
      return getAndCnd(Ops);
    }

  // Dirty Hack:
  if (Ops.size() == 1)
    return Ops[0];

  // Create the condition
  return createNAryCnd<scopAndCnd>(Ops);
}



const SCoPCnd *SCoPCondition::getLoopCnd(BasicBlock *Entry,
                                         const SCoPCnd *EntryCnd,
                                         const SCoPCnd *LoopCnd) {
  const Loop *L = LI->getLoopFor(Entry);
  assert(L && "Entry is not the entry of the loop!");

  return createLoopCnd(L, EntryCnd, LoopCnd);
}

const SCoPCnd *SCoPCondition::createLoopCnd(const Loop *L,
                                            const SCoPCnd *EntryCnd,
                                            const SCoPCnd *LoopCnd) {
  FoldingSetNodeID ID;
  ID.AddInteger(scopLoopCnd);
  ID.AddPointer(L);
  ID.AddPointer(EntryCnd);
  ID.AddPointer(LoopCnd);

  void *IP = 0;
  if (const SCoPCnd* C = UniqueSCoPCnds.FindNodeOrInsertPos(ID, IP)) {
    assert(0 && "Why we create the same loop condition twice?");
    return C;
  }

  SCoPCnd *C = new (SCoPCndAllocator) SCoPLoopCnd(ID.Intern(SCoPCndAllocator),
                                                  L, EntryCnd, LoopCnd);
  UniqueSCoPCnds.InsertNode(C, IP);
  return C;
}

bool SCoPCondition::isOppBrCnd(const SCoPBrCnd *BrLHS,
                               const SCoPBrCnd *BrRHS) const {
  // If both of them are according the same condition?
  if (BrLHS->getCnd() == BrRHS->getCnd())
    // If the two Conditions have difference side?
    if (BrLHS->getSide() != BrRHS->getSide())
      // The two conditions have the same cmp instruction, but branching to
      // difference side.
      return true;

  return false;
}

const SCoPCnd *SCoPCondition::getInDomCnd(DomTreeNode *Node,
                                          DomTreeNode *DomNode) {
  if (Node == DomNode)
    return getAlwaysTrue();

  DomTreeNode *IDom = Node->getIDom();

  assert(DT->dominates(DomNode, IDom)
         && "DomNode must dominate Node and its idom!");
  SmallVector<const SCoPCnd*, 4> AndCnds;
  if (IDom != DomNode) {
    // Compute InDomCond(IDom(BB), DomBB)
    const SCoPCnd *PredDomCnd = getInDomCnd(IDom, DomNode);
    AndCnds.push_back(PredDomCnd);
  }
  // // Compute InDomCond(BB, IDom(BB))
  const SCoPCnd *InDomCond = getInDomCnd(Node);
  AndCnds.push_back(InDomCond);
  // Compute InDomCond(BB, IDom(BB)) & InDomCond(IDom(BB), DomBB)
  const SCoPCnd *DomCond = getAndCnd(AndCnds);
  return DomCond->evalAt(Node->getBlock(), *this);
}

const SCoPCnd *SCoPCondition::getInDomCnd(DomTreeNode *Node) {
  BasicBlock *BB = Node->getBlock();
  if (const SCoPCnd *Cnd = lookUpInDomCond(BB))
    return Cnd;

  SmallVector<const SCoPCnd*, 4> UnionCnds;
  // Condition for backedges
  SmallVector<const SCoPCnd*, 4> BECnds;
  for (pred_iterator I = pred_begin(BB), E = pred_end(BB); I != E; ++I) {
    BasicBlock *PredBB = *I;
    DomTreeNode *PredNode = DT->getNode(PredBB);
    if (DT->dominates(Node, PredNode))
      // The Condition of paths that containing a backedge depends on
      // the conditions of paths not containing a backedge.
      BECnds.push_back(getBEPathCnd(Node, PredNode));
    else
      UnionCnds.push_back(getInDomPathCnd(Node, PredNode));
  }
  const SCoPCnd *InDomCond = getOrCnd(UnionCnds);
  //Compute backedge condition
  if (!BECnds.empty()) {
    const SCoPCnd *BECnd = getOrCnd(BECnds);
    DEBUG(dbgs() << "Backedge condition: ");
    DEBUG(BECnd->print(dbgs()));
    DEBUG(dbgs() << "\n");
    InDomCond = getLoopCnd(BB, InDomCond, BECnd);
  }

  // Remember the result
  BBtoInDomCond.insert(std::make_pair(BB, InDomCond));
  return InDomCond;
}


const SCoPCnd *SCoPCondition::getBEPathCnd(DomTreeNode *BB, DomTreeNode *PredBB) {
  assert(DT->dominates(BB, PredBB) && "Expect passing a backedge in!");
  const SCoPCnd *FECnd = getInDomCnd(PredBB, BB);
  const SCoPCnd *BECnd = getEdgeCnd(PredBB->getBlock(), BB->getBlock());
  DEBUG(dbgs() << "Pred to BB: ");
  DEBUG(BECnd->print(dbgs()));
  DEBUG(dbgs() << "\n");
  return getAndCnd(FECnd, BECnd);
}

const SCoPCnd *SCoPCondition::getInDomPathCnd(DomTreeNode *Node,
                                              DomTreeNode *PredBB) {
  DomTreeNode *IDom = Node->getIDom();

  // Compute Union(CondOfEdge(BB, pred(BB)) & InDomCond(pred(BB), IDom(BB)))
  assert(IDom && "Expect IDom not be null!");
  const SCoPCnd *EdgeCnd = getEdgeCnd(PredBB->getBlock(), Node->getBlock()),
                *PredInDomCond = getInDomCnd(PredBB, IDom);

  const SCoPCnd *InDomEdgeCond = getAndCnd(EdgeCnd, PredInDomCond);
  return InDomEdgeCond;
}

const SCoPCnd *SCoPCondition::getEdgeCnd(BasicBlock *SrcBB, BasicBlock *DstBB) {
  BranchInst *Br = dyn_cast<BranchInst>(SrcBB->getTerminator());
  // We only support BranchInst at this moment, so just return something if
  // the terminator is not a br.
  // Note that this will give us an WRONG analysis result.
  if (!Br)
    return getAlwaysFalse();

  assert((DstBB == Br->getSuccessor(0) || DstBB == Br->getSuccessor(1))
         && "DstBB is not the successor of SrcBB!");
  return getBrCnd(Br, DstBB == Br->getSuccessor(0));
}

void SCoPCondition::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<DominatorTree>();
  AU.addRequired<LoopInfo>();
  AU.setPreservesAll();
}

bool SCoPCondition::runOnFunction(Function &F) {
  DT = &getAnalysis<DominatorTree>();
  LI = &getAnalysis<LoopInfo>();
  return false;
}

void SCoPCondition::print(raw_ostream &OS, const Module *) const {
  SCoPCondition &SC = *const_cast<SCoPCondition*>(this);
  Function *F = DT->getRoot()->getParent();
  for (Function::iterator I = F->begin(), E = F->end(); I != E; ++I) {
    BasicBlock *BB = I;
    OS << "Condition of BB: " << BB->getName() << " is\n";
    SC.getCndForBB(BB, &F->getEntryBlock())->print(OS.indent(2));
    OS << "\n";
  }
}


char SCoPCondition::ID = 0;

static RegisterPass<SCoPCondition>
X("polly-scop-condition", "Polly - Extract conditions Constraint for each BB");

// Force linking
Pass *polly::createSCoPConditionPass() {
  return new SCoPCondition();
}
