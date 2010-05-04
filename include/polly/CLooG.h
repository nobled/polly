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
  CloogProgram *Program;
  CloogOptions *Options;
  CloogState *State;

  unsigned StatementNumber;
public:
  CLooG(SCoP *Scop);

  void ScatterProgram();

  ~CLooG();

  /// Print a .cloog input file, that is equivalent to this program.
  // TODO: use raw_ostream as parameter.
  void dump();

  /// Print a source code representation of the program.
  // TODO: use raw_ostream as parameter.
  void pprint();

  /// Print the content of the Program data structure.
  // TODO: use raw_ostream as parameter.
  void print();

  /// Create the CLooG AST from this program.
  struct clast_stmt *getClast();

  void buildCloogOptions();

  /// Allocate a CloogLoop data structure containing information about stmt.
  CloogLoop *buildCloogLoop(SCoPStmt* stmt);

  /// Create a list of CloogLoops containing the statements of the SCoP.
  CloogLoop *buildCloogLoopList();

  /// Allocate a CloogScatteringList data structure and fill it with the
  /// scattering polyhedron of all statements in the SCoP. Ordered as they
  /// appear in the SCoP statement iterator.
  CloogScatteringList *buildScatteringList();

  /// Allocate a CloogNames data structure and fill it with default names.
  CloogNames *buildCloogNames(unsigned nb_scalars,
                              unsigned nb_scattering,
                              unsigned nb_iterators,
                              unsigned nb_parameters) const;

  int *buildScaldims(CloogProgram *Program) const;

  CloogBlockList *buildCloogBlockList(CloogLoop *LL);

  int getLoopIVfor(clast_name *name);

  void buildCloogProgram();
};
}
#endif /* POLLY_CLOOG_H */
