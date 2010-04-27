//===- GmpConv.cpp - Recreate LLVM IR from the SCoP.  ---------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Functions for converting between gmp objects and apint.
//
//===----------------------------------------------------------------------===//
#include "polly/Support/GmpConv.h"

using namespace llvm;

void polly::MPZ_from_APInt (mpz_t v, const APInt apint) {
  // There is no sign taken from the data, rop will simply be a positive
  // integer. An application can handle any sign itself, and apply it for
  // instance with mpz_neg.
  APInt abs = apint.abs();

  const uint64_t *rawdata = abs.getRawData();
  unsigned numWords = abs.getNumWords();

  // TODO: Check if this is true for all platforms.
  mpz_import(v, numWords, 1, sizeof (uint64_t), 0, 0, rawdata);

  if (apint.isNegative()) mpz_neg(v, v);
}
