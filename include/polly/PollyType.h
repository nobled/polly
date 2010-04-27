//===- polly/SCoP/PollyType.h - typedef for isl structs ----------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// typedef for isl structs
//
//===----------------------------------------------------------------------===//
//
#ifndef POLLY_POLLY_TYPE_DEF_H
#define POLLY_POLLY_TYPE_DEF_H

// No namespace

struct isl_dim;
typedef struct isl_dim polly_dim;

struct isl_set;
typedef struct isl_set polly_set;

struct isl_map;
typedef struct isl_map polly_map;

struct isl_ctx;
typedef struct isl_ctx polly_ctx;

struct isl_constraint;
typedef struct isl_constraint polly_constraint;

struct isl_basic_set;
typedef struct isl_basic_set polly_basic_set;

struct isl_basic_map;
typedef struct isl_basic_map polly_basic_map;

#endif
