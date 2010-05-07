//===- polly/SCoP/SCoPExchange.h - Create SCoPs from LLVM IR ----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Export/Import SCoPs with openscop library
//
//===----------------------------------------------------------------------===//
//
#ifndef POLLY_SCOP_EXCHANGE_H
#define POLLY_SCOP_EXCHANGE_H

#include "llvm/Pass.h"
#include "llvm/Analysis/RegionPass.h"

using namespace llvm;

namespace polly {
  Pass *createSCoPExporterPass();
  Pass *createSCoPImporterPass();
}

#endif
