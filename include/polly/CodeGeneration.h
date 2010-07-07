//===- CodeGeneration.h - Generate LLVM-IR ----------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Generate the LLVM-IR code from SCoP description.
//
//===----------------------------------------------------------------------===//
//
#ifndef POLLY_CODEGENERATION_H
#define POLLY_CODEGENERATION_H

#include "llvm/Analysis/RegionPass.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/Support/raw_ostream.h"
namespace polly {
llvm::RegionPass* createCodeGenerationPass();
} //end polly namespace
#endif /* POLLY_CODEGENERATION_H */
