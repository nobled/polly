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

#include "polly/SCoPPass.h"
#include "polly/Support/GmpConv.h"

#include "llvm/Analysis/RegionInfo.h"
#include "llvm/Analysis/RegionIterator.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/ScalarEvolutionExpressions.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/SmallVector.h"

#define DEBUG_TYPE "polly-scop"
#include "llvm/Support/Debug.h"


using namespace llvm;
using namespace polly;

#include "isl_constraint.h"
#include <algorithm>

//===----------------------------------------------------------------------===//
/// Helper functions.
// TODO: Helper function. Needs to be moved to the right place.
// TODO: May be this function should place in "LoopInfo.h"
static Loop* findCommonLoop(Loop* A, Loop* B) {
  while (A && !A->contains(B))
    A = A->getParentLoop();

  return A;
}

//===----------------------------------------------------------------------===//
/// Statment implement.
void Statement::AllocateScattering(unsigned *value) {
  unsigned iterators = Scop->getLoopDepth(BB);
  unsigned scat_dims = Scop->getMaxLoopDepth() * 2 + 1;
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
  struct isl_dim *dim = isl_dim_set_alloc(Scop->isl_ctx, 0, NumIts);
  struct isl_constraint *c = isl_inequality_alloc(isl_dim_copy(dim));

  isl_int v;
  isl_int_init(v);

  if (Scop->getLoopDepth(L) == 0)
    return c;

  unsigned iterator = Scop->getLoopDepth(L) - 1;

  // i >= 0
  isl_int_set_si(v, 0);
  isl_constraint_set_constant(c, v);
  isl_int_set_si(v, 1);
  isl_constraint_set_coefficient(c, isl_dim_set, iterator, v);

  return c;
}

/// Create a constraint describing the upper bound of the Loop L.
struct isl_constraint *Statement::createUBConstraintForLoop(Loop *L) {
  struct isl_dim *dim = isl_dim_set_alloc(Scop->isl_ctx, 0, NumIts);
  struct isl_constraint *c = isl_inequality_alloc(isl_dim_copy(dim));

  isl_int v;
  isl_int_init(v);
  isl_int ub;
  isl_int_init(ub);

  if (NumIts == 0)
    return c;

  const SCEV *scev = Scop->SE->getBackedgeTakenCount(L);
  if (scConstant == scev->getSCEVType()) {
    const SCEVConstant *cscev = (SCEVConstant *) scev;
    const APInt apint = cscev->getValue()->getValue();
    MPZ_from_APInt (ub, apint);
  }
  // if backedge taken count is scCouldNotCompute?

  if (Scop->getLoopDepth(L) == 0)
    return c;

  unsigned iterator = Scop->getLoopDepth(L) - 1;

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
  struct isl_dim *dim = isl_dim_set_alloc(Scop->isl_ctx, 0, NumIts);
  struct isl_basic_set *bset = isl_basic_set_universe(isl_dim_copy(dim));

  Loop *Lo = Scop->LI->getLoopFor(BB);

