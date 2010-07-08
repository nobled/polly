//===-- SCoPExporter.cpp  - Export SCoPs with openscop library --*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Export the SCoPs build by SCoPInfo pass to text file.
//
//===----------------------------------------------------------------------===//

#include "polly/LinkAllPasses.h"
#include "polly/SCoPInfo.h"

#include "llvm/Support/CommandLine.h"

#define OPENSCOP_INT_T_IS_MP
#include "openscop/openscop.h"

#include "stdio.h"
#include "isl/isl_set.h"
#include "isl/isl_constraint.h"

using namespace llvm;
using namespace polly;

namespace {
static cl::opt<std::string>
ExportDir("polly-export-dir",
          cl::desc("The directory to export the .scop files to."), cl::Hidden,
          cl::value_desc("Directory path"), cl::ValueRequired, cl::init("."));

struct SCoPExporter : public RegionPass {
  static char ID;
  SCoP *S;
  explicit SCoPExporter() : RegionPass(&ID) {}

  std::string getFileName(Region *R) const;

  virtual bool runOnRegion(Region *R, RGPassManager &RGM);
  void print(raw_ostream &OS, const Module *) const;
  void getAnalysisUsage(AnalysisUsage &AU) const;
};

}

char SCoPExporter::ID = 0;

class OpenSCoP {
  SCoP *PollySCoP;
  openscop_scop_p openscop;

  std::map<const Value*, int> ArrayMap;

  void initializeArrays();
  void initializeParameters();
  void initializeScattering();
  void initializeStatements();
  openscop_statement_p initializeStatement(SCoPStmt *stmt);
  void freeStatement(openscop_statement_p stmt);
  static int accessToMatrix_constraint(isl_constraint *c, void *user);
  static int accessToMatrix_basic_map(isl_basic_map *bmap, void *user);
  openscop_matrix_p createAccessMatrix(SCoPStmt *S, bool isRead);
  static int domainToMatrix_constraint(isl_constraint *c, void *user);
  static int domainToMatrix_basic_set(isl_basic_set *bset, void *user);
  openscop_matrix_p domainToMatrix(polly_set *PS);
  static int scatteringToMatrix_constraint(isl_constraint *c, void *user);
  static int scatteringToMatrix_basic_map(isl_basic_map *bmap, void *user);
  openscop_matrix_p scatteringToMatrix(polly_map *pmap);

public:
  OpenSCoP(SCoP *S);
  ~OpenSCoP();
  void print(FILE *F);

};

OpenSCoP::OpenSCoP(SCoP *S) : PollySCoP(S) {
  openscop = openscop_scop_malloc();

  initializeArrays();
  initializeParameters();
  initializeScattering();
  initializeStatements();
}

void OpenSCoP::initializeParameters() {
  openscop->nb_parameters = PollySCoP->getNumParams();
  openscop->parameters = new char*[openscop->nb_parameters];

  for (int i = 0; i < openscop->nb_parameters; ++i) {
    openscop->parameters[i] = new char[20];
    sprintf(openscop->parameters[i], "p_%d", i);
  }
}

void OpenSCoP::initializeArrays() {
  int nb_arrays = 0;

  for (SCoP::iterator SI = PollySCoP->begin(), SE = PollySCoP->end(); SI != SE;
       ++SI)
    for (SCoPStmt::memacc_iterator MI = (*SI)->memacc_begin(),
         ME = (*SI)->memacc_end(); MI != ME; ++MI) {
      const Value *BaseAddr = (*MI)->getBaseAddr();
      if (ArrayMap.find(BaseAddr) == ArrayMap.end()) {
        ArrayMap.insert(std::make_pair(BaseAddr, nb_arrays));
        ++nb_arrays;
      }
    }

  openscop->nb_arrays = nb_arrays;
  openscop->arrays = new char*[nb_arrays];

  for (int i = 0; i < nb_arrays; ++i)
    for (std::map<const Value*, int>::iterator VI = ArrayMap.begin(),
         VE = ArrayMap.end(); VI != VE; ++VI)
      if ((*VI).second == i) {
        const Value *V = (*VI).first;
        std::string name = V->getNameStr();
        openscop->arrays[i] = new char[name.size() + 1];
        strcpy(openscop->arrays[i], name.c_str());
      }
}

