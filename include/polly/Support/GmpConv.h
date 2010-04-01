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
/// FIXME: move the function elsewhere, level its declare here.
/// TODO: documents.
///
static inline void MPZ_from_APInt (mpz_t v, const APInt apint) {
  const uint64_t *rawdata = apint.getRawData();
  unsigned numWords = apint.getNumWords();

  // TODO: Check if this is true for all platforms.
  mpz_import(v, numWords, 1, sizeof (uint64_t), 0, 0, rawdata);
}

} //end namespace polly

#endif