  while (Lo && NumIts && Scop->getSurroundingLoop()->contains(Lo)) {
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

Statement::Statement(SCoPPass *SCoP, BasicBlock *BasicBlock, unsigned *value) {
  BB = BasicBlock;
  Scop = SCoP;
  NumIts = SCoP->getLoopDepth(BB);
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
  if (Domain) {
    isl_set_print(Domain, stderr, 20, ISL_FORMAT_ISL);
    DEBUG(isl_set_dump(Domain, stderr, 20));
  }
  else
    OS << "\t\t\tn/a\n";

  OS << "\n";

  OS << "\t\t Scattering:\n";
  if (Scattering) {
    isl_map_print(Scattering, stderr, 20, ISL_FORMAT_ISL);
    DEBUG(isl_map_dump(Scattering, stderr, 20));
  } else
    OS << "\t\t\tn/a\n";
}

//===----------------------------------------------------------------------===//
/// SCoPPass implement.
unsigned SCoPPass::getMaxLoopDepth() const {
  unsigned max = 0;
  Region *R = getRegion();

  for (Region::block_iterator BI = R->block_begin(), BE = R->block_end();
       BI != BE; ++BI)
    max = std::max(max, getLoopDepth((*BI)->getEntry()));

  return max;
}


void SCoPPass::createStmts() {
  // Is this the Dimension of scattering function?
  unsigned constc = getMaxLoopDepth() + 1;

  // The scattering function?
  unsigned *value = (unsigned *) malloc (sizeof (unsigned) * (constc + 1));
  for (unsigned i = 0; i < constc + 1; ++i)
    value[i] = 0;

  Loop *last = 0;
  // Use the element iterator.
  // Are we iterating the statments in a right order?
  for (Region::block_iterator BI = R->block_begin(), BE = R->block_end();
       BI != BE; ++BI) {
    Loop *L = LI->getLoopFor((*BI)->getEntry());

    // BI may not in any loop.
    Loop *CL = findCommonLoop(last, L);
    // Get the scatter position?
    int LD = getLoopDepth(CL);

    ++value[LD];

    Statement *stmt = new Statement(this, (*BI)->getEntry(), value);
    Stmts.insert(stmt);

    last = L;
  }
}

bool SCoPPass::isValid() {
  return valid;
}

SCoPPass::SCoPPass() : RegionPass(&ID) {
  isl_ctx = isl_ctx_alloc();
  // TODO: Support parameters.
  // Setup the dim of context with number of param and
  // constrain number of parameter?
  struct isl_dim *dim = isl_dim_set_alloc(isl_ctx, 0, 0);
  Context = isl_set_universe (dim);
  valid = true;
}

SCoPPass::~SCoPPass() {
  for (StmtSet::iterator SI = Stmts.begin(), SE = Stmts.end();
       SI != SE; ++SI)
    delete(*SI);

  isl_set_free(Context);

  // XXX: Cannot be removed as ctx might still be used in different passes.
  if (0)
    isl_ctx_free(isl_ctx);
}

bool SCoPPass::runOnRegion(Region *R, RGPassManager &RGM) {
    this->R = R;
    Stmts.clear();
    SE = &getAnalysis<ScalarEvolution>();
    LI = &getAnalysis<LoopInfo>();
    SD = &getAnalysis<SCoPDetection>();

    valid = SD->isMaxValid(R);

    if (!valid)
      return false;

    NumScatterDim = getMaxLoopDepth() * 2 + 1;

    createStmts();
    return false;
}

void SCoPPass::getAnalysisUsage(AnalysisUsage &AU) const {
    AU.setPreservesAll();
    AU.addRequired<ScalarEvolution>();
    AU.addRequired<LoopInfo>();
    AU.addRequired<SCoPDetection>();
}

unsigned SCoPPass::getNumParams() const {
  return isl_set_n_param(Context);
}

void SCoPPass::printContext(raw_ostream &OS) const {
  OS << "\tContext:\n";
  if (Context) {
    isl_set_print(Context, stderr, 12, ISL_FORMAT_ISL);
    DEBUG(isl_set_dump(Context, stderr, 12));
  }
  else
    OS << "\t\tn/a\n";

  OS << "\n";
}

void SCoPPass::printStatements(raw_ostream &OS) const {
  for (StmtSet::const_iterator SI = Stmts.begin(), SE = Stmts.end();
       SI != SE; ++SI)
    OS << (*SI) << "\n";
}

void SCoPPass::print(raw_ostream &OS, const Module *) const {
  if (!valid) {
    OS << "Invalid region\n";
    return;
  }

  printContext(OS);
  printStatements(OS);
}

Loop *SCoPPass::getSurroundingLoop() const {
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

char polly::SCoPPass::ID = 0;

static RegisterPass<SCoPPass>
X("polly-scop", "Polly - Create polyhedral SCoPs");

RegionPass* polly::createSCoPPass() {
  return new SCoPPass();
}
