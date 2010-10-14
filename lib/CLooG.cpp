//===- CLooG.cpp - CLooG interface ----------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// CLooG[1] interface.
//
// The CLooG interface takes a SCoP and generates a CLooG AST (clast). This
// clast can either be returned directly or it can be pretty printed to stdout.
//
// A typical clast output looks like this:
//
// for (c2 = max(0, ceild(n + m, 2); c2 <= min(511, floord(5 * n, 3)); c2++) {
//   bb2(c2);
// }
//
// [1] http://www.cloog.org/ - The Chunky Loop Generator
//
//===----------------------------------------------------------------------===//

#include "polly/CLooG.h"
#include "polly/LinkAllPasses.h"
#include "polly/SCoPInfo.h"

#include "cloog/isl/domain.h"

using namespace llvm;
using namespace polly;

namespace polly {

CLooG::CLooG(SCoP *Scop) : S(Scop) {
  State = cloog_state_malloc();
  buildCloogOptions();
  ClastRoot = cloog_clast_create_from_input(buildCloogInput(), Options);
}

CLooG::~CLooG() {
  cloog_options_free(Options);
  cloog_clast_free(ClastRoot);
  cloog_state_free(State);
}

/// Print a .cloog input file, that is equivalent to this program.
// TODO: use raw_ostream as parameter.
void CLooG::dump(FILE *F) {
  CloogInput *Input = buildCloogInput();
  cloog_input_dump_cloog(F, Input, Options);
  cloog_input_free(Input);
  fflush(F);
}

/// Print a source code representation of the program.
// TODO: use raw_ostream as parameter.
void CLooG::pprint() {
  clast_pprint(stdout, ClastRoot, 0, Options);
  fflush(stdout);
}

/// Create the CLooG AST from this program.
struct clast_stmt *CLooG::getClast() {
  return ClastRoot;
}

void CLooG::buildCloogOptions() {
  Options = cloog_options_malloc(State);
  Options->quiet = 1;
  Options->strides = 1;
}

CloogUnionDomain *CLooG::buildCloogUnionDomain() {
  CloogUnionDomain *DU = cloog_union_domain_alloc(S->getNumParams());

  for (SCoP::iterator SI = S->begin(), SE = S->end(); SI != SE; ++SI) {
    CloogScattering *Scattering=
      cloog_scattering_from_isl_map(isl_map_copy((*SI)->getScattering()));
    CloogDomain *Domain =
      cloog_domain_from_isl_set(isl_set_copy((*SI)->getDomain()));

    std::string entryName;
    raw_string_ostream OS(entryName);
    WriteAsOperand(OS, (*SI)->getBasicBlock(), false);
    entryName = OS.str();
    char *Name = (char*)malloc(sizeof(char) * (entryName.size() + 1));
    strcpy(Name, entryName.c_str());

    DU = cloog_union_domain_add_domain(DU, Name, Domain, Scattering, *SI);
  }

  return DU;
}

CloogInput *CLooG::buildCloogInput() {
  CloogDomain *Context =
    cloog_domain_from_isl_set(isl_set_copy(S->getContext()));
  CloogUnionDomain *Statements = buildCloogUnionDomain();
  CloogInput *Input = cloog_input_alloc (Context, Statements);
  return Input;
}
} // End namespace polly.

namespace {

struct CLooGExporter : public RegionPass {
  static char ID;
  SCoP *S;
  explicit CLooGExporter() : RegionPass(ID) {}

  std::string getFileName(Region *R) const;
  virtual bool runOnRegion(Region *R, RGPassManager &RGM);
  void getAnalysisUsage(AnalysisUsage &AU) const;
};

}
std::string CLooGExporter::getFileName(Region *R) const {
  std::string FunctionName = R->getEntry()->getParent()->getNameStr();
  std::string ExitName, EntryName;

  raw_string_ostream ExitStr(ExitName);
  raw_string_ostream EntryStr(EntryName);

  WriteAsOperand(EntryStr, R->getEntry(), false);
  EntryStr.str();

  if (R->getExit()) {
    WriteAsOperand(ExitStr, R->getExit(), false);
    ExitStr.str();
  } else
    ExitName = "FunctionExit";

  std::string RegionName = EntryName + "---" + ExitName;
  std::string FileName = FunctionName + "___" + RegionName + ".cloog";

  return FileName;
}

char CLooGExporter::ID = 0;
bool CLooGExporter::runOnRegion(Region *R, RGPassManager &RGM) {
  S = getAnalysis<SCoPInfo>().getSCoP();

  if (!S)
    return false;

  std::string FunctionName = R->getEntry()->getParent()->getNameStr();
  std::string Filename = getFileName(R);

  errs() << "Writing SCoP '" << R->getNameStr() << "' in function '"
    << FunctionName << "' to '" << Filename << "'...\n";

  FILE *F = fopen(Filename.c_str(), "w");

  CLooG C(S);
  C.dump(F);
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

llvm::Pass* polly::createCLooGExporterPass() {
  return new CLooGExporter();
}

