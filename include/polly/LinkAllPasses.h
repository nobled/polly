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

#include "polly/SCoPDetection.h"
#include "polly/SCoPInfo.h"
/// TODO: Place the headers that containing pass you want to force link here.

#include <cstdlib>

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
      (void) polly::createSCoPDetectionPass();
      (void) polly::createScopCodeGenPass();
    }
  } PollyForcePassLinking; // Force link by creating a global definition.
}

#endif
