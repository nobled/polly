//===- Support/GmpConv.h --------- APInt <=> GMP objects --------------------===//
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
//
#ifndef POLLY_SUPPORT_GMP_CONV_H
#define POLLY_SUPPORT_GMP_CONV_H

#include "llvm/ADT/APInt.h"
#include <gmp.h>

namespace polly {

/// @brief Convert APInt to mpz.
///
/// FIXME: move the function body elsewhere, leave its declaretion here.
///
/// @param v      The mpz_t object your want to hold the result.
/// @param apint  The APInt you want to convert.
static inline void MPZ_from_APInt (mpz_t v, const APInt apint) {
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

} //end namespace polly

#endif
