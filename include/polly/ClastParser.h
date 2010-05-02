//===- ClastParser.h - Parse the CLooG AST. ---------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Parse the CLooG AST.
//
//===----------------------------------------------------------------------===//
//
#ifndef POLLY_CLAST_PARSER_H
#define POLLY_CLAST_PARSER_H

#include "llvm/Analysis/RegionPass.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/Support/raw_ostream.h"

struct clast_stmt;
struct clast_expr;

namespace polly {

enum cp_dir {
  DFS_IN,
  DFS_OUT
};

struct cp_ctx {
  cp_dir dir;
  int depth;
  clast_stmt *pred;
};

class CPActions {
  protected:
  void eval(clast_expr *expr, cp_ctx ctx);

  public:
  virtual void in(clast_stmt *s, cp_ctx *ctx) = 0;
  virtual void in(clast_expr *e, cp_ctx *ctx) = 0;

  virtual void out(clast_stmt *s, cp_ctx *ctx) = 0;
  virtual void out(clast_expr *e, cp_ctx *ctx) = 0;
};

class ClastParser {
  CPActions *act;
 
  public:
  ClastParser(CPActions *actions);

  void parse(clast_stmt *root);

  protected:
  void dfs(clast_stmt *stmt, cp_ctx ctx);
};

/// Function for force linking.
llvm::RegionPass* createClastPrinterPass();
} //end polly namespace
#endif /* POLLY_CLAST_PARSER_H */
