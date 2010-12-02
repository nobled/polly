//===-- ScopLibImporter.cpp  - Import Scops with scoplib. -----------------===//
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

#ifdef SCOPLIB_FOUND

#include "polly/ScopInfo.h"
#include "polly/Dependences.h"
#include "llvm/Support/CommandLine.h"

#define SCOPLIB_INT_T_IS_MP
#include "scoplib/scop.h"

#include "isl/set.h"
#include "isl/constraint.h"

using namespace llvm;
using namespace polly;

namespace {
  static cl::opt<std::string>
    ImportDir("polly-import-scoplib-dir",
              cl::desc("The directory to import the .scoplib files from."),
              cl::Hidden, cl::value_desc("Directory path"), cl::ValueRequired,
              cl::init("."));
  static cl::opt<std::string>
    ImportPostfix("polly-import-scoplib-postfix",
                  cl::desc("Postfix to append to the import .scoplib files."),
                  cl::Hidden, cl::value_desc("File postfix"), cl::ValueRequired,
                  cl::init(""));

  struct ScopLibImporter : public RegionPass {
    static char ID;
    Scop *S;
    Dependences *D;
    explicit ScopLibImporter() : RegionPass(ID) {}

    bool updateScattering(Scop *S, scoplib_scop_p OScop);
      std::string getFileName(Region *R) const;
      virtual bool runOnRegion(Region *R, RGPassManager &RGM);
      virtual void print(raw_ostream &OS, const Module *) const;
      void getAnalysisUsage(AnalysisUsage &AU) const;
    };
}

char ScopLibImporter::ID = 0;

