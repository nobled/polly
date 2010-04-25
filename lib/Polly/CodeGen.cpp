//===- CodeGen.cpp - Recreate LLVM IR from the SCoP.  ---------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Recreate LLVM IR from the SCoP.
//
//===----------------------------------------------------------------------===//

#include "polly/SCoPPass.h"
#include "llvm/Support/CFG.h"
#include "llvm/Support/Debug.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#define CLOOG_INT_GMP 1
#include "cloog/cloog.h"
#include "cloog/isl/domain.h"

#ifdef _WINDOWS
#define snprintf _snprintf
#endif

using namespace polly;
using namespace llvm;

namespace {

static void createSingleEntryEdge(Region *R) {
  BasicBlock *BB = R->getEntry();

  if (BB->getSinglePredecessor())
    return;

  BasicBlock::iterator SplitIt = BB->begin();

  while (isa<PHINode>(SplitIt))
    ++SplitIt;

  BasicBlock *newBB = BB->splitBasicBlock(SplitIt, BB->getName()+".region");

  for (pred_iterator PI = pred_begin(BB), PE = pred_end(BB); PI != PE; ++PI)
    (*PI)->getTerminator()->replaceUsesOfWith(BB, newBB);

  for (BasicBlock::iterator PI = BB->begin(); isa<PHINode>(PI); ++PI) {
    PHINode *PN = cast<PHINode>(PI);
    PHINode *NPN =
        PHINode::Create(PN->getType(), PN->getName()+".ph", newBB->begin());

    for (pred_iterator PI = pred_begin(BB), PE = pred_end(BB); PI != PE; ++PI) {
      if (R->contains(*PI)) {
        Value *V = PN->removeIncomingValue(*PI, false);
        NPN->addIncoming(V, *PI);
      }
    }
    PN->replaceAllUsesWith(NPN);
    NPN->addIncoming(PN,BB);
 }
}

static void createSingleExitEdge(Region *R, Pass *P) {
  BasicBlock *BB = R->getExit();
  int num = 0, i = 0;

  for (pred_iterator PI = pred_begin(BB), PE = pred_end(BB); PI != PE; ++PI)
    if (R->contains(*PI))
      ++num;

  BasicBlock **Preds = new BasicBlock *[num];

  for (pred_iterator PI = pred_begin(BB), PE = pred_end(BB); PI != PE; ++PI) {
    if (R->contains(*PI))
      Preds[i] = *PI;
    ++i;
  }

  SplitBlockPredecessors(BB, Preds, num, ".region", P);

  delete Preds;
}

/// SplitBlockPredecessors - This method transforms BB by introducing a new
/// basic block into the function, and moving some of the predecessors of BB to
/// be predecessors of the new block.  The new predecessors are indicated by the
/// Preds array, which has NumPreds elements in it.  The new block is given a
/// suffix of 'Suffix'.
///
/// This currently updates the LLVM IR, AliasAnalysis, DominatorTree,
/// DominanceFrontier, LoopInfo, and LCCSA but no other analyses.
/// In particular, it does not preserve LoopSimplify (because it's
/// complicated to handle the case where one of the edges being split
/// is an exit of a loop with other exits).
///

/// CLooG interface to convert from a SCoP back to LLVM-IR.
class CLooG {
  SCoPPass *S;
  CloogProgram *Program;
  CloogOptions *Options;
  CloogState *State;

  unsigned StatementNumber;

public:
  CLooG(SCoPPass *Scop) : S(Scop) {
    StatementNumber = 0;
    State = cloog_state_malloc();
    buildCloogOptions();
    buildCloogProgram();
    ScatterProgram();

    // XXX: Necessary as otherwise cloog would free the blocks twice.
    // TODO: Free the blocklist ourselves.
    Program->blocklist = 0;

    Program = cloog_program_generate(Program, Options);
  }

  void ScatterProgram() {
    CloogScatteringList *scattering = buildScatteringList();

    //Extract scalar dimensions to simplify the code generation problem.
    cloog_program_extract_scalars (Program, scattering, Options);

    // Apply scattering.
    cloog_program_scatter (Program, scattering, Options);

    // Iterators corresponding to scalar dimensions have to be extracted.
    cloog_names_scalarize (Program->names, Program->nb_scattdims,
                           Program->scaldims);
  }

