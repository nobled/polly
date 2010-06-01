//===- CLooG.cpp - CLooG interface connector. -----------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// CLooG interface connection.
//
//===----------------------------------------------------------------------===//

#include "polly/CLooG.h"
#include "polly/CLooGExporter.h"
#include "polly/SCoPInfo.h"
#include "cloog/cloog.h"
#include "cloog/isl/domain.h"

#ifdef _WINDOWS
#define snprintf _snprintf
#endif

using namespace llvm;
using namespace polly;

namespace polly {

CLooG::CLooG(SCoP *Scop) : S(Scop) {
  StatementNumber = 0;
  State = cloog_state_malloc();
  buildCloogOptions();
  buildCloogProgram();
}

void CLooG::generate() {
  ScatterProgram();

  // XXX: Necessary as otherwise cloog would free the blocks twice.
  // TODO: Free the blocklist ourselves.
  Program->blocklist = 0;

  Program = cloog_program_generate(Program, Options);
}

void CLooG::ScatterProgram() {
  CloogScatteringList *scattering = buildScatteringList();

  //Extract scalar dimensions to simplify the code generation problem.
  cloog_program_extract_scalars (Program, scattering, Options);

  // Apply scattering.
  cloog_program_scatter (Program, scattering, Options);

  // Iterators corresponding to scalar dimensions have to be extracted.
  cloog_names_scalarize (Program->names, Program->nb_scattdims,
			 Program->scaldims);
}

CLooG::~CLooG() {
  cloog_program_free(Program);
  cloog_options_free(Options);
  cloog_state_free(State);
}

/// Print a .cloog input file, that is equivalent to this program.
// TODO: use raw_ostream as parameter.
void CLooG::dump(FILE *F) {
  cloog_program_dump_cloog(F, Program);
  fflush(F);
}

/// Print a source code representation of the program.
// TODO: use raw_ostream as parameter.
void CLooG::pprint() {
  cloog_program_pprint(stdout, Program, Options);
  fflush(stdout);
}

/// Print the content of the Program data structure.
// TODO: use raw_ostream as parameter.
void CLooG::print() {
  cloog_program_print(stdout, Program);
  fflush(stdout);
}

/// Create the CLooG AST from this program.
struct clast_stmt *CLooG::getClast() {
  return cloog_clast_create (Program, Options);
}

void CLooG::buildCloogOptions() {
  Options = cloog_options_malloc(State);
  Options->quiet = 1;
  Options->strides = 1;
}

/// Allocate a CloogLoop data structure containing information about stmt.
CloogLoop *CLooG::buildCloogLoop(SCoPStmt* stmt) {
  CloogStatement *Statement = cloog_statement_malloc(State);
  Statement->number = StatementNumber++;
  Statement->usr = stmt;

  CloogBlock *Block = cloog_block_alloc(Statement, 0, NULL, 1);
  CloogDomain *Domain =
    cloog_domain_from_isl_set(isl_set_copy(stmt->getDomain()));

  CloogLoop *Loop = cloog_loop_malloc(State);
  Loop->domain = Domain;
  Loop->block = Block;

  return Loop;
}

/// Create a list of CloogLoops containing the statements of the SCoP.
CloogLoop *CLooG::buildCloogLoopList() {
  CloogLoop *Loop = 0;

  for (SCoP::iterator SI = S->begin(), SE =
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
CloogScatteringList *CLooG::buildScatteringList() {
  CloogScatteringList *ScatteringList = 0;

  for (SCoP::iterator SI = S->begin(), SE =
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
CloogNames *CLooG::buildCloogNames(unsigned nb_scalars,
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

int *CLooG::buildScaldims(CloogProgram *Program) const {
  int *scaldims = (int *) malloc (Program->nb_scattdims * (sizeof (int)));
  for (int i = 0; i < Program->nb_scattdims; i++)
    scaldims[i] = 0 ;

  return scaldims;
}

CloogBlockList *CLooG::buildCloogBlockList(CloogLoop *LL) {
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

void CLooG::buildCloogProgram() {
  Program = cloog_program_malloc();
  Program->context = cloog_domain_from_isl_set(isl_set_copy(S->getContext()));
  Program->loop = buildCloogLoopList();

  // XXX: Not necessary? Check with the CLooG guys.
  Program->blocklist = buildCloogBlockList(Program->loop);

  // TODO: * Replace constants.
  //       * Support parameters.
  Program->names = buildCloogNames(0, S->getScatterDim(), S->getMaxLoopDepth(),
				   S->getNumParams());

  // XXX: Not sure if the next two stmts are necessary.  Check with CLooG
  // guys.
  Program->nb_scattdims = S->getScatterDim();
  Program->scaldims = buildScaldims(Program);
}
}


namespace {

struct CLooGExporter : public RegionPass {
  static char ID;
  SCoP *S;
  explicit CLooGExporter() : RegionPass(&ID) {}

  virtual bool runOnRegion(Region *R, RGPassManager &RGM);
  void getAnalysisUsage(AnalysisUsage &AU) const;
};

}

char CLooGExporter::ID = 0;
bool CLooGExporter::runOnRegion(Region *R, RGPassManager &RGM) {
  S = getAnalysis<SCoPInfo>().getSCoP();

  if (!S)
    return false;

  std::string FunctionName = R->getEntry()->getParent()->getNameStr();
  std::string Filename = FunctionName + "_-_" + R->getNameStr() + ".cloog";

  errs() << "Writing SCoP '" << R->getNameStr() << "' in function '"
    << FunctionName << "' to '" << Filename << "'...\n";

  FILE *F = fopen(Filename.c_str(), "w");

  CLooG C(S);
  C.dump(F);
  C.pprint();
  fclose(F);

  return false;
}

void CLooGExporter::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
  AU.addRequired<SCoPInfo>();
}

static RegisterPass<CLooGExporter> A("polly-export-cloog",
                                    "Polly - Export the CLooG input file"
                                    " (Writes a .cloog file for each SCoP)"
                                    );

namespace polly {
  llvm::RegionPass *createCLooGExporterPass() {
    return new CLooGExporter();
  }
}

