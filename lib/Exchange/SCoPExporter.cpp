//===-- SCoPExporter.cpp  - Export SCoPs with openscop library --*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Export the SCoPs build by SCoPInfo pass to text file.
//
//===----------------------------------------------------------------------===//

#include "polly/SCoPExchange.h"
#include "polly/SCoPInfo.h"

#define OPENSCOP_INT_T_IS_MP
#include "openscop/openscop.h"

#include "stdio.h"

using namespace llvm;
using namespace polly;

namespace {

struct SCoPExporter : public RegionPass {
  static char ID;
  SCoP *S;
  explicit SCoPExporter() : RegionPass(&ID) {}

  virtual bool runOnRegion(Region *R, RGPassManager &RGM);
  void getAnalysisUsage(AnalysisUsage &AU) const;
};

}

char SCoPExporter::ID = 0;

openscop_scop_p	ScopToOpenScop(SCoP *S) {
  openscop_scop_p OpenSCoP = openscop_scop_malloc();
  return OpenSCoP;
}

bool SCoPExporter::runOnRegion(Region *R, RGPassManager &RGM) {
  S = getAnalysis<SCoPInfo>().getSCoP();

  if (!S)
    return false;

  std::string exit = R->getExit() ? R->getExit()->getNameStr()
    : "FunctionReturn";
  std::string Filename = R->getEntry()->getParent()->getNameStr() + "-"
    + R->getEntry()->getNameStr() + "_to_" + exit
    + ".scop";
  errs() << "Writing '" << Filename << "'...\n";

  FILE *F = fopen(Filename.c_str(), "w");
  openscop_scop_print_dot_scop(F, ScopToOpenScop(S));
  fclose(F);

  return false;
}

void SCoPExporter::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
  AU.addRequired<SCoPInfo>();
}

static RegisterPass<SCoPExporter> A("polly-export-scop",
                                    "Polly - Export SCoPs with openscop library");

Pass *polly::createSCoPExporterPass() {
  return new SCoPExporter();
}

// Dirty hack

Pass *polly::createSCoPImporterPass() {
  return new SCoPExporter();
}
