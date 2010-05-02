//===- ClastPrinter.cpp - Print the CLooG AST.  ---------------------------===//
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
#include "polly/Support/GmpConv.h"
#include "polly/CLooG.h"
#include "llvm/ADT/APInt.h"

#define CLOOG_INT_GMP 1
#include "cloog/cloog.h"

#ifdef _WINDOWS
#define snprintf _snprintf
#endif

using namespace polly;
using namespace llvm;

namespace polly {

class CPPrinterActions : virtual public CPActions {
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
	  eval(a->RHS, *ctx);
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
	indent(depth);
	*ost << "}\n";
	break;
    }
  }

  void print(struct clast_equation *eq, int depth, cp_ctx *ctx) {
    eval(eq->LHS, *ctx);
    if (eq->sign == 0)
      *ost << "==";
    else if (eq->sign > 0)
      *ost << ">=";
    else
      *ost << "<=";
    eval(eq->RHS, *ctx);
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
  CPPrinterActions(raw_ostream& strm) {
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
class ClastPrinter : public RegionPass {
  Region *region;
  SCoP *S;

  public:
  static char ID;

  ClastPrinter() : RegionPass(&ID) {}

  bool runOnRegion(Region *R, RGPassManager &RGM) {
    region = R;
    S = getAnalysis<SCoPInfo>().getSCoP();

    return false;
  }

  void print(raw_ostream &OS, const Module *) const {
    if(!S) {
      OS << "Invalid SCoP\n";
      return;
    }
    CLooG C = CLooG(S);
    CPPrinterActions cpa = CPPrinterActions(OS);
    ClastParser cp = ClastParser(cpa);
    cp_ctx ctx;

    cp.parse(C.getClast(), ctx);
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