void OpenSCoP::initializeScattering() {
  openscop->nb_scattdims = PollySCoP->getScatterDim();
  openscop->scattdims = new char*[openscop->nb_scattdims];

  for (int i = 0; i < openscop->nb_scattdims; ++i) {
    openscop->scattdims[i] = new char[20];
    sprintf(openscop->scattdims[i], "s_%d", i);
  }
}

openscop_statement_p OpenSCoP::initializeStatement(SCoPStmt *stmt) {
  openscop_statement_p Stmt = openscop_statement_malloc();

  // Domain & Schedule
  Stmt->domain = domainToMatrix(stmt->getDomain());
  Stmt->schedule = scatteringToMatrix(stmt->getScattering());

  // Statement name
  std::string str = stmt->getBasicBlock()->getNameStr();
  Stmt->body = new char[str.size() + 1];
  strcpy(Stmt->body, str.c_str());

  // Iterator names
  Stmt->nb_iterators = isl_set_n_dim(stmt->getDomain());
  Stmt->iterators = new char*[Stmt->nb_iterators];

  for (int i = 0; i < Stmt->nb_iterators; ++i) {
    Stmt->iterators[i] = new char[20];
    sprintf(Stmt->iterators[i], "i_%d", i);
  }

  // Memory Accesses
  Stmt->read = createAccessMatrix(stmt, true);
  Stmt->write = createAccessMatrix(stmt, false);

  return Stmt;
}

void OpenSCoP::initializeStatements() {
  for (SCoP::reverse_iterator SI = PollySCoP->rbegin(), SE = PollySCoP->rend();
       SI != SE; ++SI) {
    openscop_statement_p stmt = initializeStatement(*SI);
    stmt->next = openscop->statement;
    openscop->statement = stmt;
  }
}

void OpenSCoP::freeStatement(openscop_statement_p stmt) {

  if (stmt->read)
    openscop_matrix_free(stmt->read);
  stmt->read = NULL;

  if (stmt->write)
    openscop_matrix_free(stmt->write);
  stmt->write = NULL;

  if (stmt->domain)
    openscop_matrix_free(stmt->domain);
  stmt->domain = NULL;

  if (stmt->schedule)
    openscop_matrix_free(stmt->schedule);
  stmt->schedule = NULL;

  for (int i = 0; i < stmt->nb_iterators; ++i)
    delete[](stmt->iterators[i]);

  delete[](stmt->iterators);
  stmt->iterators = NULL;
  stmt->nb_iterators = 0;

  delete[](stmt->body);
  stmt->body = NULL;

  openscop_statement_free(stmt);
}

void OpenSCoP::print(FILE *F) {
  openscop_scop_print_dot_scop(F, openscop);
}

/// Add an isl constraint to an OpenSCoP matrix.
///
/// @param user The matrix
/// @param c The constraint
int OpenSCoP::domainToMatrix_constraint(isl_constraint *c, void *user) {
  openscop_matrix_p m = (openscop_matrix_p) user;

  int nb_params = isl_constraint_dim(c, isl_dim_param);
  int nb_vars = isl_constraint_dim(c, isl_dim_set);
  int nb_div = isl_constraint_dim(c, isl_dim_div);

  assert(!nb_div && "Existentially quantified variables not yet supported");

  openscop_vector_p vec = openscop_vector_malloc(nb_params + nb_vars + 2);

  // Assign type
  if (isl_constraint_is_equality(c))
    openscop_vector_tag_equality(vec);
  else
    openscop_vector_tag_inequality(vec);

  isl_int v;
  isl_int_init(v);

  // Assign variables
  for (int i = 0; i < nb_vars; ++i) {
    isl_constraint_get_coefficient(c, isl_dim_set, i, &v);
    isl_int_set(vec->p[i + 1], v);
  }

  // Assign parameters
  for (int i = 0; i < nb_params; ++i) {
    isl_constraint_get_coefficient(c, isl_dim_param, i, &v);
    isl_int_set(vec->p[nb_vars + i + 1], v);
  }

  // Assign constant
  isl_constraint_get_constant(c, &v);
  isl_int_set(vec->p[nb_params + nb_vars + 1], v);

  openscop_matrix_insert_vector(m, vec, m->NbRows);

  return 0;
}

