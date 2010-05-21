//===-- SCoPImporter.cpp  - Export SCoPs with openscop library --*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Import modified .scop files into Polly. This allows to change the schedule of
// statements.
//
//===----------------------------------------------------------------------===//

#include "polly/SCoPExchange.h"
#include "polly/SCoPInfo.h"

#define OPENSCOP_INT_T_IS_MP
#include "openscop/openscop.h"

#include "stdio.h"
#include "isl/isl_set.h"
#include "isl/isl_constraint.h"

using namespace llvm;
using namespace polly;

namespace {

struct SCoPImporter : public RegionPass {
  static char ID;
  SCoP *S;
  explicit SCoPImporter() : RegionPass(&ID) {}

  virtual bool runOnRegion(Region *R, RGPassManager &RGM);
  void getAnalysisUsage(AnalysisUsage &AU) const;
};

}

char SCoPImporter::ID = 0;

isl_map *scattering_to_map(openscop_matrix_p m, SCoPStmt *PollyStmt,
                           unsigned nb_scatt) {

  unsigned nb_param = PollyStmt->getNumParams();
  unsigned nb_iterators = PollyStmt->getNumIterators();
  unsigned nb_scat = m->NbColumns - 2 - nb_param - nb_iterators;

  isl_ctx *ctx = PollyStmt->getParent()->getCtx();
  polly_dim *dim = isl_dim_alloc(ctx, nb_param, nb_iterators, nb_scatt);
  polly_basic_map *bmap = isl_basic_map_universe(isl_dim_copy(dim));
  isl_int v;
  isl_int_init(v);

  for (unsigned i = 0; i < m->NbRows; ++i) {
    polly_constraint *c;

    if (m->p[i][0])
      c = isl_equality_alloc(isl_dim_copy(dim));
    else
      c = isl_inequality_alloc(isl_dim_copy(dim));

    for (unsigned j = 0; j < nb_scat; ++j)
      isl_constraint_set_coefficient(c, isl_dim_out, j, m->p[i][1 + j]);

    for (unsigned j = 0; j < nb_iterators; ++j)
      isl_constraint_set_coefficient(c, isl_dim_in, j,
                                          m->p[i][nb_scat + 1 + j]);

    for (unsigned j = 0; j < nb_param; ++j)
      isl_constraint_set_coefficient(c, isl_dim_param, j,
                                     m->p[i][nb_scat + nb_iterators + 1 + j]);

    isl_constraint_set_constant(c,
                                m->p[i][nb_scat + nb_iterators + nb_param + 1]);

    bmap = isl_basic_map_add_constraint(bmap, c);
  }

  isl_dim_free(dim);
  return isl_map_from_basic_map(bmap);
}

void updateScattering(SCoPStmt *PollyStmt, openscop_statement_p OStmt,
                      unsigned nb_scatt) {
  assert(OStmt && "No openscop statement available");
  isl_map *m = scattering_to_map (OStmt->schedule, PollyStmt, nb_scatt);
  PollyStmt->setScattering(m);
}

void updateScattering(SCoP *S, openscop_scop_p OSCoP) {

  openscop_statement_p stmt = OSCoP->statement;
  unsigned max_scattering = 0;
  // Initialize the statements.
  for (SCoP::iterator SI = S->begin(), SE = S->end(); SI != SE; ++SI) {
    unsigned nb_scattering = stmt->schedule->NbColumns - 2
      - (*SI)->getNumParams() - (*SI)->getNumIterators();

    max_scattering = std::max(max_scattering, nb_scattering);
    stmt = stmt->next;
  }

  stmt = OSCoP->statement;
  // Initialize the statements.
  for (SCoP::iterator SI = S->begin(), SE = S->end(); SI != SE; ++SI) {
    updateScattering(*SI, stmt, max_scattering);
    stmt = stmt->next;
  }
}

bool SCoPImporter::runOnRegion(Region *R, RGPassManager &RGM) {
  S = getAnalysis<SCoPInfo>().getSCoP();

  if (!S)
    return false;

  std::string FunctionName = R->getEntry()->getParent()->getNameStr();
  std::string Filename = FunctionName + "_-_" + R->getNameStr() + ".scop";

  errs() << "Reading SCoP '" << R->getNameStr() << "' in function '"
    << FunctionName << "' from '" << Filename << "'...\n";

  FILE *F = fopen(Filename.c_str(), "r");
  openscop_scop_p scop = openscop_scop_read(F);
  fclose(F);

  updateScattering(S, scop);

  return false;
}

void SCoPImporter::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
  AU.addRequired<SCoPInfo>();
}

static RegisterPass<SCoPImporter> A("polly-import",
                                    "Polly - Import SCoPs with OpenSCoP library"
                                    " (Reads a .scop file for each SCoP)"
                                    );

Pass *polly::createSCoPImporterPass() {
  return new SCoPImporter();
}
