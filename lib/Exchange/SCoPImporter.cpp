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

#include "polly/LinkAllPasses.h"
#include "polly/SCoPInfo.h"
#include "llvm/Support/CommandLine.h"

#define OPENSCOP_INT_T_IS_MP
#include "openscop/openscop.h"

#include "isl/isl_set.h"
#include "isl/isl_constraint.h"

using namespace llvm;
using namespace polly;

namespace {
static cl::opt<std::string>
ImportDir("polly-import-dir",
          cl::desc("The directory to import the .scop files from."), cl::Hidden,
          cl::value_desc("Directory path"), cl::ValueRequired, cl::init("."));
static cl::opt<std::string>
ImportPostfix("polly-import-postfix",
          cl::desc("Postfix to append to the import .scop files."), cl::Hidden,
          cl::value_desc("File postfix"), cl::ValueRequired, cl::init(""));

struct SCoPImporter : public RegionPass {
  static char ID;
  SCoP *S;
  explicit SCoPImporter() : RegionPass(ID) {}

  std::string getFileName(Region *R) const;
  virtual bool runOnRegion(Region *R, RGPassManager &RGM);
  virtual void print(raw_ostream &OS, const Module *) const;
  void getAnalysisUsage(AnalysisUsage &AU) const;
};

}

char SCoPImporter::ID = 0;

/// @brief Create an isl constraint from a row of OpenSCoP integers.
///
/// @param row An array of isl/OpenSCoP integers.
/// @param dim An isl dim object, describing how to spilt the dimensions.
///
/// @return An isl constraint representing this integer array.
isl_constraint *constraintFromMatrixRow(isl_int *row, isl_dim *dim) {
  polly_constraint *c;

  unsigned NbOut = isl_dim_size(dim, isl_dim_out);
  unsigned NbIn = isl_dim_size(dim, isl_dim_in);
  unsigned NbParam = isl_dim_size(dim, isl_dim_param);

  if (isl_int_is_zero(row[0]))
    c = isl_equality_alloc(isl_dim_copy(dim));
  else
    c = isl_inequality_alloc(isl_dim_copy(dim));

  unsigned current_column = 1;

  for (unsigned j = 0; j < NbOut; ++j)
    isl_constraint_set_coefficient(c, isl_dim_out, j, row[current_column++]);

  for (unsigned j = 0; j < NbIn; ++j)
    isl_constraint_set_coefficient(c, isl_dim_in, j, row[current_column++]);

  for (unsigned j = 0; j < NbParam; ++j)
    isl_constraint_set_coefficient(c, isl_dim_param, j, row[current_column++]);

  isl_constraint_set_constant(c, row[current_column]);

  return c;
}

/// @brief Create an isl map from a OpenSCoP matrix.
///
/// @param m The OpenSCoP matrix to translate.
/// @param dim The dimensions that are contained in the OpenSCoP matrix.
///
/// @return An isl map representing m.
isl_map *mapFromMatrix(openscop_matrix_p m, isl_dim *dim) {
  polly_basic_map *bmap = isl_basic_map_universe(isl_dim_copy(dim));

  for (unsigned i = 0; i < m->NbRows; ++i) {
    polly_constraint *c;

    c = constraintFromMatrixRow(m->p[i], dim);
    bmap = isl_basic_map_add_constraint(bmap, c);
  }

  return isl_map_from_basic_map(bmap);
}

/// @brief Create a new scattering for PollyStmt.
///
/// @param m The matrix describing the new scattering.
/// @param PollyStmt The statement to create the scattering for.
///
/// @return An isl_map describing the scattering.
isl_map *scatteringForStmt(openscop_matrix_p m, SCoPStmt *PollyStmt) {

  unsigned NbParam = PollyStmt->getNumParams();
  unsigned NbIterators = PollyStmt->getNumIterators();
  unsigned NbScattering = m->NbColumns - 2 - NbParam - NbIterators;

  isl_ctx *ctx = PollyStmt->getParent()->getCtx();
  polly_dim *dim = isl_dim_alloc(ctx, NbParam, NbIterators, NbScattering);
  isl_map *map = mapFromMatrix(m, dim);
  isl_dim_free(dim);

  return map;
}

/// @brief Update the scattering in PollyStmt.
///
/// @param PollyStmt The statement to update.
/// @param OStmt The OpenSCoP statement describing the new scattering.
void updateScattering(SCoPStmt *PollyStmt, openscop_statement_p OStmt) {
  assert(OStmt && "No openscop statement available");
  isl_map *m = scatteringForStmt(OStmt->schedule, PollyStmt);
  PollyStmt->setScattering(m);
}

/// @brief Update the scattering in a SCoP using the OpenSCoP description of
/// the scattering.
///
/// @S The SCoP to update
/// @OSCoP The OpenSCoP data structure describing the new scattering.
/// @return Returns false, if the update failed.
bool updateScattering(SCoP *S, openscop_scop_p OSCoP) {
  openscop_statement_p stmt = OSCoP->statement;

  for (SCoP::iterator SI = S->begin(), SE = S->end(); SI != SE; ++SI) {

    if (!stmt) {
      errs() << "Not enough statements available in OpenSCoP file\n";
      return false;
    }

    updateScattering(*SI, stmt);
    stmt = stmt->next;
  }

  if (stmt) {
    errs() << "Too many statements in OpenSCoP file\n";
    return false;
  }

  return true;
}

std::string SCoPImporter::getFileName(Region *R) const {
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
  std::string FileName = FunctionName + "___" + RegionName + ".scop";

  return FileName;
}

void SCoPImporter::print(raw_ostream &OS, const Module *) const {}

bool SCoPImporter::runOnRegion(Region *R, RGPassManager &RGM) {
  S = getAnalysis<SCoPInfo>().getSCoP();

  if (!S)
    return false;

  std::string FileName = ImportDir + "/" + getFileName(R) + ImportPostfix;
  FILE *F = fopen(FileName.c_str(), "r");

  if (!F) {
    errs() << "Cannot open file: " << FileName << "\n";
    errs() << "Skipping import.\n";
    return false;
  }

  openscop_scop_p scop = openscop_scop_read(F);
  fclose(F);

  std::string FunctionName = R->getEntry()->getParent()->getNameStr();
  errs() << "Reading SCoP '" << R->getNameStr() << "' in function '"
    << FunctionName << "' from '" << FileName << ImportPostfix << "'.\n";

  bool UpdateSuccessfull = updateScattering(S, scop);

  if (!UpdateSuccessfull) {
    errs() << "Update failed" << "\n";
  }

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
