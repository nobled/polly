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

#include <isl_flow.h>

using namespace polly;
using namespace llvm;

namespace polly {

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

        const char *DomainName = isl_map_get_tuple_name(accdom, isl_dim_in);
        const char *ArrayName = isl_map_get_tuple_name(accdom, isl_dim_out);

        std::string DN = DomainName;
        std::string AN = ArrayName;

        std::string Combined = DN + "__" + AN;

        accdom = isl_map_intersect_domain(accdom, domcp);

        accdom = isl_map_set_tuple_name(accdom, isl_dim_in, Combined.c_str());

        if ((*MI)->isRead())
          isl_union_map_add_map(sink, accdom);
        else
          isl_union_map_add_map(must_source, accdom);

        isl_map *scattering = isl_map_copy(Stmt->getScattering());
        scattering = isl_map_set_tuple_name(scattering, isl_dim_in,
                                            Combined.c_str());
        isl_union_map_add_map(schedule, scattering);
      }
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
    isl_dim *dim = isl_dim_alloc(S->getCtx(), S->getNumParams(), 0, 0);

    isl_union_map *schedule = isl_union_map_empty(dim);

    for (SCoP::iterator SI = S->begin(), SE = S->end(); SI != SE; ++SI) {
      SCoPStmt *Stmt = *SI;

      for (SCoPStmt::memacc_iterator MI = Stmt->memacc_begin(),
           ME = Stmt->memacc_end(); MI != ME; ++MI) {
        isl_map *accdom = (*MI)->getAccessFunction();
        const char *DomainName = isl_map_get_tuple_name(accdom, isl_dim_in);
        const char *ArrayName = isl_map_get_tuple_name(accdom, isl_dim_out);

        std::string DN = DomainName;
        std::string AN = ArrayName;

        std::string Combined = DN + "__" + AN;

        isl_map *scattering;

        if (NewScattering->find(*SI) == NewScattering->end())
          scattering = isl_map_copy(Stmt->getScattering());
        else
          scattering = isl_map_copy((*NewScattering)[Stmt]);

        scattering = isl_map_set_tuple_name(scattering, isl_dim_in,
                                            Combined.c_str());
        isl_union_map_add_map(schedule, scattering);
      }
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
