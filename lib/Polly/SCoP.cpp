//===- SCoP.cpp - Create a SCoP -------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Create a polyhedral description of the region.
//
//===----------------------------------------------------------------------===//

#include "polly/SCoP.h"
#include "polly/LinkAllPasses.h"
#include "llvm/Analysis/RegionInfo.h"
#include "llvm/Analysis/RegionIterator.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/ScalarEvolutionExpressions.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/SmallVector.h"

using namespace llvm;
using namespace polly;

#include "isl_constraint.h"
#include <algorithm>

namespace polly {

/// TODO: Helper function. Needs to be moved to the right place.
void MPZ_from_APInt (mpz_t v, const APInt apint) {
    const uint64_t *rawdata = apint.getRawData();
    unsigned numWords = apint.getNumWords();

    // TODO: Check if this is true for all platforms.
    mpz_import(v, numWords, 1, sizeof (uint64_t), 0, 0, rawdata);
}

/// TODO: Helper function. Needs to be moved to the right place.
Loop *getSurroundingLoop(LoopInfo *LI, Region *R) {
  Loop *loop = LI->getLoopFor(R->getEntry());
  while (loop && loop->getHeader() && R->contains(loop->getHeader())) {
    SmallVector<BasicBlock*, 8> ExitingBlocks;
    loop->getExitingBlocks(ExitingBlocks);

    for (unsigned i = 0, e = ExitingBlocks.size(); i != e; ++i) {
      BasicBlock *ExitingBlock = ExitingBlocks[i];
      if (!R->contains(ExitingBlock))
        break;
    }

    loop = loop->getParentLoop();
  }

  return loop;
}
/// TODO: Helper function. Needs to be moved to the right place.
unsigned getLoopDepth(SCoP *S, BasicBlock *BB) {
    Loop *surroundingL = getSurroundingLoop(S->LI, S->getRegion());
    unsigned BBLD = S->LI->getLoopDepth(BB);
    unsigned SLD = 0;

    if (surroundingL)
      SLD = surroundingL->getLoopDepth();

    return BBLD - SLD;
}

/// TODO: Helper function. Needs to be moved to the right place.
unsigned getLoopDepth(SCoP *S, Loop *L) {
    assert(L);
    Loop *surroundingL = getSurroundingLoop(S->LI, S->getRegion());
    unsigned BBLD = L->getLoopDepth();
    unsigned SLD = 0;

    if (surroundingL)
      SLD = surroundingL->getLoopDepth();

    return BBLD - SLD;
}

/// TODO: Helper function. Needs to be moved to the right place.
unsigned getMaxLoopDepth(SCoP *S) {
  unsigned max = 0;
  Region *R = S->getRegion();

  for (Region::block_iterator BI = R->block_begin(), BE = R->block_end();
       BI != BE; ++BI)
    max = std::max(max, getLoopDepth(S, (*BI)->getBB()));

  return max;
}

// TODO: Helper function. Needs to be moved to the right place.
Loop* findCommonLoop(Loop* A, Loop* B) {
    while (A && !A->contains(B))
      A = A->getParentLoop();

    return A;
}

void Statement::AllocateScattering(unsigned *value) {
  unsigned iterators = getLoopDepth(Scop, BB);
  unsigned scat_dims = getMaxLoopDepth(Scop) * 2 + 1;
  struct isl_dim *dim = isl_dim_alloc(Scop->isl_ctx, 0, iterators, scat_dims);
  struct isl_basic_map *bmap = isl_basic_map_universe(isl_dim_copy(dim));

  // Loop dimensions.
  for (unsigned i = 0; i < iterators; ++i) {
    isl_int v;
    isl_int_init(v);
    struct isl_constraint *c = isl_equality_alloc(isl_dim_copy(dim));
    isl_int_set_si(v, 1);
    isl_constraint_set_coefficient(c, isl_dim_out, 2 * i + 1, v);
    isl_int_set_si(v, -1);
    isl_constraint_set_coefficient(c, isl_dim_in, i, v);

    bmap = isl_basic_map_add_constraint(bmap, c);
  }

  // Constant dimensions
  for (unsigned i = 0; i < iterators + 1; ++i) {
    isl_int v;
    isl_int_init(v);
    struct isl_constraint *c = isl_equality_alloc(isl_dim_copy(dim));
    isl_int_set_si(v, -1);
    isl_constraint_set_coefficient(c, isl_dim_out, 2 * i, v);
    isl_int_set_si(v, value[i]);
    isl_constraint_set_constant(c, v);

    bmap = isl_basic_map_add_constraint(bmap, c);
  }

  // Fill scattering dimensions.
  for (unsigned i = 2 * iterators + 1; i < scat_dims ; ++i) {
    isl_int v;
    isl_int_init(v);
    struct isl_constraint *c = isl_equality_alloc(isl_dim_copy(dim));
    isl_int_set_si(v, 1);
    isl_constraint_set_coefficient(c, isl_dim_out, i, v);
    isl_int_set_si(v, -1);
    isl_constraint_set_constant(c, v);

    bmap = isl_basic_map_add_constraint(bmap, c);
  }

  isl_dim_free(dim);
  Scattering = isl_map_from_basic_map(bmap);
}

/// Create a constraint describing the lower bound of the Loop L.
struct isl_constraint *Statement::createLBConstraintForLoop(Loop *L) {
  struct isl_dim *dim = isl_dim_set_alloc(Scop->isl_ctx, 0, NbIterators);
  struct isl_constraint *c = isl_inequality_alloc(isl_dim_copy(dim));