  ~CLooG() {
    cloog_program_free(Program);
    cloog_options_free(Options);
    cloog_state_free(State);
  }

  /// Print a .cloog input file, that is equivalent to this program.
  // TODO: use raw_ostream as parameter.
  void dump() {
    cloog_program_dump_cloog(stdout, Program);
    fflush(stdout);
  }

  /// Print a source code representation of the program.
  // TODO: use raw_ostream as parameter.
  void pprint() {
    cloog_program_pprint(stdout, Program, Options);
    fflush(stdout);
  }

  /// Print the content of the Program data structure.
  // TODO: use raw_ostream as parameter.
  void print() {
    cloog_program_print(stdout, Program);
    fflush(stdout);
  }

  /// Create the CLooG AST from this program.
  struct clast_stmt *getClast() {
    return cloog_clast_create (Program, Options);
  }

  void buildCloogOptions() {
    Options = cloog_options_malloc(State);
    Options->quiet = 1;
  }

  /// Allocate a CloogLoop data structure containing information about stmt.
  CloogLoop *buildCloogLoop(Statement* stmt) {
    CloogStatement *Statement = cloog_statement_malloc(State);
    Statement->number = StatementNumber++;

    CloogBlock *Block = cloog_block_alloc(Statement, 0, NULL, 1);
    CloogDomain *Domain =
      cloog_domain_from_isl_set(isl_set_copy(stmt->getDomain()));

    CloogLoop *Loop = cloog_loop_malloc(State);
    Loop->domain = Domain;
    Loop->block = Block;

    return Loop;
  }

  /// Create a list of CloogLoops containing the statements of the SCoP.
  CloogLoop *buildCloogLoopList() {
    CloogLoop *Loop = 0;

    for (SCoPPass::iterator SI = S->begin(), SE =
         S->end(); SI != SE; ++SI) {
      CloogLoop *NextLoop = buildCloogLoop(*SI);
      NextLoop->next = Loop;
      Loop = NextLoop;
    }

    return Loop;
  }

  /// Allocate a CloogScatteringList data structure and fill it with the
  /// scattering polyhedron of all statements in the SCoP. Ordered as they
  /// appear in the SCoP statement iterator.
  CloogScatteringList *buildScatteringList() {
    CloogScatteringList *ScatteringList = 0;

    for (SCoPPass::iterator SI = S->begin(), SE =
         S->end(); SI != SE; ++SI) {
      // XXX: cloog_domain_list_alloc() not implemented in CLooG.
      CloogScatteringList *NewScatteringList
        = (CloogScatteringList *) malloc (sizeof (CloogScatteringList));

      CloogScattering *Scattering=
        cloog_scattering_from_isl_map(isl_map_copy((*SI)->getScattering()));

      NewScatteringList->scatt = Scattering;
      NewScatteringList->next = ScatteringList;
      ScatteringList = NewScatteringList;
    }

    return ScatteringList;
  }

  /// Allocate a CloogNames data structure and fill it with default names.
  CloogNames *buildCloogNames(unsigned nb_scalars,
                              unsigned nb_scattering,
                              unsigned nb_iterators,
                              unsigned nb_parameters) const {
    unsigned i;
    CloogNames *Names = cloog_names_malloc();

    // XXX: This works. Never thought about the optimal length.
    unsigned static const length = 20;

    char **scalars;
    scalars = (char **) malloc (sizeof (char*) * nb_scalars);
    for (i = 0; i < nb_scalars; i++) {
      scalars[i] = (char *) malloc (sizeof (char) * length);
      snprintf (scalars[i], length, "scal%d", i);
    }

    char **scattering;
    scattering = (char **) malloc (sizeof (char*) * nb_scattering);
    for (i = 0; i < nb_scattering; i++) {
      scattering[i] = (char *) malloc (sizeof (char) * length);
      snprintf (scattering[i], length, "s%d", i);
    }

    char **iterators;
    iterators = (char **) malloc (sizeof (char*) * nb_iterators);
    for (i = 0; i < nb_iterators; i++) {
      iterators[i] = (char *) malloc (sizeof (char) * length);
      snprintf (iterators[i], length, "i%d", i);
    }

    char **parameters;
    parameters = (char **) malloc (sizeof (char*) * nb_parameters);
    for (i = 0; i < nb_parameters; i++) {
      parameters[i] = (char *) malloc (sizeof (char) * length);
      snprintf (parameters[i], length, "p%d", i);
    }

    Names->nb_scalars = nb_scalars;
    Names->nb_scattering = nb_scattering;
    Names->nb_iterators = nb_iterators;
    Names->nb_parameters = nb_parameters;
    Names->references = 1;

    Names->scalars = scalars;
    Names->scattering = scattering;
    Names->iterators = iterators;
    Names->parameters = parameters;

    return Names;
  }

