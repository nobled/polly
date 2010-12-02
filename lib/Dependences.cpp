//===- Dependency.cpp - Calculate dependency information for a SCoP.  -----===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Calculate the data dependency relations for a SCoP using ISL.
//
// The integer set library (ISL) from Sven, has a integrated dependency analysis
// to calculate data dependences. This pass takes advantage of this and
// calculate those dependences a SCoP.
//
// The dependences in this pass are exact in terms that for a specific read
// statement instance only the last write statement instance is returned. In
// case of may writes a set of possible write instances is returned. This
// analysis will never produce redundant dependences.
//
//===----------------------------------------------------------------------===//
//
#include "polly/Dependences.h"

#include "polly/LinkAllPasses.h"
#include "polly/SCoPInfo.h"

#define DEBUG_TYPE "polly-dependences"
#include "llvm/Support/Debug.h"
#include "llvm/Support/CommandLine.h"

#include <isl/flow.h>
#include <isl/map.h>
#include <isl/constraint.h>

using namespace polly;
using namespace llvm;

namespace polly {
static cl::opt<bool>
  LegalityCheckDisabled("disable-polly-legality",
       cl::desc("Disable polly legality check"), cl::Hidden,
       cl::init(false));

  Dependences::Dependences() : RegionPass(ID), S(0) {
    must_dep = may_dep = NULL;
    must_no_source = may_no_source = NULL;
    sink = must_source = may_source = NULL;
  }

  bool Dependences::runOnRegion(Region *R, RGPassManager &RGM) {
    S = getAnalysis<SCoPInfo>().getSCoP();

    if (!S)
      return false;

    isl_dim *dim = isl_dim_alloc(S->getCtx(), S->getNumParams(), 0, 0);

    if (sink)
      isl_union_map_free(sink);

    if (must_source)
      isl_union_map_free(must_source);

    if (may_source)
      isl_union_map_free(may_source);

    sink = isl_union_map_empty(isl_dim_copy(dim));
    must_source = isl_union_map_empty(isl_dim_copy(dim));
    may_source = isl_union_map_empty(isl_dim_copy(dim));
    isl_union_map *schedule = isl_union_map_empty(dim);

    if (must_dep)
      isl_union_map_free(must_dep);

    if (may_dep)
      isl_union_map_free(may_dep);

    if (must_no_source)
      isl_union_set_free(must_no_source);

    if (may_no_source)
      isl_union_set_free(may_no_source);

    must_dep = may_dep = NULL;
    must_no_source = may_no_source = NULL;

    for (SCoP::iterator SI = S->begin(), SE = S->end(); SI != SE; ++SI) {
      SCoPStmt *Stmt = *SI;

      for (SCoPStmt::memacc_iterator MI = Stmt->memacc_begin(),
           ME = Stmt->memacc_end(); MI != ME; ++MI) {
        isl_set *domcp = isl_set_copy(Stmt->getDomain());
        isl_map *accdom = isl_map_copy((*MI)->getAccessFunction());

        accdom = isl_map_intersect_domain(accdom, domcp);

        if ((*MI)->isRead())
          isl_union_map_add_map(sink, accdom);
        else
          isl_union_map_add_map(must_source, accdom);
      }
      isl_map *scattering = isl_map_copy(Stmt->getScattering());
      isl_union_map_add_map(schedule, scattering);
    }

    DEBUG(
      isl_printer *p = isl_printer_to_str(S->getCtx());

      errs().indent(4) << "Sink:\n";
      isl_printer_print_union_map(p, sink);
      errs().indent(8) << isl_printer_get_str(p) << "\n";
      isl_printer_flush(p);

      errs().indent(4) << "Must Source:\n";
      isl_printer_print_union_map(p, must_source);
      errs().indent(8) << isl_printer_get_str(p) << "\n";
      isl_printer_flush(p);

      errs().indent(4) << "May Source:\n";
      isl_printer_print_union_map(p, may_source);
      errs().indent(8) << isl_printer_get_str(p) << "\n";
      isl_printer_flush(p);

      errs().indent(4) << "Schedule:\n";
      isl_printer_print_union_map(p, schedule);
      errs().indent(8) << isl_printer_get_str(p) << "\n";
      isl_printer_flush(p);

      isl_printer_free(p));

    isl_union_map_compute_flow(isl_union_map_copy(sink),
                               isl_union_map_copy(must_source),
                               isl_union_map_copy(may_source), schedule,
                               &must_dep, &may_dep, &must_no_source,
                               &may_no_source);

    // Remove redundant statements.
    must_dep = isl_union_map_coalesce(must_dep);
    may_dep = isl_union_map_coalesce(may_dep);
    must_no_source = isl_union_set_coalesce(must_no_source);
    may_no_source = isl_union_set_coalesce(may_no_source);

    return false;
  }