namespace {
/// @brief Create an isl constraint from a row of OpenScop integers.
///
/// @param row An array of isl/OpenScop integers.
/// @param dim An isl dim object, describing how to spilt the dimensions.
///
/// @return An isl constraint representing this integer array.
isl_constraint *constraintFromMatrixRow(isl_int *row, isl_dim *dim) {
  isl_constraint *c;

  unsigned NbIn = isl_dim_size(dim, isl_dim_in);
  unsigned NbParam = isl_dim_size(dim, isl_dim_param);

  if (isl_int_is_zero(row[0]))
    c = isl_equality_alloc(isl_dim_copy(dim));
  else
    c = isl_inequality_alloc(isl_dim_copy(dim));

  unsigned current_column = 1;

  for (unsigned j = 0; j < NbIn; ++j)
    isl_constraint_set_coefficient(c, isl_dim_in, j, row[current_column++]);

  for (unsigned j = 0; j < NbParam; ++j)
    isl_constraint_set_coefficient(c, isl_dim_param, j, row[current_column++]);

  isl_constraint_set_constant(c, row[current_column]);

  return c;
}

/// @brief Create an isl map from a OpenScop matrix.
///
/// @param m The OpenScop matrix to translate.
/// @param dim The dimensions that are contained in the OpenScop matrix.
///
/// @return An isl map representing m.
isl_map *mapFromMatrix(scoplib_matrix_p m, isl_dim *dim,
                       unsigned scatteringDims) {
  isl_basic_map *bmap = isl_basic_map_universe(isl_dim_copy(dim));

  for (unsigned i = 0; i < m->NbRows; ++i) {
    isl_constraint *c;

    c = constraintFromMatrixRow(m->p[i], dim);

    mpz_t minusOne;
    mpz_init(minusOne);
    mpz_set_si(minusOne, -1);
    isl_constraint_set_coefficient(c, isl_dim_out, i, minusOne);

    bmap = isl_basic_map_add_constraint(bmap, c);
  }

  for (unsigned i = m->NbRows; i < scatteringDims; i++) {
    isl_constraint *c;

    c = isl_equality_alloc(isl_dim_copy(dim));

    mpz_t One;
    mpz_init(One);
    mpz_set_si(One, 1);
    isl_constraint_set_coefficient(c, isl_dim_out, i, One);

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
isl_map *scatteringForStmt(scoplib_matrix_p m, ScopStmt *PollyStmt,
                           unsigned scatteringDims) {

  unsigned NbParam = PollyStmt->getNumParams();
  unsigned NbIterators = PollyStmt->getNumIterators();

  isl_ctx *ctx = PollyStmt->getParent()->getCtx();
  isl_dim *dim = isl_dim_alloc(ctx, NbParam, NbIterators, scatteringDims);
  dim = isl_dim_set_tuple_name(dim, isl_dim_out, "scattering");
  dim = isl_dim_set_tuple_name(dim, isl_dim_in, PollyStmt->getBaseName());
  isl_map *map = mapFromMatrix(m, dim, scatteringDims);

  isl_dim_free(dim);

  return map;
}

unsigned maxScattering(scoplib_statement_p stmt) {
  unsigned max = 0;

  while (stmt) {
    max = std::max(max, stmt->schedule->NbRows);
    stmt = stmt->next;
  }

  return max;
}

typedef Dependences::StatementToIslMapTy StatementToIslMapTy;

/// @brief Read the new scattering from the scoplib description.
///
/// @S      The Scop to update
/// @OScop  The ScopLib data structure describing the new scattering.
/// @return A map that contains for each Statement the new scattering.
StatementToIslMapTy *readScattering(Scop *S, scoplib_scop_p OScop) {
  StatementToIslMapTy &NewScattering = *(new StatementToIslMapTy());
  scoplib_statement_p stmt = OScop->statement;
  unsigned numScatteringDims = maxScattering(stmt);

  for (Scop::iterator SI = S->begin(), SE = S->end(); SI != SE; ++SI) {

    if ((*SI)->isFinalRead())
      continue;

    if (!stmt) {
      errs() << "Not enough statements available in OpenScop file\n";
      delete &NewScattering;
      return NULL;
    }

    NewScattering[*SI] = scatteringForStmt(stmt->schedule, *SI,
                                           numScatteringDims);
    stmt = stmt->next;
  }

  if (stmt) {
    errs() << "Too many statements in OpenScop file\n";
    delete &NewScattering;
    return NULL;
  }

  return &NewScattering;
}

/// @brief Update the scattering in a Scop using the scoplib description of
/// the scattering.
///
/// @S The Scop to update
/// @OScop The scoplib data structure describing the new scattering.
/// @return Returns false, if the update failed.
bool ScopLibImporter::updateScattering(Scop *S, scoplib_scop_p OScop) {
  StatementToIslMapTy *NewScattering = readScattering(S, OScop);

  if (!NewScattering)
    return false;

  if (!D->isValidScattering(NewScattering)) {
    errs() << "OpenScop file contains a scattering that changes the "
      << "dependences.\n";
    return false;
  }

  for (Scop::iterator SI = S->begin(), SE = S->end(); SI != SE; ++SI) {
    ScopStmt *Stmt = *SI;

    if (NewScattering->find(Stmt) != NewScattering->end())
      Stmt->setScattering((*NewScattering)[Stmt]);
  }

  return true;
}

std::string ScopLibImporter::getFileName(Region *R) const {
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
  std::string FileName = FunctionName + "___" + RegionName + ".scoplib";

  return FileName;
}

void ScopLibImporter::print(raw_ostream &OS, const Module *) const {}

bool ScopLibImporter::runOnRegion(Region *R, RGPassManager &RGM) {
  S = getAnalysis<ScopInfo>().getScop();
  D = &getAnalysis<Dependences>();

  if (!S)
    return false;

  std::string FileName = ImportDir + "/" + getFileName(R) + ImportPostfix;
  FILE *F = fopen(FileName.c_str(), "r");

  if (!F) {
    errs() << "Cannot open file: " << FileName << "\n";
    errs() << "Skipping import.\n";
    return false;
  }

  scoplib_scop_p scop = scoplib_scop_read(F);
  fclose(F);

  std::string FunctionName = R->getEntry()->getParent()->getNameStr();
  errs() << "Reading Scop '" << R->getNameStr() << "' in function '"
    << FunctionName << "' from '" << FileName << ImportPostfix << "'.\n";

  bool UpdateSuccessfull = updateScattering(S, scop);

  if (!UpdateSuccessfull) {
    errs() << "Update failed" << "\n";
  }

  return false;
}

void ScopLibImporter::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
  AU.addRequired<ScopInfo>();
  AU.addRequired<Dependences>();
}

}

static RegisterPass<ScopLibImporter> A("polly-import-scoplib",
                                    "Polly - Import Scops with ScopLib library"
                                    " (Reads a .scoplib file for each Scop)"
                                    );

Pass *polly::createScopLibImporterPass() {
  return new ScopLibImporter();
}

#endif