/// Add an isl basic set to a OpenSCoP matrix_list
///
/// @param bset The basic set to add
/// @param user The matrix list we should add the basic set to
///
/// XXX: At the moment this function expects just a matrix, as support
/// for matrix lists is currently not available in OpenSCoP. So union of
/// polyhedron are not yet supported
int OpenSCoP::domainToMatrix_basic_set(isl_basic_set *bset, void *user) {
  openscop_matrix_p m = (openscop_matrix_p) user;
  assert(!m->NbRows && "Union of polyhedron not yet supported");

  isl_basic_set_foreach_constraint(bset, &domainToMatrix_constraint, user);
  return 0;
}

/// Translate a isl_set to a OpenSCoP matrix.
///
/// @param PS The set to be translated
/// @return A OpenSCoP Matrix
openscop_matrix_p OpenSCoP::domainToMatrix(polly_set *PS) {

  // Create a canonical copy of this set.
  polly_set *set = isl_set_copy(PS);
  set = isl_set_compute_divs (set);
  set = isl_set_align_divs (set);

  // Initialize the matrix.
  unsigned NbRows, NbColumns;
  NbRows = 0;
  NbColumns = isl_set_n_dim(PS) + isl_set_n_param(PS) + 2;
  openscop_matrix_p matrix = openscop_matrix_malloc(NbRows, NbColumns);

  // Copy the content into the matrix.
  isl_set_foreach_basic_set(set, &domainToMatrix_basic_set, matrix);

  isl_set_free(set);

  return matrix;
}

/// Add an isl constraint to an OpenSCoP matrix.
///
/// @param user The matrix
/// @param c The constraint
int OpenSCoP::scatteringToMatrix_constraint(isl_constraint *c, void *user) {
  openscop_matrix_p m = (openscop_matrix_p) user;

  int nb_params = isl_constraint_dim(c, isl_dim_param);
  int nb_in = isl_constraint_dim(c, isl_dim_in);
  int nb_out = isl_constraint_dim(c, isl_dim_out);
  int nb_div = isl_constraint_dim(c, isl_dim_div);

  assert(!nb_div && "Existentially quantified variables not yet supported");

  openscop_vector_p vec =
    openscop_vector_malloc(nb_params + nb_in + nb_out + 2);

  // Assign type
  if (isl_constraint_is_equality(c))
    openscop_vector_tag_equality(vec);
  else
    openscop_vector_tag_inequality(vec);

  isl_int v;
  isl_int_init(v);

  // Assign scattering
  for (int i = 0; i < nb_out; ++i) {
    isl_constraint_get_coefficient(c, isl_dim_out, i, &v);
    isl_int_set(vec->p[i + 1], v);
  }

  // Assign variables
  for (int i = 0; i < nb_in; ++i) {
    isl_constraint_get_coefficient(c, isl_dim_in, i, &v);
    isl_int_set(vec->p[nb_out + i + 1], v);
  }

  // Assign parameters
  for (int i = 0; i < nb_params; ++i) {
    isl_constraint_get_coefficient(c, isl_dim_param, i, &v);
    isl_int_set(vec->p[nb_out + nb_in + i + 1], v);
  }

  // Assign constant
  isl_constraint_get_constant(c, &v);
  isl_int_set(vec->p[nb_out + nb_in + nb_params + 1], v);

  openscop_matrix_insert_vector(m, vec, m->NbRows);

  return 0;
}