  isl_int v;
  isl_int_init(v);

  if (getLoopDepth(Scop, L) == 0)
    return c;

  unsigned iterator = getLoopDepth(Scop, L) - 1;

  // i >= 0
  isl_int_set_si(v, 0);
  isl_constraint_set_constant(c, v);
  isl_int_set_si(v, 1);
  isl_constraint_set_coefficient(c, isl_dim_set, iterator, v);

  return c;
}

/// Create a constraint describing the upper bound of the Loop L.
struct isl_constraint *Statement::createUBConstraintForLoop(Loop *L) {
  struct isl_dim *dim = isl_dim_set_alloc(Scop->isl_ctx, 0, NbIterators);
  struct isl_constraint *c = isl_inequality_alloc(isl_dim_copy(dim));

  isl_int v;
  isl_int_init(v);
  isl_int ub;
  isl_int_init(ub);

  if (NbIterators == 0)
    return c;

  const SCEV *scev = Scop->SE->getBackedgeTakenCount(L);
  if (scConstant == scev->getSCEVType()) {
    const SCEVConstant *cscev = (SCEVConstant *) scev;
    const APInt apint = cscev->getValue()->getValue();
    MPZ_from_APInt (ub, apint);
  }

  if (getLoopDepth(Scop, L) == 0)
    return c;

  unsigned iterator = getLoopDepth(Scop, L) - 1;

  // i <= # of loop iterations
  c = isl_inequality_alloc(isl_dim_copy(dim));
  isl_constraint_set_constant(c, ub);
  isl_int_set_si(v, -1);
  isl_constraint_set_coefficient(c, isl_dim_set, iterator, v);
  isl_int_clear(v);
  isl_int_clear(ub);

