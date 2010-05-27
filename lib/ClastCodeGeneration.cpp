//===- ClastCodeGeneration.cpp - Create LLVM IR from the CLooG AST --------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Create LLVM IR code using the ClastParser.
//
//===----------------------------------------------------------------------===//

#include "polly/ClastParser.h"
#include "polly/Support/GmpConv.h"
#include "polly/Support/IRHelper.h"
#include "polly/Support/BasicBlockEdge.h"
#include "polly/CLooG.h"
#include "llvm/ADT/APInt.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"


#define CLOOG_INT_GMP 1
#include "cloog/cloog.h"

#include <vector>

#ifdef _WINDOWS
#define snprintf _snprintf
#endif

using namespace polly;
using namespace llvm;

namespace polly {

typedef DenseMap<const Value*, Value*> ValueMapT;
typedef DenseMap<const char*, Value*> CharMapT;

struct codegenctx : cp_ctx {
  bb_edge edge;
  Pass *P;
  std::vector<succ_iterator> edges;
  ValueMapT ValueMap;
  codegenctx(succ_iterator e, Pass *p, SCoP *scop): edge(e), P(p), S(scop) {}
  std::vector<Value*> loop_ivs;
  Value *exprValue;
  CharMapT CharMap;
  SCoP *S;
  SCoPStmt *stmt;
  BasicBlock *BB;
  unsigned assignmentCount;
};

/// Create a loop on a specific edge
/// @returns An edge on which the loop body can be inserted.
succ_iterator createLoop(succ_iterator edge, Value *UB,
                         Pass *P, APInt stride, Value **IV) {
  BasicBlock *dest = *edge;
  BasicBlock *src = edge.getSource();
  const IntegerType *LoopIVType = IntegerType::getInt64Ty(src->getContext());

  // Create the loop header
  BasicBlock *loop = SplitEdge(src, dest, P);

  if (*succ_begin(loop) != dest) {
    loop = dest;
    dest = *succ_begin(dest);
  }

  loop->setName("polly.loop.header");
  TerminatorInst *oldTerminator = loop->getTerminator();
  PHINode *loopIV = PHINode::Create(LoopIVType, "polly.loopiv",
                                    loop->begin());
  Value *iv = static_cast<Value*>(loopIV);
  *IV = iv;

  ICmpInst *IS = new ICmpInst(oldTerminator, ICmpInst::ICMP_SGT, loopIV,
                              UB);
  BranchInst::Create(dest, loop, IS, loop);
  loopIV->addIncoming(loopIV, loop);
  loopIV->addIncoming(ConstantInt::get(LoopIVType, 0), src);
  oldTerminator->eraseFromParent();

  // Create the loop latch
  BasicBlock *latch = SplitEdge(loop, loop, P);
  latch->setName("polly.loop.latch");

  // Add loop induction variable increment
  stride.zext(64);
  Value *ConstOne = ConstantInt::get(src->getContext(), stride);
  Instruction* IncrementedIV = BinaryOperator::CreateAdd(loopIV, ConstOne);
  IncrementedIV->insertBefore(latch->begin());
  loopIV->replaceUsesOfWith(loopIV,IncrementedIV);

  // Return the loop body
  succ_iterator SI = succ_begin(loop);

  while (*SI != latch)
    ++SI;

  return SI;
}

succ_iterator copyBB(succ_iterator edge, BasicBlock *BB,
                     ValueMapT &VMap, Pass *P) {
  BasicBlock *dest = *edge;
  BasicBlock *BBCopy = SplitEdge(edge.getSource(), dest, P);

  if (*succ_begin(BBCopy) != dest) {
    BBCopy= dest;
    dest = *succ_begin(dest);
  }

  BBCopy->setName("polly_stmt." + BB->getName());
  // Loop over all instructions, and copy them over.
  BasicBlock::iterator it = BBCopy->begin();
  for (BasicBlock::const_iterator II = BB->begin(), IE = BB->end();
       II != IE; ++II) {
    if (II->isTerminator() || dyn_cast<PHINode>(II))
      continue;

    Instruction *NewInst = II->clone();
    VMap[II] = NewInst;

    for (Instruction::op_iterator UI = NewInst->op_begin(),
         UE = NewInst->op_end(); UI != UE; ++UI) {
      if (VMap.find(*UI) != VMap.end())
        NewInst->replaceUsesOfWith(*UI, VMap[*UI]);
    }

    it = BBCopy->getInstList().insert(it, NewInst);
    ++it;
  }

  return succ_begin(BBCopy);
}

class CPCodeGenerationActions : public CPActions {
  protected:
  void print(struct clast_assignment *a, int depth, cp_ctx *ctx) {
    codegenctx *cg_ctx = (codegenctx*) ctx;
    switch(ctx->dir) {
      case DFS_IN:
	eval(a->RHS, ctx);

        if (!a->LHS) {
          PHINode *PN = cg_ctx->stmt->IVS[cg_ctx->assignmentCount];
          cg_ctx->assignmentCount++;
          Value *V = PN;
          if (PN->getNumOperands() == 2)
            V = *(PN->use_begin());
          cg_ctx->ValueMap[V] = cg_ctx->exprValue;
        }
	break;
      case DFS_OUT:
	break;
    }
  }

