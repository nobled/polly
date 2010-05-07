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

using namespace llvm;
using namespace polly;

namespace {

struct SCoPExporter : public RegionPass {
  static char ID;
  explicit SCoPExporter() : RegionPass(&ID) {}

  virtual bool runOnRegion(Region *R, RGPassManager &RGM);
};

}

char SCoPExporter::ID = 0;

bool SCoPExporter::runOnRegion(Region *R, RGPassManager &RGM) {
  return false;
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