  return c;
}

void Statement::AllocateDomain() {
  struct isl_dim *dim = isl_dim_set_alloc(Scop->isl_ctx, 0, NbIterators);
  struct isl_basic_set *bset = isl_basic_set_universe(isl_dim_copy(dim));

  Loop *Lo = Scop->LI->getLoopFor(BB);

  while (Lo && NbIterators && getSurroundingLoop(Scop->LI, Scop->getRegion())->contains(Lo)) {
    isl_constraint *C;

    C = createLBConstraintForLoop(Lo);
    bset = isl_basic_set_add_constraint(bset, C);

    C = createUBConstraintForLoop(Lo);
    bset = isl_basic_set_add_constraint(bset, C);

    Lo = Lo->getParentLoop();
  }

  Domain = isl_set_from_basic_set(bset);

  isl_dim_free(dim);
}

Statement::Statement(SCoP *SCoP, BasicBlock *BasicBlock, unsigned *value) {
  BB = BasicBlock;
  Scop = SCoP;
  NbIterators = getLoopDepth(SCoP, BB);
  AllocateScattering(value);
  AllocateDomain();
}


Statement::~Statement() {
  if (Domain)
    isl_set_free(Domain);
  if (Scattering)
    isl_map_free(Scattering);
}

void Statement::print(raw_ostream &OS) const {
  OS << "\tStatement " << BB->getNameStr() << ":\n";

  OS << "\t\tDomain:\n";
  if (Domain)
    isl_set_dump(Domain, stderr, 20);
  else
    OS << "\t\t\tn/a\n";

  OS << "\n";

  OS << "\t\t Scattering:\n";
  if (Scattering)
    isl_map_dump(Scattering, stderr, 20);
  else
    OS << "\t\t\tn/a\n";
}

raw_ostream& operator<<(raw_ostream &O, const Statement *S) {
    S->print(O);
    return O;
}

void SCoP::findBlackBoxes() {
  unsigned constc = getMaxLoopDepth(this) + 1;

  unsigned *value = (unsigned *) malloc (sizeof (unsigned) * (constc + 1));
  for (unsigned i = 0; i < constc + 1; ++i)
    value[i] = 0;

  Loop *last = 0;
  for (Region::block_iterator BI = region->block_begin(), BE = region->block_end();
       BI != BE; ++BI) {
    Loop *L = LI->getLoopFor((*BI)->getBB());

    int a;
    if (last) {
      Loop *CL = findCommonLoop(last, L);
      a = getLoopDepth(this, CL);
    } else
      a = 0;

    ++value[a];

    Statement *stmt = new Statement(this, (*BI)->getBB(), value);
    Statements.insert(stmt);

    last = L;
  }
}

SCoP::SCoP() : RegionPass(&ID) {
  isl_ctx = isl_ctx_alloc();
  // TODO: Support parameters.
  struct isl_dim *dim = isl_dim_set_alloc(isl_ctx, 0, 0);
  Context = isl_set_universe (dim);
}

SCoP::~SCoP() {
  for (StmtSet::iterator SI = Statements.begin(), SE = Statements.end();
       SI != SE; ++SI)
    delete(*SI);

  isl_set_free(Context);

  // XXX: Cannot be removed as ctx might still be used in different passes.
  if (0)
    isl_ctx_free(isl_ctx);
}

bool SCoP::runOnRegion(Region *R, RGPassManager &RGM) {
    region = R;
    Statements.clear();
    SE = &getAnalysis<ScalarEvolution>();
    LI = &getAnalysis<LoopInfo>();
    NbScatteringDimensions = getMaxLoopDepth(this) * 2 + 1;

    findBlackBoxes();
    return false;
}

void SCoP::getAnalysisUsage(AnalysisUsage &AU) const {
    AU.setPreservesAll();
    AU.addRequired<ScalarEvolution>();
    AU.addRequired<LoopInfo>();
}

unsigned SCoP::getNumberParameters() {
  return isl_set_n_param(Context);
}

void SCoP::printContext(raw_ostream &OS) const {
  OS << "\tContext:\n";
  if (Context)
    isl_set_dump(Context, stderr, 12);
  else
    OS << "\t\tn/a\n";

  OS << "\n";
}

void SCoP::printStatements(raw_ostream &OS) const {
  for (StmtSet::iterator SI = Statements.begin(), SE = Statements.end();
       SI != SE; ++SI)
    OS << (*SI) << "\n";
}

void SCoP::print(raw_ostream &OS, const Module *) const {
  printContext(OS);
  printStatements(OS);
}
} //end polly namespace

char polly::SCoP::ID = 0;

static RegisterPass<SCoP>
X("polly-scop", "Polly - Create polyhedral SCoPs");

RegionPass* polly::createSCoPPass() {
  return new SCoP();
}