  void print(struct clast_user_stmt *u, int depth, cp_ctx *ctx) {
    codegenctx *cg_ctx = (codegenctx*) ctx;
    // Actually we have a list of pointers here. be careful.
    SCoPStmt *stmt = (SCoPStmt *)u->statement->usr;
    BasicBlock *BB = stmt->getBasicBlock();
    switch(ctx->dir) {
      case DFS_IN:
        cg_ctx->BB = BB;
        cg_ctx->stmt = stmt;
        cg_ctx->assignmentCount = 0;
	break;
      case DFS_OUT:
        cg_ctx->edge = copyBB(cg_ctx->edge, BB, cg_ctx->ValueMap, cg_ctx->P);
        cg_ctx->BB = 0;
	break;
    }
  }

  void print(struct clast_block *b, int depth, cp_ctx *ctx) {
    switch(ctx->dir) {
      case DFS_IN:
	break;
      case DFS_OUT:
	break;
    }
  }

  void print(struct clast_for *f, int depth, cp_ctx *ctx) {
    codegenctx *cg_ctx = (codegenctx*) ctx;
    switch(ctx->dir) {
      case DFS_IN: {
	APInt stride = APInt_from_MPZ(f->stride);
        Value *IV;
	eval(f->LB, ctx);
        //Value *LB = cg_ctx->exprValue;
	eval(f->UB, ctx);
        Value *UB = cg_ctx->exprValue;
        cg_ctx->edge = createLoop(cg_ctx->edge, UB, cg_ctx->P,
                                  stride, &IV);
        cg_ctx->edges.push_back(succ_begin(cg_ctx->edge.getSource()));
        CharMapT *M = &cg_ctx->CharMap;
        (*M)[f->iterator] = IV;
        cg_ctx->loop_ivs.push_back(IV);
	break;
      }
      case DFS_OUT:
        cg_ctx->edge = cg_ctx->edges.back();
        cg_ctx->edges.pop_back();
        cg_ctx->loop_ivs.pop_back();
	break;
    }
  }

  void print(struct clast_equation *eq, int depth, cp_ctx *ctx) {
    llvm_unreachable("Clast equation not yet supported");
  }

  void print(struct clast_guard *g, int depth, cp_ctx *ctx) {
    llvm_unreachable("Clast guards not yet supported");
    switch(ctx->dir) {
      case DFS_IN:
	break;
      case DFS_OUT:
	break;
    }
  }

  void print(struct clast_root *r, int depth, cp_ctx *ctx) {}

  void print(clast_stmt *stmt, int depth, cp_ctx *ctx) {
    if	    (CLAST_STMT_IS_A(stmt, stmt_root))
      print((struct clast_root *)stmt, depth, ctx);
    else if (CLAST_STMT_IS_A(stmt, stmt_ass))
      print((struct clast_assignment *)stmt, depth, ctx);
    else if (CLAST_STMT_IS_A(stmt, stmt_user))
      print((struct clast_user_stmt *)stmt, depth, ctx);
    else if (CLAST_STMT_IS_A(stmt, stmt_block))
      print((struct clast_block *)stmt, depth, ctx);
    else if (CLAST_STMT_IS_A(stmt, stmt_for))
      print((struct clast_for *)stmt, depth, ctx);
    else if (CLAST_STMT_IS_A(stmt, stmt_guard))
      print((struct clast_guard *)stmt, depth, ctx);
  }

  void print(clast_name *e, cp_ctx *ctx) {
    if(ctx->dir == DFS_IN) {
      codegenctx *cg_ctx = (codegenctx*) ctx;
      CharMapT::iterator I = cg_ctx->CharMap.find(e->name);

      if (I != cg_ctx->CharMap.end())
        cg_ctx->exprValue = I->second;
      else
        llvm_unreachable("Value not found");

    }
  }

  void print(clast_term *e, cp_ctx *ctx) {
    codegenctx *cg_ctx = (codegenctx*) ctx;
    APInt a = APInt_from_MPZ(e->val);
    if (ctx->dir == DFS_IN) {
      a.zext(64);
      Value *ConstOne = ConstantInt::get((cg_ctx->edge)->getContext(), a);
      cg_ctx->exprValue = ConstOne;
    }
  }

