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
  class PassInfo;
  class RegionPass;
}

using namespace llvm;

namespace polly {
  Pass *createAffSCEVItTesterPass();
  Pass *createCLooGExporterPass();
  Pass *createCodeGenerationPass();
  Pass *createDependencesPass();
  Pass *createDOTOnlyPrinterPass();
  Pass *createDOTOnlyViewerPass();
  Pass *createDOTPrinterPass();
  Pass *createDOTViewerPass();
  Pass *createIndependentBlocksPass();
  Pass *createSCoPCodePrepPass();
  Pass *createSCoPInfoPass();
  Pass *createSCoPPrinterPass();

#ifdef OPENSCOP_FOUND
  Pass *createSCoPExporterPass();
  Pass *createSCoPImporterPass();
#endif

  extern char &IndependentBlocksID;
  extern char &SCoPCodePrepID;
}

using namespace polly;

namespace {
  struct PollyForcePassLinking {
    PollyForcePassLinking() {
      // We must reference the passes in such a way that compilers will not
      // delete it all as dead code, even with whole program optimization,
      // yet is effectively a NO-OP. As the compiler isn't smart enough
      // to know that getenv() never returns -1, this will do the job.
      if (std::getenv("bar") != (char*) -1)
        return;

       createAffSCEVItTesterPass();
       createCLooGExporterPass();
       createCodeGenerationPass();
       createDependencesPass();
       createDOTOnlyPrinterPass();
       createDOTOnlyViewerPass();
       createDOTPrinterPass();
       createDOTViewerPass();
       createIndependentBlocksPass();
       createSCoPCodePrepPass();
       createSCoPInfoPass();
       createSCoPPrinterPass();

#ifdef OPENSCOP_FOUND
       createSCoPExporterPass();
       createSCoPImporterPass();
#endif

    }
  } PollyForcePassLinking; // Force link by creating a global definition.
}

#endif
