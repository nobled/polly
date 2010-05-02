//===- ClastParser.cpp - Utilities to parse the CLooG AST.  ---------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Parse a CLooG abstract syntax tree (AST).
//
//===----------------------------------------------------------------------===//

#include "polly/ClastParser.h"
#include "polly/LinkAllPasses.h"
#include "polly/Support/GmpConv.h"
#include "llvm/ADT/APInt.h"
#include "polly/CLooG.h"

#define CLOOG_INT_GMP 1
#include "cloog/cloog.h"

#ifdef _WINDOWS
#define snprintf _snprintf
#endif

using namespace polly;
using namespace llvm;

namespace polly {

ClastParser::ClastParser(CPActions *actions) {
  act = actions;
}

void ClastParser::parse(clast_stmt *root) {
  cp_ctx ctx;
  ctx.dir   = DFS_IN;
  ctx.depth = 0;
  ctx.pred  = NULL;

  if (root)
    dfs(root, ctx);
}

// Traverse the clast with Depth-First-Search and
// fire in-/out- events when entering/leaving a node.
void ClastParser::dfs(clast_stmt *stmt, cp_ctx ctx) {
  clast_stmt *next = NULL;

  // Before child-dfs.
  act->in(stmt, &ctx);

  // Select next child.
  if      (CLAST_STMT_IS_A(stmt, stmt_user))
    next = ((struct clast_user_stmt*)stmt)->substitutions;
  else if (CLAST_STMT_IS_A(stmt, stmt_block))
    next = ((struct clast_block*) stmt)->body;
  else if (CLAST_STMT_IS_A(stmt, stmt_for))
    next = ((struct clast_for*) stmt)->body;
  else if (CLAST_STMT_IS_A(stmt, stmt_guard))
    next = ((struct clast_guard*) stmt)->then;

  // DFS to child.
  if(next) {
    cp_ctx nctx = ctx;
    ++(nctx.depth);
    dfs(next, nctx);
  }

  // After child-dfs.
  act->out(stmt, &ctx);

  // Continue with sibling.
  if(stmt->next)
    dfs(stmt->next, ctx);
}

void CPActions::eval(clast_expr *expr, cp_ctx ctx) {
  int n = 1;
  clast_expr* next =  NULL;

  // Before child-dfs.
  in(expr, &ctx);

  // Select next child.
  switch(expr->type) {
    case clast_expr_term:
      next = ((clast_term *)expr)->var;
      break;
    case clast_expr_bin:
      next = ((clast_binary *)expr)->LHS;
      break;
    case clast_expr_red: {
	clast_reduction *r = (clast_reduction *)expr;
	n = r->n;
	next = r->elts[0];
      }
      break;
    case clast_expr_name:
      break;
  }

  // DFS to child.
  cp_ctx nctx = ctx;
  ++(nctx.depth);
  for (int i=0; i < n; i++) {
    if(next) {
      eval(&(next[i]), nctx);
    }
  }

  // After child-dfs.
  out(expr, &ctx);
}

class CPPrinterActions : public CPActions {
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
  CPPrinterActions(raw_ostream& strm) {
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

class ClastPrinter : public RegionPass {
  Region *region;
  SCoP *S;

  public:
  static char ID;

  ClastPrinter() : RegionPass(&ID) {}

  bool runOnRegion(Region *R, RGPassManager &RGM) {
    region = R;
    S = getAnalysis<SCoPInfo>().getSCoP();

    if(!S)
      return false;

    return false;
  }

  void print(raw_ostream &OS, const Module *) const {
    if(!S) {
      OS << "Invalid SCoP\n";
      return;
    }

    CLooG C = CLooG(S);
    CPPrinterActions cpa = CPPrinterActions(OS);
    ClastParser cp = ClastParser(&cpa);
    cp.parse(C.getClast());
    OS << "\n";
  }

  virtual void getAnalysisUsage(AnalysisUsage &AU) const {
    // XXX: Cannot be removed, as otherwise LLVM crashes.
    AU.setPreservesAll();
    AU.addRequired<SCoPInfo>();
  }
};
}

char ClastPrinter::ID = 1;

static RegisterPass<ClastPrinter>
Z("polly-print-clast", "Polly - Print CLooG's abstract syntax tree CLAST");

RegionPass* polly::createClastPrinterPass() {
	return new ClastPrinter();
}