  int *buildScaldims(CloogProgram *Program) const {
    int *scaldims = (int *) malloc (Program->nb_scattdims * (sizeof (int)));
    for (int i = 0; i < Program->nb_scattdims; i++)
      scaldims[i] = 0 ;

    return scaldims;
  }

  CloogBlockList *buildCloogBlockList(CloogLoop *LL) {
    CloogBlockList *List = cloog_block_list_malloc();
    CloogBlockList *Start = List;
    do {
      List->block = LL->block;

      if (LL->next)
        List->next = cloog_block_list_malloc();
      else
        List->next = 0;

      LL = LL->next;

    } while (LL);

    return Start;
  }

  void buildCloogProgram() {
    Program = cloog_program_malloc();
    Program->context = cloog_domain_from_isl_set(isl_set_copy(S->Context));
    Program->loop = buildCloogLoopList();

    // XXX: Not necessary? Check with the CLooG guys.
    Program->blocklist = buildCloogBlockList(Program->loop);

    // TODO: * Replace constants.
    //       * Support parameters.
    Program->names = buildCloogNames(0, S->NumScatterDim, S->getMaxLoopDepth(),
                                     0);

    // XXX: Not sure if the next two stmts are necessary.  Check with CLooG
    // guys.
    Program->nb_scattdims = S->NumScatterDim;
    Program->scaldims = buildScaldims(Program);
  }
};


class ScopPrinter : public RegionPass {

  Region *region;
  SCoPPass *S;

public:
  static char ID;

  ScopPrinter() : RegionPass(&ID) {}

  bool runOnRegion(Region *R, RGPassManager &RGM) {
    region = R;
    S = &getAnalysis<SCoPPass>();
    return false;
  }

  void print(raw_ostream &OS, const Module *) const {

    if (!S->isValid()) {
      OS << "Invalid SCoP\n";
      return;
    }

    CLooG C = CLooG(S);
    struct clast_stmt *clast = C.getClast();
    OS << "Generated CLAST '" << clast << "'\n";
    C.pprint();
  }

  virtual void getAnalysisUsage(AnalysisUsage &AU) const {
    // XXX: Cannot be removed, as otherwise LLVM crashes.
    AU.setPreservesAll();
    AU.addRequired<SCoPPass>();
  }
};
} //end anonymous namespace

char ScopPrinter::ID = 0;

static RegisterPass<ScopPrinter>
X("polly-print-scop", "Polly - Print SCoP as C code");

RegionPass* polly::createScopPrinterPass() {
  return new ScopPrinter();
}

namespace {
class ScopCodeGen : public RegionPass {

  Region *region;
  SCoPPass *S;

public:
  static char ID;

  ScopCodeGen() : RegionPass(&ID) {}

  void insertNewCodeBranch() {
  }

  bool runOnRegion(Region *R, RGPassManager &RGM) {
    region = R;
    S = &getAnalysis<SCoPPass>();

    if (!S->isValid())
      return false;

    createSingleEntryEdge(R);
    createSingleExitEdge(R, this);
    insertNewCodeBranch();

    return true;
  }

  void print(raw_ostream &OS, const Module *) const {
  }

  virtual void getAnalysisUsage(AnalysisUsage &AU) const {
    // XXX: Cannot be removed, as otherwise LLVM crashes.
    AU.addRequired<SCoPPass>();
  }
};
} //end anonymous namespace

char ScopCodeGen::ID = 0;

static RegisterPass<ScopCodeGen>
Y("polly-codegen-scop", "Polly - Code generation");

RegionPass* polly::createScopCodeGenPass() {
  return new ScopCodeGen();
}
