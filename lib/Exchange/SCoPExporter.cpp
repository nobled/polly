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

struct SCoPExporter : public RegionPass {
  static char ID;
  SCoP *S;
  explicit SCoPExporter() : RegionPass(&ID) {}

  virtual bool runOnRegion(Region *R, RGPassManager &RGM);
  void getAnalysisUsage(AnalysisUsage &AU) const;
};

}

char SCoPExporter::ID = 0;

/// Add an isl constraint to an OpenSCoP matrix.
///
/// @param user The matrix
/// @param c The constraint
int domainToMatrix_constraint(isl_constraint *c, void *user) {
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
int domainToMatrix_basic_set(isl_basic_set *bset, void *user) {
  openscop_matrix_p m = (openscop_matrix_p) user;
  assert(!m->NbRows && "Union of polyhedron not yet supported");

  isl_basic_set_foreach_constraint(bset, &domainToMatrix_constraint, user);
  return 0;
}

/// Translate a isl_set to a OpenSCoP matrix.
///
/// @param PS The set to be translated
/// @return A OpenSCoP Matrix
openscop_matrix_p domainToMatrix(polly_set *PS) {

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
int scatteringToMatrix_constraint(isl_constraint *c, void *user) {
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
int scatteringToMatrix_basic_map(isl_basic_map *bmap, void *user) {
  openscop_matrix_p m = (openscop_matrix_p) user;
  assert(!m->NbRows && "Union of polyhedron not yet supported");

  isl_basic_map_foreach_constraint(bmap, &scatteringToMatrix_constraint, user);
  return 0;
}

/// Translate a isl_map to a OpenSCoP matrix.
///
/// @param map The map to be translated
/// @return A OpenSCoP Matrix
openscop_matrix_p scatteringToMatrix(polly_map *pmap) {

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
int accessToMatrix_constraint(isl_constraint *c, void *user) {
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
int accessToMatrix_basic_map(isl_basic_map *bmap, void *user) {
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
openscop_matrix_p createAccessMatrix(SCoPStmt *S, bool isRead,
                                     std::map<const Value*, int> *ArrayMap) {

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
        ArrayMap->find((*MI)->getBaseAddr());
      isl_int_set_si(m->p[m->NbRows - 1][0], (*BA).second + 1);
    }

  return m;
}

/// Create an openscop statement from a polly statement.
openscop_statement_p SCoPStmtToOpenSCoPStmt(SCoPStmt *S,
  std::map<const Value*, int> *ArrayMap) {
    openscop_statement_p Stmt = openscop_statement_malloc();

    // Domain & Schedule
    Stmt->domain = domainToMatrix(S->getDomain());
    Stmt->schedule = scatteringToMatrix(S->getScattering());

    // Statement name
    std::string str = S->getBasicBlock()->getNameStr();
    Stmt->body = new char[str.size() + 1];
    strcpy(Stmt->body, str.c_str());

    // Iterator names
    Stmt->nb_iterators = isl_set_n_dim(S->getDomain());
    Stmt->iterators = new char*[Stmt->nb_iterators];
    for (int i = 0; i < Stmt->nb_iterators; ++i) {
      Stmt->iterators[i] = new char[20];
      sprintf(Stmt->iterators[i], "i_%d", i);
    }

    // Memory Accesses
    Stmt->read = createAccessMatrix(S, true, ArrayMap);
    Stmt->write = createAccessMatrix(S, false, ArrayMap);

    return Stmt;
}

/// Create an openscop scop from a polly SCoP.
openscop_scop_p	ScopToOpenScop(SCoP *S) {
  openscop_scop_p OpenSCoP = openscop_scop_malloc();
  OpenSCoP->nb_parameters = S->getNumParams();

  std::map<const Value*, int> ArrayMap;


  // Initialize the referenced arrays.
  int nb_arrays = 0;

  for (SCoP::iterator SI = S->begin(), SE = S->end(); SI != SE; ++SI)
    for (SCoPStmt::memacc_iterator MI = (*SI)->memacc_begin(),
         ME = (*SI)->memacc_end(); MI != ME; ++MI) {
      const Value *BaseAddr = (*MI)->getBaseAddr();
      if (ArrayMap.find(BaseAddr) == ArrayMap.end()) {
        ArrayMap.insert(std::make_pair(BaseAddr, nb_arrays));
        ++nb_arrays;
      }
    }

  OpenSCoP->nb_arrays = nb_arrays;
  OpenSCoP->arrays = new char*[nb_arrays];

  for (int i = 0; i < nb_arrays; ++i)
    for (std::map<const Value*, int>::iterator VI = ArrayMap.begin(),
         VE = ArrayMap.end(); VI != VE; ++VI)
      if ((*VI).second == i) {
        const Value *V = (*VI).first;
        std::string name = V->getNameStr();
        OpenSCoP->arrays[i] = new char[name.size() + 1];
        strcpy(OpenSCoP->arrays[i], name.c_str());
      }

  // Set the scattering dimensions
  OpenSCoP->nb_scattdims = S->getScatterDim();
  OpenSCoP->scattdims = new char*[OpenSCoP->nb_scattdims];

  for (int i = 0; i < OpenSCoP->nb_scattdims; ++i) {
    OpenSCoP->scattdims[i] = new char[20];
    sprintf(OpenSCoP->scattdims[i], "s_%d", i);
  }

  // Initialize the statements.
  for (SCoP::reverse_iterator SI = S->rbegin(), SE = S->rend(); SI != SE;
       ++SI) {
    openscop_statement_p stmt = SCoPStmtToOpenSCoPStmt(*SI, &ArrayMap);
    stmt->next = OpenSCoP->statement;
    OpenSCoP->statement = stmt;
  }


  return OpenSCoP;
}

bool SCoPExporter::runOnRegion(Region *R, RGPassManager &RGM) {
  S = getAnalysis<SCoPInfo>().getSCoP();

  if (!S)
    return false;

  std::string FunctionName = R->getEntry()->getParent()->getNameStr();
  std::string Filename = FunctionName + "_-_" + R->getNameStr() + ".scop";

  errs() << "Writing SCoP '" << R->getNameStr() << "' in function '"
    << FunctionName << "' to '" << Filename << "'...\n";

  FILE *F = fopen(Filename.c_str(), "w");
  openscop_scop_p OpenSCoP = ScopToOpenScop(S);

  openscop_scop_print_dot_scop(F, OpenSCoP);

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
