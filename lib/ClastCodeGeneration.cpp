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
#include "polly/LinkAllPasses.h"
#include "polly/Support/GmpConv.h"
#include "polly/Support/IRHelper.h"
#include "polly/CLooG.h"
#include "llvm/ADT/APInt.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

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
  void print(struct clast_assignment *a, cp_ctx *ctx) {
    switch(ctx->dir) {
      case DFS_IN:
        indent(ctx->depth);
	if(a->LHS)
	  *ost << a->LHS << "=";
	  eval(a->RHS, *ctx);
	  *ost << "\n";
	break;
      case DFS_OUT:
	break;
    }
  }

  void print(struct clast_user_stmt *u, cp_ctx *ctx) {
    // Actually we have a list of pointers here. be careful.
    BasicBlock *BB = (BasicBlock*)u->statement->usr;
    switch(ctx->dir) {
      case DFS_IN:
	indent(ctx->depth);
	*ost << BB->getNameStr() << " == S" << u->statement->number << " {\n";
	break;
      case DFS_OUT:
	indent(ctx->depth);
	*ost << "}\n";
	break;
    }
  }

  void print(struct clast_block *b, cp_ctx *ctx) {
    switch(ctx->dir) {
      case DFS_IN:
	indent(ctx->depth);
	*ost << "{\n";
	break;
      case DFS_OUT:
	indent(ctx->depth);
	*ost << "}\n";
	break;
    }
  }

  void print(struct clast_for *f, cp_ctx *ctx) {
    switch(ctx->dir) {
      case DFS_IN:
	indent(ctx->depth);
	*ost << "for (" << f->iterator <<"=";
	eval(f->LB, *ctx);
	*ost << ";";
	*ost << f->iterator <<"<=";
	eval(f->UB, *ctx);
	*ost << ";";
	*ost << f->iterator << "+=";
	APInt_from_MPZ(f->stride).print(*ost, false);
	*ost << ") {\n";
	break;
      case DFS_OUT:
	indent(ctx->depth);
	*ost << "}\n";
	break;
    }
  }

  void print(struct clast_equation *eq, cp_ctx *ctx) {
    eval(eq->LHS, *ctx);
    if (eq->sign == 0)
      *ost << "==";
    else if (eq->sign > 0)
      *ost << ">=";
    else
      *ost << "<=";
    eval(eq->RHS, *ctx);
  }

  void print(struct clast_guard *g, cp_ctx *ctx) {
    switch(ctx->dir) {
      case DFS_IN:
	indent(ctx->depth);
	*ost << "if (";

	print(&(g->eq[0]), ctx);
	if (g->n > 1)
	  for(int i=1;i<g->n;i++) {
	    *ost << " && ";
	    print(&(g->eq[i]), ctx);
	  }

	*ost << ") {\n";
	break;
      case DFS_OUT:
	indent(ctx->depth);
	*ost << "}\n";
	break;
    }
  }

  void print(struct clast_root *r, cp_ctx *ctx) {}

  void print(clast_stmt *stmt, cp_ctx *ctx) {
    if	    (CLAST_STMT_IS_A(stmt, stmt_root))
      print((struct clast_root *)stmt, ctx);
    else if (CLAST_STMT_IS_A(stmt, stmt_ass))
      print((struct clast_assignment *)stmt, ctx);
    else if (CLAST_STMT_IS_A(stmt, stmt_user))
      print((struct clast_user_stmt *)stmt, ctx);
    else if (CLAST_STMT_IS_A(stmt, stmt_block))
      print((struct clast_block *)stmt, ctx);
    else if (CLAST_STMT_IS_A(stmt, stmt_for))
      print((struct clast_for *)stmt, ctx);
    else if (CLAST_STMT_IS_A(stmt, stmt_guard))
      print((struct clast_guard *)stmt, ctx);
  }

  void print(clast_name *e, cp_ctx *ctx) {
    if(ctx->dir == DFS_IN)
      *ost << e->name;
  }

  void print(clast_term *e, cp_ctx *ctx) {
    clast_expr *v = e->var;
    static APInt a;

    switch(ctx->dir) {
      case DFS_IN:
	a = APInt_from_MPZ(e->val);
	if(a != 1) {
	  if (v) *ost << "(";
	  a.print(*ost, false);
	  if (v) *ost << " * ";
	}
	break;
      case DFS_OUT:
	if(a != 1) {
	  if (v) *ost << ")";
	}
	break;
    }
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
	*ost << " min(";
	break;
      case clast_red_max:
	*ost << " max(";
	break;
      case clast_red_sum:
	*ost << " sum(";
    }
  }

  void print(clast_reduction *r, cp_ctx *ctx) {
    switch(ctx->dir) {
      case DFS_IN:
	if (r->n <= 1)
	  return;

	if (r->n > 1)
	  print_red_name(r, ctx);
	else
	  *ost << " ";

	eval(r->elts[0], *ctx);

	for (int i=1; i<r->n; ++i) {
	  *ost << ",";
	  eval(r->elts[i], *ctx);
	}

	break;
      case DFS_OUT:
	if (r->n > 1)
	  *ost << ")";
	break;
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

  virtual void in(clast_stmt *s, cp_ctx *ctx) {
    ctx->dir = DFS_IN;
    print(s, ctx);
  }

  virtual void out(clast_stmt *s, cp_ctx *ctx) {
    ctx->dir = DFS_OUT;
    print(s, ctx);
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

    for (pred_iterator PI = pred_begin(region->getExit()),
         PE = pred_end(region->getExit()); PI != PE; ++PI) {
      if (region->contains(*PI))
        SExit = *PI;
    }

    SplitEdge(region->getEntry(), SEntry, this);
    BasicBlock *branch = *succ_begin(region->getEntry());
    TerminatorInst *oldT = branch->getTerminator();
    BranchInst::Create(*succ_begin(branch), SExit,
                       ConstantInt::getFalse(branch->getContext()), branch);
    oldT->eraseFromParent();

    //TODO: Insert PHI nodes for every Value used outside of region

    for (succ_iterator SI = succ_begin(branch),
         SE = succ_end(branch); SI != SE; ++SI)
      if (*SI == SExit)
        return SI;
    return succ_end(branch);
  }

  void createLoop(succ_iterator edge) {
    BasicBlock *dest = *edge;
    BasicBlock *src = edge.getSource();
    BasicBlock *loop = SplitEdge(src, dest, this);
    TerminatorInst *oldT = loop->getTerminator();
    //IRBuilder LBuilder(loop);
    PHINode *NPN =
        PHINode::Create(IntegerType::getInt64Ty(loop->getContext()), "asph",
                        loop->begin());
    BranchInst::Create(dest, loop,
                       ConstantInt::getFalse(loop->getContext()), loop);
    NPN->addIncoming(NPN, loop);
    NPN->addIncoming(
      ConstantInt::get(IntegerType::getInt64Ty(loop->getContext()), 0, false),
      src);

    //BasicBlock *latch = SplitEdge(loop, loop, this);
    oldT->eraseFromParent();
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
    createLoop(edge);

    CLooG C = CLooG(S);
    //CPCodeGenerationActions cpa = CPCodeGenerationActions(dbgs());
    //ClastParser cp = ClastParser(&cpa);
    //cp.parse(C.getClast());
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
