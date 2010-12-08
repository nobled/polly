//===- GmpConv.cpp - Recreate LLVM IR from the Scop.  ---------------------===//
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

void polly::MPZ_from_APInt (mpz_t v, const APInt apint, bool is_signed) {
  // There is no sign taken from the data, rop will simply be a positive
  // integer. An application can handle any sign itself, and apply it for
  // instance with mpz_neg.
  APInt abs;
  if (is_signed)
   abs  = apint.abs();
  else
   abs = apint;

  const uint64_t *rawdata = abs.getRawData();
  unsigned numWords = abs.getNumWords();

  // TODO: Check if this is true for all platforms.
  mpz_import(v, numWords, 1, sizeof (uint64_t), 0, 0, rawdata);

  if (is_signed && apint.isNegative()) mpz_neg(v, v);
}

APInt polly::APInt_from_MPZ (const mpz_t mpz) {
  uint64_t *p = NULL;
  size_t sz;

  p = (uint64_t*) mpz_export(p, &sz, 1, sizeof(uint64_t), 0, 0, mpz);

  if (p) {
    APInt A((unsigned)mpz_sizeinbase(mpz, 2), (unsigned)sz , p);
    A = A.zext(A.getBitWidth() + 1);

    if (mpz_sgn(mpz) == -1)
      return -A;
    else
      return A;
  } else {
    uint64_t val = 0;
    return APInt(1, 1, &val);
  }
}
