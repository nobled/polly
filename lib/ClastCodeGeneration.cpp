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
#include "polly/CLooG.h"
#include "llvm/ADT/APInt.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Support/Debug.h"

#define CLOOG_INT_GMP 1
#include "cloog/cloog.h"

#ifdef _WINDOWS
#define snprintf _snprintf
#endif

using namespace polly;
using namespace llvm;

namespace polly {

class CPCodeGenerationActions : public CPActions {
  private:
    raw_ostream *ost;

  void indent(int depth) {
    for(int i=0;i<depth;i++)
      *ost << " ";
  }

  protected:
  void print(struct clast_assignment *a, int depth, cp_ctx *ctx) {
    switch(ctx->dir) {
      case DFS_IN:
        indent(depth);
	if(a->LHS)
	  *ost << a->LHS << "=";
	  eval(a->RHS, ctx);
	  *ost << "\n";
	break;
      case DFS_OUT:
	break;
    }
  }

  void print(struct clast_user_stmt *u, int depth, cp_ctx *ctx) {
    // Actually we have a list of pointers here. be careful.
    BasicBlock *BB = (BasicBlock*)u->statement->usr;
    switch(ctx->dir) {
      case DFS_IN:
	indent(depth);
	*ost << BB->getNameStr() << " {\n";
	break;
      case DFS_OUT:
	indent(depth);
	*ost << "}\n";
	break;
    }
  }

  void print(struct clast_block *b, int depth, cp_ctx *ctx) {
    switch(ctx->dir) {
      case DFS_IN:
	indent(depth);
	*ost << "{\n";
	break;
      case DFS_OUT:
	indent(depth);
	*ost << "}\n";
	break;
    }
  }

  void print(struct clast_for *f, int depth, cp_ctx *ctx) {
    switch(ctx->dir) {
      case DFS_IN:
	indent(depth);
	*ost << "for (" << f->iterator <<"=";
	eval(f->LB, ctx);
	*ost << ";";
	*ost << f->iterator <<"<=";
	eval(f->UB, ctx);
	*ost << ";";
	*ost << f->iterator << "+=";
	APInt_from_MPZ(f->stride).print(*ost, false);
	*ost << ") {\n";
	break;
      case DFS_OUT:
	indent(depth);
	*ost << "}\n";
	break;
    }
  }

  void print(struct clast_equation *eq, int depth, cp_ctx *ctx) {
    eval(eq->LHS, ctx);
    if (eq->sign == 0)
      *ost << "==";
    else if (eq->sign > 0)
      *ost << ">=";
    else
      *ost << "<=";
    eval(eq->RHS, ctx);
  }

  void print(struct clast_guard *g, int depth, cp_ctx *ctx) {
    switch(ctx->dir) {
      case DFS_IN:
	indent(depth);
	*ost << "if (";

	print(&(g->eq[0]), depth, ctx);
	if (g->n > 1)
	  for(int i=1;i<g->n;i++) {
	    *ost << " && ";
	    print(&(g->eq[i]), depth, ctx);
	  }

	*ost << ") {\n";
	break;
      case DFS_OUT:
	indent(depth);
	*ost << "}\n";
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
    if(ctx->dir == DFS_IN)
      *ost << e->name;
  }

  void print(clast_term *e, cp_ctx *ctx) {
    APInt a = APInt_from_MPZ(e->val);
    if(e->var) {
      static int brkt;
      brkt = (e->var->type == clast_expr_red) &&
             ((struct clast_reduction*) e->var)->n > 1;
      if (ctx->dir == DFS_IN) {
        if (a == 1)
          ;
        else if (a == -1)
          *ost << "-";
        else {
          a.print(*ost, true);
          *ost << " *";
        }
        if (brkt)
          *ost << "(";

      }
      else {
       if (brkt)
         *ost << ")";
      }
    } else
      a.print(*ost, true);
  }

  void print(clast_binary *e, cp_ctx *ctx) {
    switch(ctx->dir) {
      case DFS_IN:
        break;
      case DFS_OUT:
	break;
    }
  }

  void print_red_name(clast_reduction *r, cp_ctx *ctx) {
    switch(r->type) {
      case clast_red_min:
	*ost << "min(";
	break;
      case clast_red_max:
	*ost << "max(";
	break;
      case clast_red_sum:
	*ost << "sum(";
    }
  }

  void print(clast_reduction *r, cp_ctx *ctx) {
    if (r->n > 1) {
      if (ctx->dir == DFS_IN)
        print_red_name(r, ctx);
      else
        *ost << ")";
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
    ost = &strm;
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


  /// Create a loop on a specific edge
  /// @returns An edge on which the loop body can be inserted.
  succ_iterator createLoop(succ_iterator edge, APInt NumLoopIterations) {
    BasicBlock *dest = *edge;
    BasicBlock *src = edge.getSource();
    const IntegerType *LoopIVType = IntegerType::getInt64Ty(src->getContext());

    // Create the loop header
    BasicBlock *loop = SplitEdge(src, dest, this);

    if (*succ_begin(loop) != dest) {
      loop = dest;
      dest = *succ_begin(dest);
    }

    loop->setName("polly.loop.header");
    TerminatorInst *oldTerminator = loop->getTerminator();
    PHINode *loopIV = PHINode::Create(LoopIVType, "polly.loopiv",
                                      loop->begin());

    ICmpInst *IS = new ICmpInst(oldTerminator, ICmpInst::ICMP_SGT, loopIV,
                                ConstantInt::get(src->getContext(),
                                                 NumLoopIterations));
    BranchInst::Create(dest, loop, IS, loop);
    loopIV->addIncoming(loopIV, loop);
    loopIV->addIncoming(ConstantInt::get(LoopIVType, 0), src);
    oldTerminator->eraseFromParent();

    // Create the loop latch
    BasicBlock *latch = SplitEdge(loop, loop, this);
    latch->setName("polly.loop.latch");

    // Add loop induction variable increment
    Value *ConstOne = ConstantInt::get(LoopIVType, 1);
    Instruction* IncrementedIV = BinaryOperator::CreateAdd(loopIV, ConstOne);
    IncrementedIV->insertBefore(latch->begin());
    loopIV->replaceUsesOfWith(loopIV,IncrementedIV);

    // Return the loop body
    succ_iterator SI = succ_begin(loop);

    while (*SI != latch)
      ++SI;

    return SI;
   }

  bool runOnRegion(Region *R, RGPassManager &RGM) {
    region = R;
    S = getAnalysis<SCoPInfo>().getSCoP();
    DT = &getAnalysis<DominatorTree>();

    if(!S)
      return false;

    createSingleEntryEdge(R, this);
    createSingleExitEdge(R, this);
    succ_iterator edge = insertNewCodeBranch();
    APInt NumLoopIterations(64, 2047);
    edge = createLoop(edge, NumLoopIterations);
    edge = createLoop(edge, NumLoopIterations);

    CLooG C = CLooG(S);
    //CPCodeGenerationActions cpa = CPCodeGenerationActions(dbgs());
    //ClastParser cp = ClastParser(cpa);
    //cp_ctx ctx;

    //cp.parse(C.getClast(), &ctx);
    //OS << "\n";
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
