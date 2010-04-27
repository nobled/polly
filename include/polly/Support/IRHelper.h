//===- Support/IRHelper.h --------- APInt <=> GMP objects -----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Small functions that help with LLVM-IR.
//
//===----------------------------------------------------------------------===//

#ifndef POLLY_SUPPORT_IRHELPER_H
#define POLLY_SUPPORT_IRHELPER_H

namespace llvm {
  class Region;
  class Pass;
}

namespace polly {
  void createSingleEntryEdge(llvm::Region *R);
  void createSingleExitEdge(llvm::Region *R, llvm::Pass *P);
}
#endif