  bool Dependences::isValidScattering(StatementToIslMapTy *NewScattering) {

    if (LegalityCheckDisabled)
      return true;

    isl_dim *dim = isl_dim_alloc(S->getCtx(), S->getNumParams(), 0, 0);

    isl_union_map *schedule = isl_union_map_empty(dim);

    for (SCoP::iterator SI = S->begin(), SE = S->end(); SI != SE; ++SI) {
      SCoPStmt *Stmt = *SI;

      isl_map *scattering;

      if (NewScattering->find(*SI) == NewScattering->end())
        scattering = isl_map_copy(Stmt->getScattering());
      else
        scattering = isl_map_copy((*NewScattering)[Stmt]);

      isl_union_map_add_map(schedule, scattering);
    }

    isl_union_map *temp_must_dep, *temp_may_dep;
    isl_union_set *temp_must_no_source, *temp_may_no_source;

    DEBUG(
      isl_printer *p = isl_printer_to_str(S->getCtx());

      errs().indent(4) << "Sink:\n";
      isl_printer_print_union_map(p, sink);
      errs().indent(8) << isl_printer_get_str(p) << "\n";
      isl_printer_flush(p);

      errs().indent(4) << "Must Source:\n";
      isl_printer_print_union_map(p, must_source);
      errs().indent(8) << isl_printer_get_str(p) << "\n";
      isl_printer_flush(p);

      errs().indent(4) << "May Source:\n";
      isl_printer_print_union_map(p, may_source);
      errs().indent(8) << isl_printer_get_str(p) << "\n";
      isl_printer_flush(p);

      errs().indent(4) << "Schedule:\n";
      isl_printer_print_union_map(p, schedule);
      errs().indent(8) << isl_printer_get_str(p) << "\n";
      isl_printer_flush(p);

      isl_printer_free(p));

    isl_union_map_compute_flow(isl_union_map_copy(sink),
                               isl_union_map_copy(must_source),
                               isl_union_map_copy(may_source), schedule,
                               &temp_must_dep, &temp_may_dep,
                               &temp_must_no_source, &temp_may_no_source);

    // Remove redundant statements.
    temp_must_dep = isl_union_map_coalesce(temp_must_dep);
    temp_may_dep = isl_union_map_coalesce(temp_may_dep);
    temp_must_no_source = isl_union_set_coalesce(temp_must_no_source);
    temp_may_no_source = isl_union_set_coalesce(temp_may_no_source);

    if (!isl_union_map_is_equal(temp_must_dep, must_dep))
      return false;

    if (!isl_union_map_is_equal(temp_may_dep, may_dep))
      return false;

    if (!isl_union_set_is_equal(temp_must_no_source, must_no_source))
      return false;

    if (!isl_union_set_is_equal(temp_may_no_source, may_no_source))
      return false;

    return true;
  }

static isl_map *getReverseMap(isl_ctx *context, int parameterDimension,
                              int scatteringDimensions,
                              int dimensionToReverse) {
  isl_dim *dim = isl_dim_alloc(context, parameterDimension,
                               scatteringDimensions, scatteringDimensions);

  isl_basic_map *bmap = isl_basic_map_universe(isl_dim_copy(dim));

  isl_int one;
  isl_int_init(one);
  isl_int_set_si(one, 1);

  isl_int minusOne;
  isl_int_init(minusOne);
  isl_int_set_si(minusOne, -1);

  for (int i = 0; i < scatteringDimensions; i++) {
    isl_constraint *eq = isl_equality_alloc(isl_dim_copy(dim));

    isl_constraint_set_coefficient(eq, isl_dim_in, i, one);

    if (i == dimensionToReverse)
      isl_constraint_set_coefficient(eq, isl_dim_out, i, one);
    else
      isl_constraint_set_coefficient(eq, isl_dim_out, i, minusOne);

    bmap = isl_basic_map_add_constraint(bmap, eq);
  }

  isl_int_clear(one);
  isl_int_clear(minusOne);
  isl_dim_free(dim);
  bmap = isl_basic_map_set_tuple_name(bmap, isl_dim_in, "scattering");
  bmap = isl_basic_map_set_tuple_name(bmap, isl_dim_out, "scattering");

  return isl_map_from_basic_map(bmap);
}

bool Dependences::isParallelDimension(isl_set *loopDomain,
                                      unsigned parallelDimension) {
  isl_dim *dim = isl_dim_alloc(S->getCtx(), S->getNumParams(), 0, 0);

  isl_union_map *schedule = isl_union_map_empty(dim);

  for (SCoP::iterator SI = S->begin(), SE = S->end(); SI != SE; ++SI) {
    SCoPStmt *Stmt = *SI;
    isl_map *scattering = isl_map_copy(Stmt->getScattering());

    int missingDimensions = Stmt->getNumScattering()
      - isl_set_n_dim(loopDomain);

    assert(missingDimensions >= 0 && "Scattering space too large");

    isl_set *scatteringDomain;

    if (missingDimensions > 0)
      scatteringDomain = isl_set_add_dims(isl_set_copy(loopDomain),
                                          isl_dim_set, missingDimensions);
    else
      scatteringDomain = isl_set_copy(loopDomain);

    scatteringDomain = isl_set_set_tuple_name(scatteringDomain, "scattering");

    isl_map *scatteringTmp = isl_map_intersect_range(isl_map_copy(scattering),
                                                     scatteringDomain);
    scatteringTmp = isl_map_intersect_domain(scatteringTmp,
                                          isl_set_copy(Stmt->getDomain()));

    // The statement instances executed in parallel.
    isl_set *parallelDomain = isl_map_domain(isl_map_copy(scatteringTmp));

    // The statement instances executed in sequential order.
    isl_set *nonParallelDomain =
      isl_set_universe(isl_set_get_dim(parallelDomain));

    // Set the correct tuple names.
    const char *iteratorName = isl_map_get_tuple_name(scattering, isl_dim_in);
    nonParallelDomain = isl_set_set_tuple_name(nonParallelDomain,
                                               iteratorName);
    nonParallelDomain = isl_set_subtract(nonParallelDomain,
                                         isl_set_copy(parallelDomain));

    // The non parallel part of the iteration domain is executed with the
    // normal scattering, whereas the subset of the iteration domain that
    // is in the parallel part is executed with a schedule where the parallel
    // dimension is reversed. In case this yields the same dependences as an
    // execution in normal execution order, the dimension does not carry any
    // dependences and execution it in parallel is therefore valid.
    isl_map *scatteringChanged = isl_map_copy(scattering);
    scatteringChanged = isl_map_intersect_domain(scatteringChanged,
                                                 parallelDomain);
    isl_map *scatteringUnchanged = scattering;
    scatteringUnchanged = isl_map_intersect_domain(scatteringUnchanged,
                                                   nonParallelDomain);

    isl_map *reverseMap = getReverseMap(S->getCtx(), S->getNumParams(),
                                        isl_map_n_out(scatteringChanged),
                                        parallelDimension);

    scatteringChanged = isl_map_apply_range(scatteringChanged,
                                            isl_map_copy(reverseMap));

    scattering = isl_map_union(scatteringChanged, scatteringUnchanged);

    isl_union_map_add_map(schedule, scattering);
  }

  isl_union_map *temp_must_dep, *temp_may_dep;
  isl_union_set *temp_must_no_source, *temp_may_no_source;

  isl_union_map_compute_flow(isl_union_map_copy(sink),
                             isl_union_map_copy(must_source),
                             isl_union_map_copy(may_source), schedule,
                             &temp_must_dep, &temp_may_dep,
                             &temp_must_no_source, &temp_may_no_source);

  if (!isl_union_map_is_equal(temp_must_dep, must_dep))
    return false;

  if (!isl_union_map_is_equal(temp_may_dep, may_dep))
    return false;

  if (!isl_union_set_is_equal(temp_must_no_source, must_no_source))
    return false;

  if (!isl_union_set_is_equal(temp_may_no_source, may_no_source))
    return false;

  return true;
}

