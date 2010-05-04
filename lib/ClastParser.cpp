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

ClastParser::ClastParser(CPActions &actions) {
  act = &actions;
}

void ClastParser::parse(clast_stmt *root, cp_ctx *ctx) {
  if(root)
    dfs(root, -1, ctx);
}

// Traverse the clast with Depth-First-Search and
// fire in-/out- events when entering/leaving a node.
void ClastParser::dfs(clast_stmt *stmt, int depth, cp_ctx *ctx) {
  clast_stmt *next = NULL;
  // Before child-dfs.
  act->in(stmt, depth, ctx);

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
    dfs(next, depth+1, ctx);
  }

  // After child-dfs.
  act->out(stmt, depth, ctx);

  // Continue with sibling.
  if(stmt->next)
    dfs(stmt->next, depth, ctx);
}

void CPActions::eval(clast_expr *expr, cp_ctx *ctx) {
  clast_expr* next =  NULL;
  clast_expr_type t = expr->type;

  // Before child-dfs.
  in(expr, ctx);

  // Select next child.
  if      (t == clast_expr_term)
    next = ((clast_term *)expr)->var;
  else if (t == clast_expr_bin)
    next = ((clast_binary *)expr)->LHS;
  else if (t == clast_expr_red) {
    // Contract: Implementing actions have to evaluate
    // the elements of a reduction theirselves (with
    // calls to eval(..)), as this way the semantics of
    // a reduction can be captured in a clean way, without
    // carrying a parent-pointer in the cp_ctx.
  }
  // After child-dfs.
  out(expr, ctx);

  // DFS to child.
  if(next)
    eval(next, ctx);

}
}
