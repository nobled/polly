//===- polly/LinkAllPasses.h ------------ Reference All Passes ---*- C++ -*-===//
//
//                      The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This header file pulls in all transformation and analysis passes for tools
// like opt and bugpoint that need this functionality.
//
//===----------------------------------------------------------------------===//

#ifndef POLLY_LINKALLPASSES_H
#define POLLY_LINKALLPASSES_H

#include "polly/Config/config.h"
#include <cstdlib>

namespace llvm {
  class Pass;
  class RegionPass;
}

namespace polly {
  llvm::Pass *createScalarDataRefPass();
  llvm::Pass *createAffSCEVItTesterPass();
  llvm::Pass *createSCoPInfoPass();
  llvm::Pass *createScopPrinterPass();
  llvm::Pass *createScopCodeGenPass();
  llvm::Pass *createSCoPExporterPass();
  llvm::Pass *createSCoPImporterPass();
  llvm::RegionPass* createCLooGExporterPass();
  llvm::RegionPass* createCodeGenerationPass();
  llvm::RegionPass* createIndependentBlocksPass();
}

namespace {
  struct PollyForcePassLinking {
    PollyForcePassLinking() {
      // We must reference the passes in such a way that compilers will not
      // delete it all as dead code, even with whole program optimization,
      // yet is effectively a NO-OP. As the compiler isn't smart enough
      // to know that getenv() never returns -1, this will do the job.
      if (std::getenv("bar") != (char*) -1)
        return;

      (void) polly::createSCoPInfoPass();
      (void) polly::createScopPrinterPass();
      (void) polly::createAffSCEVItTesterPass();
      (void) polly::createCodeGenerationPass();
      (void) polly::createScalarDataRefPass();
      (void) polly::createCLooGExporterPass();
      (void) polly::createIndependentBlocksPass();

#ifdef OPENSCOP_FOUND
      (void) polly::createSCoPExporterPass();
      (void) polly::createSCoPImporterPass();
#endif
    }
  } PollyForcePassLinking; // Force link by creating a global definition.
}

#endif