  void Dependences::print(raw_ostream &OS, const Module *) const {
    if (!S)
      return;

    isl_printer *p = isl_printer_to_str(S->getCtx());

    OS.indent(4) << "Must dependences:\n";
    isl_printer_print_union_map(p, must_dep);
    OS.indent(8) << isl_printer_get_str(p) << "\n";
    isl_printer_flush(p);

    OS.indent(4) << "May dependences:\n";
    isl_printer_print_union_map(p, may_dep);
    OS.indent(8) << isl_printer_get_str(p) << "\n";
    isl_printer_flush(p);

    OS.indent(4) << "Must no source:\n";
    isl_printer_print_union_set(p, must_no_source);
    OS.indent(8) << isl_printer_get_str(p) << "\n";
    isl_printer_flush(p);

    OS.indent(4) << "May no source:\n";
    isl_printer_print_union_set(p, may_no_source);
    OS.indent(8) << isl_printer_get_str(p) << "\n";
    isl_printer_flush(p);

    isl_printer_free(p);
  }

  void Dependences::releaseMemory() {
    S = 0;

    if (must_dep)
      isl_union_map_free(must_dep);

    if (may_dep)
      isl_union_map_free(may_dep);

    if (must_no_source)
      isl_union_set_free(must_no_source);

    if (may_no_source)
      isl_union_set_free(may_no_source);

    must_dep = may_dep = NULL;
    must_no_source = may_no_source = NULL;

    if (sink)
      isl_union_map_free(sink);

    if (must_source)
      isl_union_map_free(must_source);

    if (may_source)
      isl_union_map_free(may_source);

    sink = must_source = may_source = NULL;
  }

  void Dependences::getAnalysisUsage(AnalysisUsage &AU) const {
    AU.addRequired<SCoPInfo>();
    AU.setPreservesAll();
  }
} //end anonymous namespace

char Dependences::ID = 0;

static RegisterPass<Dependences>
X("polly-dependences", "Polly - Calculate dependences for SCoP");

Pass* polly::createDependencesPass() {
  return new Dependences();
}
