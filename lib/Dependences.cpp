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

#include "polly/LinkAllPasses.h"
#include "polly/SCoPInfo.h"

#include "llvm/Analysis/RegionPass.h"

#define DEBUG_TYPE "polly-dependences"
#include "llvm/Support/Debug.h"

#include <isl_flow.h>

using namespace polly;
using namespace llvm;

namespace {

class Dependences : public RegionPass {

  SCoP *S;

  isl_union_map *must_dep, *may_dep;
  isl_union_set *must_no_source, *may_no_source;

public:
  static char ID;

  Dependences() : RegionPass(ID), S(0) {
    must_dep = may_dep = NULL;
    must_no_source = may_no_source = NULL;
  }

  bool runOnRegion(Region *R, RGPassManager &RGM) {
    S = getAnalysis<SCoPInfo>().getSCoP();

    if (!S)
      return false;

    isl_dim *dim = isl_dim_alloc(S->getCtx(), S->getNumParams(), 0, 0);

    isl_union_map *sink = isl_union_map_empty(isl_dim_copy(dim));
    isl_union_map *must_source = isl_union_map_empty(isl_dim_copy(dim));
    isl_union_map *may_source = isl_union_map_empty(isl_dim_copy(dim));
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

    isl_union_map_compute_flow(sink, must_source, may_source, schedule,
                               &must_dep, &may_dep, &must_no_source,
                               &may_no_source);

    // Remove redundant statements.
    must_dep = isl_union_map_coalesce(must_dep);
    may_dep = isl_union_map_coalesce(may_dep);
    must_no_source = isl_union_set_coalesce(must_no_source);
    may_no_source = isl_union_set_coalesce(may_no_source);

    return false;
  }

  void print(raw_ostream &OS, const Module *) const {
    if (!S)
      return;

    isl_printer *p = isl_printer_to_str(S->getCtx());
    //isl_printer_set_output_format(p, ISL_FORMAT_POLYLIB);

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

  virtual void releaseMemory() {
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
  }

  virtual void getAnalysisUsage(AnalysisUsage &AU) const {
    AU.addRequired<SCoPInfo>();
    AU.setPreservesAll();
  }
};
} //end anonymous namespace

char Dependences::ID = 0;

static RegisterPass<Dependences>
X("polly-dependences", "Polly - Calculate dependences for SCoP");

Pass* polly::createDependencesPass() {
  return new Dependences();
}
