//===-- SCoPImporter.cpp  - Export SCoPs with openscop library --*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Import modified .scop files into Polly. This allows to change the schedule of
// statements.
//
//===----------------------------------------------------------------------===//

#include "polly/SCoPExchange.h"
#include "polly/SCoPInfo.h"

#define OPENSCOP_INT_T_IS_MP
#include "openscop/openscop.h"

#include "stdio.h"
#include "isl/isl_set.h"
#include "isl/isl_constraint.h"

using namespace llvm;
using namespace polly;

namespace {

struct SCoPImporter : public RegionPass {
  static char ID;
  SCoP *S;
  explicit SCoPImporter() : RegionPass(&ID) {}

  virtual bool runOnRegion(Region *R, RGPassManager &RGM);
  void getAnalysisUsage(AnalysisUsage &AU) const;
};

}

char SCoPImporter::ID = 0;

void updateScattering(SCoP *PollySCoP, openscop_scop_p OSCoP) {
}

bool SCoPImporter::runOnRegion(Region *R, RGPassManager &RGM) {
  S = getAnalysis<SCoPInfo>().getSCoP();

  if (!S)
    return false;

  std::string exit = R->getExit() ? R->getExit()->getNameStr()
    : "FunctionReturn";
  std::string Filename = R->getEntry()->getParent()->getNameStr() + "-"
    + R->getEntry()->getNameStr() + "_to_" + exit
    + ".scop";
  errs() << "Reading '" << Filename << "'...\n";

  FILE *F = fopen(Filename.c_str(), "r");
  openscop_scop_p scop = openscop_scop_read(F);
  fclose(F);
  updateScattering(S, scop);

  return false;
}

void SCoPImporter::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
  AU.addRequired<SCoPInfo>();
}

static RegisterPass<SCoPImporter> A("polly-import-scop",
                                    "Polly - Import SCoPs with openscop library");

Pass *polly::createSCoPImporterPass() {
  return new SCoPImporter();
}