  void print(clast_binary *e, cp_ctx *ctx) {
    llvm_unreachable("Binary expressions not yet supported");
    switch(ctx->dir) {
      case DFS_IN:
        break;
      case DFS_OUT:
	break;
    }
  }

  void print(clast_reduction *r, cp_ctx *ctx) {
    if (ctx->dir == DFS_IN) {
      assert((   r->type == clast_red_min
              || r->type == clast_red_max
              || r->type == clast_red_sum)
             && "Reduction type not supported");
      assert(r->n == 1
             && "Reductions with more than one element not supported");
      eval(r->elts[0], ctx);

      // TODO: Support reductions with more than one element.
      for (int i=1; i < r->n; ++i)
        eval(r->elts[i], ctx);
    }
  }

  void print(clast_expr *e, cp_ctx *ctx) {
    switch(e->type) {
      case clast_expr_name:
	print((struct clast_name *)e, ctx);
	break;
      case clast_expr_term:
	print((struct clast_term *)e, ctx);
	break;
      case clast_expr_bin:
	print((struct clast_binary *)e, ctx);
	break;
      case clast_expr_red:
	print((struct clast_reduction *)e, ctx);
	break;
    }
  }

  public:
  CPCodeGenerationActions(raw_ostream& strm) {
  }

  virtual void in(clast_stmt *s, int depth, cp_ctx *ctx) {
    ctx->dir = DFS_IN;
    print(s, depth, ctx);
  }

  virtual void out(clast_stmt *s, int depth, cp_ctx *ctx) {
    ctx->dir = DFS_OUT;
    print(s, depth, ctx);
  }

  virtual void in(clast_expr *e, cp_ctx *ctx) {
    ctx->dir = DFS_IN;
    print(e, ctx);
  }

  virtual void out(clast_expr *e, cp_ctx *ctx) {
    ctx->dir = DFS_OUT;
    print(e, ctx);
  }
};
}

namespace {
class ClastCodeGeneration : public RegionPass {
  Region *region;
  SCoP *S;
  DominatorTree *DT;

  public:
  static char ID;

  ClastCodeGeneration() : RegionPass(&ID) {}

  succ_iterator insertNewCodeBranch() {
    Region *region = const_cast<Region*>(&S->getRegion());
    BasicBlock *SEntry = *succ_begin(region->getEntry());
    BasicBlock *SExit;

    // Find the merge basic block after the region.
    for (pred_iterator PI = pred_begin(region->getExit()),
         PE = pred_end(region->getExit()); PI != PE; ++PI) {
      if (region->contains(*PI))
        SExit = *PI;
    }

    SplitEdge(region->getEntry(), SEntry, this);
    BasicBlock *branch = *succ_begin(region->getEntry());
    branch->setName("polly.new_code_branch");
    TerminatorInst *OldTermInst = branch->getTerminator();
    BranchInst::Create(*succ_begin(branch), SExit,
                       ConstantInt::getFalse(branch->getContext()), branch);
    OldTermInst->eraseFromParent();

    // Return the edge on which the new code will be inserted.
    for (succ_iterator SI = succ_begin(branch),
         SE = succ_end(branch); SI != SE; ++SI)
      if (*SI == SExit)
        return SI;

    return succ_end(branch);
  }

  bool runOnRegion(Region *R, RGPassManager &RGM) {
    region = R;
    S = getAnalysis<SCoPInfo>().getSCoP();
    DT = &getAnalysis<DominatorTree>();

    if (!S)
      return false;

    BasicBlock *newEntry = createSingleEntryEdge(R, this);

    for (SCoP::iterator SI = S->begin(), SE = S->end(); SI != SE; ++SI)
      if ((*SI)->getBasicBlock() == R->getEntry())
        (*SI)->setBasicBlock(newEntry);

    createSingleExitEdge(R, this);
    succ_iterator edge = insertNewCodeBranch();

    CLooG C = CLooG(S);
    CPCodeGenerationActions cpa = CPCodeGenerationActions(dbgs());
    ClastParser cp = ClastParser(cpa);
    codegenctx ctx (edge, this, S);

    cp.parse(C.getClast(), &ctx);
    dbgs() << "\n";
    return false;
  }

  void print(raw_ostream &OS, const Module *) const {
  }

  virtual void getAnalysisUsage(AnalysisUsage &AU) const {
    AU.addRequired<SCoPInfo>();
    AU.addRequired<DominatorTree>();
    AU.addPreserved<DominatorTree>();
  }
};
}

char ClastCodeGeneration::ID = 1;

static RegisterPass<ClastCodeGeneration>
Z("polly-codegen", "Polly - Create LLVM-IR from the polyhedral "
                         "information");

RegionPass* polly::createClastCodeGenerationPass() {
	return new ClastCodeGeneration();
}