/// Add an isl basic map to a OpenSCoP matrix_list
///
/// @param bmap The basic map to add
/// @param user The matrix list we should add the basic map to
///
/// XXX: At the moment this function expects just a matrix, as support
/// for matrix lists is currently not available in OpenSCoP. So union of
/// polyhedron are not yet supported
int OpenSCoP::scatteringToMatrix_basic_map(isl_basic_map *bmap, void *user) {
  openscop_matrix_p m = (openscop_matrix_p) user;
  assert(!m->NbRows && "Union of polyhedron not yet supported");

  isl_basic_map_foreach_constraint(bmap, &scatteringToMatrix_constraint, user);
  return 0;
}

/// Translate a isl_map to a OpenSCoP matrix.
///
/// @param map The map to be translated
/// @return A OpenSCoP Matrix
openscop_matrix_p OpenSCoP::scatteringToMatrix(polly_map *pmap) {

  // Create a canonical copy of this set.
  polly_map *map = isl_map_copy(pmap);
  map = isl_map_compute_divs (map);
  map = isl_map_align_divs (map);

  // Initialize the matrix.
  unsigned NbRows, NbColumns;
  NbRows = 0;
  NbColumns = isl_map_n_in(pmap) + isl_map_n_out(pmap) + isl_map_n_param(pmap)
    + 2;
  openscop_matrix_p matrix = openscop_matrix_malloc(NbRows, NbColumns);

  // Copy the content into the matrix.
  isl_map_foreach_basic_map(map, &scatteringToMatrix_basic_map, matrix);

  isl_map_free(map);

  return matrix;
}

/// Add an isl constraint to an OpenSCoP matrix.
///
/// @param user The matrix
/// @param c The constraint
int OpenSCoP::accessToMatrix_constraint(isl_constraint *c, void *user) {
  openscop_matrix_p m = (openscop_matrix_p) user;

  int nb_params = isl_constraint_dim(c, isl_dim_param);
  int nb_in = isl_constraint_dim(c, isl_dim_in);
  int nb_div = isl_constraint_dim(c, isl_dim_div);

  assert(!nb_div && "Existentially quantified variables not yet supported");

  openscop_vector_p vec =
    openscop_vector_malloc(nb_params + nb_in + 2);

  isl_int v;
  isl_int_init(v);

  // Assign variables
  for (int i = 0; i < nb_in; ++i) {
    isl_constraint_get_coefficient(c, isl_dim_in, i, &v);
    isl_int_set(vec->p[i + 1], v);
  }

  // Assign parameters
  for (int i = 0; i < nb_params; ++i) {
    isl_constraint_get_coefficient(c, isl_dim_param, i, &v);
    isl_int_set(vec->p[nb_in + i + 1], v);
  }

  // Assign constant
  isl_constraint_get_constant(c, &v);
  isl_int_set(vec->p[nb_in + nb_params + 1], v);

  openscop_matrix_insert_vector(m, vec, m->NbRows);

  return 0;
}


/// Add an isl basic map to a OpenSCoP matrix_list
///
/// @param bmap The basic map to add
/// @param user The matrix list we should add the basic map to
///
/// XXX: At the moment this function expects just a matrix, as support
/// for matrix lists is currently not available in OpenSCoP. So union of
/// polyhedron are not yet supported
int OpenSCoP::accessToMatrix_basic_map(isl_basic_map *bmap, void *user) {
  isl_basic_map_foreach_constraint(bmap, &accessToMatrix_constraint, user);
  return 0;
}

