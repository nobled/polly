//===------ polly/Dependences.h - Polyhedral dependency analysis *- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Calculate the data dependency relations for a SCoP using ISL.
//
// The integer set library (ISL) from Sven, has a integrated dependency analysis
// to calculate data dependences. This pass takes advantage of this and
// calculate those dependences a SCoP.
//
// The dependences in this pass are exact in terms that for a specific read
// statement instance only the last write statement instance is returned. In
// case of may writes a set of possible write instances is returned. This
// analysis will never produce redundant dependences.
//
//===----------------------------------------------------------------------===//

#ifndef POLLY_DEPENDENCES_H
#define POLLY_DEPENDENCES_H

#include "llvm/Analysis/RegionPass.h"

#include <map>

struct isl_union_map;
struct isl_union_set;
struct isl_map;

using namespace llvm;

namespace polly {

  class SCoP;
  class SCoPStmt;

  class Dependences : public RegionPass {

    SCoP *S;

    isl_union_map *must_dep, *may_dep;
    isl_union_set *must_no_source, *may_no_source;

  public:
    static char ID;
    typedef std::map<SCoPStmt*, isl_map*> StatementToIslMapTy;

    Dependences();
    bool isValidScattering(StatementToIslMapTy *NewScatterings);

    bool runOnRegion(Region *R, RGPassManager &RGM);
    void print(raw_ostream &OS, const Module *) const;
    virtual void releaseMemory();
    virtual void getAnalysisUsage(AnalysisUsage &AU) const;
  };
} // End polly namespace.

#endif
