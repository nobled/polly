//===- CLooG.h -  ----------------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//
//
//===----------------------------------------------------------------------===//
//
#ifndef POLLY_CLOOG_H
#define POLLY_CLOOG_H

#include "polly/SCoPInfo.h"

#define CLOOG_INT_GMP 1
#include "cloog/cloog.h"

struct clast_name;

namespace polly {
class CLooG {
  SCoP *S;
  CloogOptions *Options;
  CloogState *State;
  clast_stmt *ClastRoot;

  void buildCloogOptions();
  CloogUnionDomain *buildCloogUnionDomain();
  CloogInput *buildCloogInput();

public:
  CLooG(SCoP *Scop);

  ~CLooG();

  /// Print a .cloog input file, that is equivalent to this program.
  // TODO: use raw_ostream as parameter.
  void dump(FILE *F);

  /// Print a source code representation of the program.
  // TODO: use raw_ostream as parameter.
  void pprint();

  /// Create the CLooG AST from this program.
  struct clast_stmt *getClast();
};
}
#endif /* POLLY_CLOOG_H */
