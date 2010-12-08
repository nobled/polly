//===- ScopLib.cpp - ScopLib interface ------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// ScopLib Interface
//
//===----------------------------------------------------------------------===//

#include "polly/LinkAllPasses.h"

#ifdef SCOPLIB_FOUND
#include "polly/ScopLib.h"
#include "polly/ScopInfo.h"

#include "llvm/Support/CommandLine.h"

#include "stdio.h"
#include "isl/set.h"
#include "isl/constraint.h"

using namespace llvm;

namespace polly {

ScopLib::ScopLib(Scop *S) : PollyScop(S) {
  scoplib = scoplib_scop_malloc();

  initializeArrays();
  initializeParameters();
  initializeScattering();
  initializeStatements();
}

void ScopLib::initializeParameters() {
  scoplib->nb_parameters = PollyScop->getNumParams();
  scoplib->parameters = new char*[scoplib->nb_parameters];

  for (int i = 0; i < scoplib->nb_parameters; ++i) {
    scoplib->parameters[i] = new char[20];
    sprintf(scoplib->parameters[i], "p_%d", i);
  }
}

void ScopLib::initializeArrays() {
  int nb_arrays = 0;

  for (Scop::iterator SI = PollyScop->begin(), SE = PollyScop->end(); SI != SE;
       ++SI)
    for (ScopStmt::memacc_iterator MI = (*SI)->memacc_begin(),
         ME = (*SI)->memacc_end(); MI != ME; ++MI) {
      const Value *BaseAddr = (*MI)->getBaseAddr();
      if (ArrayMap.find(BaseAddr) == ArrayMap.end()) {
        ArrayMap.insert(std::make_pair(BaseAddr, nb_arrays));
        ++nb_arrays;
      }
    }

  scoplib->nb_arrays = nb_arrays;
  scoplib->arrays = new char*[nb_arrays];

  for (int i = 0; i < nb_arrays; ++i)
    for (std::map<const Value*, int>::iterator VI = ArrayMap.begin(),
         VE = ArrayMap.end(); VI != VE; ++VI)
      if ((*VI).second == i) {
        const Value *V = (*VI).first;
        std::string name = V->getNameStr();
        scoplib->arrays[i] = new char[name.size() + 1];
        strcpy(scoplib->arrays[i], name.c_str());
      }
}

void ScopLib::initializeScattering() {
}

scoplib_statement_p ScopLib::initializeStatement(ScopStmt *stmt) {
  scoplib_statement_p Stmt = scoplib_statement_malloc();

  // Domain & Schedule
  Stmt->domain = scoplib_matrix_list_malloc();
  Stmt->domain->elt = domainToMatrix(stmt->getDomain());
  Stmt->domain->next = NULL;
  Stmt->schedule = scatteringToMatrix(stmt->getScattering());

  // Statement name
  std::string entryName;
  raw_string_ostream OS(entryName);
  WriteAsOperand(OS, stmt->getBasicBlock(), false);
  entryName = OS.str();
  Stmt->body = (char*)malloc(sizeof(char) * (entryName.size() + 1));
  strcpy(Stmt->body, entryName.c_str());

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

void ScopLib::initializeStatements() {
  for (Scop::reverse_iterator SI = PollyScop->rbegin(), SE = PollyScop->rend();
       SI != SE; ++SI) {

    if ((*SI)->isFinalRead())
      continue;

    scoplib_statement_p stmt = initializeStatement(*SI);
    stmt->next = scoplib->statement;
    scoplib->statement = stmt;
  }
}

void ScopLib::freeStatement(scoplib_statement_p stmt) {

  if (stmt->read)
    scoplib_matrix_free(stmt->read);
  stmt->read = NULL;

  if (stmt->write)
    scoplib_matrix_free(stmt->write);
  stmt->write = NULL;

  while (stmt->domain) {
    scoplib_matrix_free(stmt->domain->elt);
    stmt->domain = stmt->domain->next;
  }

  if (stmt->schedule)
    scoplib_matrix_free(stmt->schedule);
  stmt->schedule = NULL;

  for (int i = 0; i < stmt->nb_iterators; ++i)
    delete[](stmt->iterators[i]);

  delete[](stmt->iterators);
  stmt->iterators = NULL;
  stmt->nb_iterators = 0;

  delete[](stmt->body);
  stmt->body = NULL;

  scoplib_statement_free(stmt);
}

void ScopLib::print(FILE *F) {
  scoplib_scop_print_dot_scop(F, scoplib);
}

/// Add an isl constraint to an ScopLib matrix.
///
/// @param user The matrix
/// @param c The constraint
int ScopLib::domainToMatrix_constraint(isl_constraint *c, void *user) {
  scoplib_matrix_p m = (scoplib_matrix_p) user;

  int nb_params = isl_constraint_dim(c, isl_dim_param);
  int nb_vars = isl_constraint_dim(c, isl_dim_set);
  int nb_div = isl_constraint_dim(c, isl_dim_div);

  assert(!nb_div && "Existentially quantified variables not yet supported");

  scoplib_vector_p vec = scoplib_vector_malloc(nb_params + nb_vars + 2);

  // Assign type
  if (isl_constraint_is_equality(c))
    scoplib_vector_tag_equality(vec);
  else
    scoplib_vector_tag_inequality(vec);

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

  scoplib_matrix_insert_vector(m, vec, m->NbRows);

  return 0;
}

/// Add an isl basic set to a ScopLib matrix_list
///
/// @param bset The basic set to add
/// @param user The matrix list we should add the basic set to
///
/// XXX: At the moment this function expects just a matrix, as support
/// for matrix lists is currently not available in ScopLib. So union of
/// polyhedron are not yet supported
int ScopLib::domainToMatrix_basic_set(isl_basic_set *bset, void *user) {
  scoplib_matrix_p m = (scoplib_matrix_p) user;
  assert(!m->NbRows && "Union of polyhedron not yet supported");

  isl_basic_set_foreach_constraint(bset, &domainToMatrix_constraint, user);
  return 0;
}

/// Translate a isl_set to a ScopLib matrix.
///
/// @param PS The set to be translated
/// @return A ScopLib Matrix
scoplib_matrix_p ScopLib::domainToMatrix(isl_set *PS) {

  // Create a canonical copy of this set.
  isl_set *set = isl_set_copy(PS);
  set = isl_set_compute_divs (set);
  set = isl_set_align_divs (set);

  // Initialize the matrix.
  unsigned NbRows, NbColumns;
  NbRows = 0;
  NbColumns = isl_set_n_dim(PS) + isl_set_n_param(PS) + 2;
  scoplib_matrix_p matrix = scoplib_matrix_malloc(NbRows, NbColumns);

  // Copy the content into the matrix.
  isl_set_foreach_basic_set(set, &domainToMatrix_basic_set, matrix);

  isl_set_free(set);

  return matrix;
}

/// Add an isl constraint to an ScopLib matrix.
///
/// @param user The matrix
/// @param c The constraint
int ScopLib::scatteringToMatrix_constraint(isl_constraint *c, void *user) {
  scoplib_matrix_p m = (scoplib_matrix_p) user;

  int nb_params = isl_constraint_dim(c, isl_dim_param);
  int nb_in = isl_constraint_dim(c, isl_dim_in);
  int nb_div = isl_constraint_dim(c, isl_dim_div);

  assert(!nb_div && "Existentially quantified variables not yet supported");

  scoplib_vector_p vec =
    scoplib_vector_malloc(nb_params + nb_in + 2);

  // Assign type
  if (isl_constraint_is_equality(c))
    scoplib_vector_tag_equality(vec);
  else
    scoplib_vector_tag_inequality(vec);

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

  scoplib_vector_p null =
    scoplib_vector_malloc(nb_params + nb_in + 2);

  vec = scoplib_vector_sub(null, vec);
  scoplib_matrix_insert_vector(m, vec, 0);

  return 0;
}

/// Add an isl basic map to a ScopLib matrix_list
///
/// @param bmap The basic map to add
/// @param user The matrix list we should add the basic map to
///
/// XXX: At the moment this function expects just a matrix, as support
/// for matrix lists is currently not available in ScopLib. So union of
/// polyhedron are not yet supported
int ScopLib::scatteringToMatrix_basic_map(isl_basic_map *bmap, void *user) {
  scoplib_matrix_p m = (scoplib_matrix_p) user;
  assert(!m->NbRows && "Union of polyhedron not yet supported");

  isl_basic_map_foreach_constraint(bmap, &scatteringToMatrix_constraint, user);
  return 0;
}

/// Translate a isl_map to a ScopLib matrix.
///
/// @param map The map to be translated
/// @return A ScopLib Matrix
scoplib_matrix_p ScopLib::scatteringToMatrix(isl_map *pmap) {

  // Create a canonical copy of this set.
  isl_map *map = isl_map_copy(pmap);
  map = isl_map_compute_divs (map);
  map = isl_map_align_divs (map);

  // Initialize the matrix.
  unsigned NbRows, NbColumns;
  NbRows = 0;
  NbColumns = isl_map_n_in(pmap) + isl_map_n_param(pmap) + 2;
  scoplib_matrix_p matrix = scoplib_matrix_malloc(NbRows, NbColumns);

  // Copy the content into the matrix.
  isl_map_foreach_basic_map(map, &scatteringToMatrix_basic_map, matrix);

  // Only keep the relevant rows.
  scoplib_matrix_p reduced = scoplib_matrix_ncopy(matrix,
                                                  isl_map_n_in(pmap) * 2 + 1);

  scoplib_matrix_free (matrix);
  isl_map_free(map);

  return reduced;
}

/// Add an isl constraint to an ScopLib matrix.
///
/// @param user The matrix
/// @param c The constraint
int ScopLib::accessToMatrix_constraint(isl_constraint *c, void *user) {
  scoplib_matrix_p m = (scoplib_matrix_p) user;

  int nb_params = isl_constraint_dim(c, isl_dim_param);
  int nb_in = isl_constraint_dim(c, isl_dim_in);
  int nb_div = isl_constraint_dim(c, isl_dim_div);

  assert(!nb_div && "Existentially quantified variables not yet supported");

  scoplib_vector_p vec =
    scoplib_vector_malloc(nb_params + nb_in + 2);

  isl_int v;
  isl_int_init(v);

  // The access dimension has to be one.
  isl_constraint_get_coefficient(c, isl_dim_out, 0, &v);
  assert(isl_int_is_one(v));
  bool inverse = true ;

  // Assign variables
  for (int i = 0; i < nb_in; ++i) {
    isl_constraint_get_coefficient(c, isl_dim_in, i, &v);

    if (inverse) isl_int_neg(v,v);

    isl_int_set(vec->p[i + 1], v);
  }

  // Assign parameters
  for (int i = 0; i < nb_params; ++i) {
    isl_constraint_get_coefficient(c, isl_dim_param, i, &v);

    if (inverse) isl_int_neg(v,v);

    isl_int_set(vec->p[nb_in + i + 1], v);
  }

  // Assign constant
  isl_constraint_get_constant(c, &v);

  if (inverse) isl_int_neg(v,v);

  isl_int_set(vec->p[nb_in + nb_params + 1], v);

  scoplib_matrix_insert_vector(m, vec, m->NbRows);

  return 0;
}


/// Add an isl basic map to a ScopLib matrix_list
///
/// @param bmap The basic map to add
/// @param user The matrix list we should add the basic map to
///
/// XXX: At the moment this function expects just a matrix, as support
/// for matrix lists is currently not available in ScopLib. So union of
/// polyhedron are not yet supported
int ScopLib::accessToMatrix_basic_map(isl_basic_map *bmap, void *user) {
  isl_basic_map_foreach_constraint(bmap, &accessToMatrix_constraint, user);
  return 0;
}

/// Create the memory access matrix for scoplib
///
/// @param S The polly statement the access matrix is created for.
/// @param isRead Are we looking for read or write accesses?
/// @param ArrayMap A map translating from the memory references to the scoplib
/// indeces
///
/// @return The memory access matrix, as it is required by scoplib.
scoplib_matrix_p ScopLib::createAccessMatrix(ScopStmt *S, bool isRead) {

  unsigned NbColumns = S->getNumIterators() + S->getNumParams() + 2;
  scoplib_matrix_p m = scoplib_matrix_malloc(0, NbColumns);

  for (ScopStmt::memacc_iterator MI = S->memacc_begin(), ME = S->memacc_end();
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

ScopLib::~ScopLib() {
  // Free array names.
  for (int i = 0; i < scoplib->nb_arrays; ++i)
    delete[](scoplib->arrays[i]);

  delete[](scoplib->arrays);
  scoplib->arrays = NULL;
  scoplib->nb_arrays = 0;

  // Free parameters
  for (int i = 0; i < scoplib->nb_parameters; ++i)
    delete[](scoplib->parameters[i]);

  delete[](scoplib->parameters);
  scoplib->parameters = NULL;
  scoplib->nb_parameters = 0;

  scoplib_statement_p stmt = scoplib->statement;

  // Free Statements
  while (stmt) {
    scoplib_statement_p TempStmt = stmt->next;
    stmt->next = NULL;
    freeStatement(stmt);
    stmt = TempStmt;
  }

  scoplib->statement = NULL;

  scoplib_scop_free(scoplib);
}
}

#endif
