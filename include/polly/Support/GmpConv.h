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
void MPZ_from_APInt (mpz_t v, const llvm::APInt apint);

} //end namespace polly

#endif