/// Create the memory access matrix for openscop
///
/// @param S The polly statement the access matrix is created for.
/// @param isRead Are we looking for read or write accesses?
/// @param ArrayMap A map translating from the memory references to the openscop
/// indeces
///
/// @return The memory access matrix, as it is required by openscop.
openscop_matrix_p OpenSCoP::createAccessMatrix(SCoPStmt *S, bool isRead) {

  unsigned NbColumns = S->getNumIterators() + S->getNumParams() + 2;
  openscop_matrix_p m = openscop_matrix_malloc(0, NbColumns);

  for (SCoPStmt::memacc_iterator MI = S->memacc_begin(), ME = S->memacc_end();
       MI != ME; ++MI)
    if ((*MI)->isRead() == isRead) {
      // Extract the access function.
      isl_map_foreach_basic_map((*MI)->getAccessFunction(),
                                &accessToMatrix_basic_map, m);

      // Set the index of the memory access base element.
      std::map<const Value*, int>::iterator BA =
        ArrayMap.find((*MI)->getBaseAddr());
      isl_int_set_si(m->p[m->NbRows - 1][0], (*BA).second + 1);
    }

  return m;
}

OpenSCoP::~OpenSCoP() {
  // Free array names.
  for (int i = 0; i < openscop->nb_arrays; ++i)
    delete[](openscop->arrays[i]);

  delete[](openscop->arrays);
  openscop->arrays = NULL;
  openscop->nb_arrays = 0;

  // Free scattering names.
  for (int i = 0; i < openscop->nb_scattdims; ++i)
    delete[](openscop->scattdims[i]);

  delete[](openscop->scattdims);
  openscop->scattdims = NULL;
  openscop->nb_scattdims = 0;

  // Free parameters
  for (int i = 0; i < openscop->nb_parameters; ++i)
    delete[](openscop->parameters[i]);

  delete[](openscop->parameters);
  openscop->parameters = NULL;
  openscop->nb_parameters = 0;

  openscop_statement_p stmt = openscop->statement;

  // Free Statements
  while (stmt) {
    openscop_statement_p TempStmt = stmt->next;
    stmt->next = NULL;
    freeStatement(stmt);
    stmt = TempStmt;
  }

  openscop->statement = NULL;

  openscop_scop_free(openscop);
}

std::string SCoPExporter::getFileName(Region *R) const {
  std::string FunctionName = R->getEntry()->getParent()->getNameStr();
  std::string ExitName;

  if (R->getExit())
    ExitName = R->getExit()->getNameStr();
  else
    ExitName = "FunctionExit";

  std::string RegionName = R->getEntry()->getNameStr() + "---" + ExitName;
  std::string FileName = FunctionName + "___" + RegionName + ".scop";

  return FileName;
}

void SCoPExporter::print(raw_ostream &OS, const Module *) const {
  if (S) {
    Region *R = const_cast<Region*>(&S->getRegion());
    std::string FunctionName = R->getEntry()->getParent()->getNameStr();
    std::string FileName = getFileName(R);

    OS << "Writing SCoP '" << R->getNameStr() << "' in function '"
      << FunctionName << "' to '" << FileName << "'...\n";
  } else {
    OS << "Invalid SCoP!\n";
  }
}

bool SCoPExporter::runOnRegion(Region *R, RGPassManager &RGM) {
  S = getAnalysis<SCoPInfo>().getSCoP();

  if (!S)
    return false;

  std::string FileName = ExportDir + "/" + getFileName(R);
  FILE *F = fopen(FileName.c_str(), "w");

  if (!F) {
    errs() << "Cannot open file: " << FileName << "\n";
    errs() << "Skipping export.\n";
    return false;
  }

  OpenSCoP openscop(S);
  openscop.print(F);
  fclose(F);

  return false;
}

void SCoPExporter::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
  AU.addRequired<SCoPInfo>();
}

static RegisterPass<SCoPExporter> A("polly-export",
                                    "Polly - Export SCoPs with OpenSCoP library"
                                    " (Writes a .scop file for each SCoP)"
                                    );

Pass *polly::createSCoPExporterPass() {
  return new SCoPExporter();
}
