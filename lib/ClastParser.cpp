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
  for (int i=0; i < n; i++)
    if(next)
      eval(&(next[i]), nctx);

  // After child-dfs.
  out(expr, &ctx);
}
}
