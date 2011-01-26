//===-- JSONExporter.cpp  - Export Scops as JSON  -------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Export the Scops build by ScopInfo pass as a JSON file.
//
//===----------------------------------------------------------------------===//

#include "polly/LinkAllPasses.h"
#include "polly/Dependences.h"
#include "polly/ScopInfo.h"
#include "polly/ScopPass.h"
#include "polly/json.h"

#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ToolOutputFile.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/system_error.h"
#include "llvm/ADT/OwningPtr.h"
#include "llvm/Assembly/Writer.h"

#include "stdio.h"
#include "isl/set.h"
#include "isl/map.h"
#include "isl/constraint.h"
#include "isl/printer.h"

#include <string>

using namespace llvm;
using namespace polly;

namespace {
static cl::opt<std::string>
ImportDir("polly-import-jscop-dir",
          cl::desc("The directory to import the .jscop files from."), cl::Hidden,
          cl::value_desc("Directory path"), cl::ValueRequired, cl::init("."));
static cl::opt<std::string>
ImportPostfix("polly-import-jscop-postfix",
              cl::desc("Postfix to append to the import .jsop files."),
              cl::Hidden, cl::value_desc("File postfix"), cl::ValueRequired,
              cl::init(""));

struct JSONExporter : public ScopPass {
  static char ID;
  Scop *S;
  explicit JSONExporter() : ScopPass(ID) {}

  std::string getFileName(Scop *S) const;
  virtual bool runOnScop(Scop &S);
  void printScop(raw_ostream &OS) const;
  void getAnalysisUsage(AnalysisUsage &AU) const;
};
struct JSONImporter : public ScopPass {
  static char ID;
  Scop *S;
  explicit JSONImporter() : ScopPass(ID) {}

  std::string getFileName(Scop *S) const;
  virtual bool runOnScop(Scop &S);
  void printScop(raw_ostream &OS) const;
  void getAnalysisUsage(AnalysisUsage &AU) const;
};

}

char JSONExporter::ID = 0;
std::string JSONExporter::getFileName(Scop *S) const {
  std::string FunctionName =
    S->getRegion().getEntry()->getParent()->getNameStr();
  std::string FileName = FunctionName + "___" + S->getNameStr() + ".jscop";
  return FileName;
}

void JSONExporter::printScop(raw_ostream &OS) const {
  S->print(OS);
}

bool JSONExporter::runOnScop(Scop &scop) {
  S = &scop;
  Region &R = S->getRegion();

  std::string FileName = ImportDir + "/" + getFileName(S);
  char *text;
  json_t *root, *label, *value, *jsonScop, *jsonStmt, *jsonStmts,
         *jsonAccesses;
  root = json_new_object();

  // Create new Scop
  jsonScop = json_new_object();

    // Set name
    label = json_new_string("name");
    value = json_new_string(R.getNameStr().c_str());
    json_insert_child(label, value);
    json_insert_child(jsonScop, label);

    // Set Context
    label = json_new_string("context");
    std::string context = S->getContextStr();
    value = json_new_string(context.c_str());
    json_insert_child(label, value);
    json_insert_child(jsonScop, label);

    // Create Statements
    jsonStmts = json_new_array();
    label = json_new_string("statements");
    json_insert_child(label, jsonStmts);
    json_insert_child(jsonScop, label);

    for (Scop::iterator SI = S->begin(), SE = S->end(); SI != SE; ++SI) {
      ScopStmt *Stmt = *SI;

      if (Stmt->isFinalRead())
        continue;

      jsonStmt = json_new_object();

      // Add Name
      label = json_new_string("name");
      value = json_new_string(Stmt->getBaseName());
      json_insert_child(label, value);
      json_insert_child(jsonStmt, label);

      // Add domain
      label = json_new_string("domain");
      std::string domain = Stmt->getDomainStr();
      value = json_new_string(domain.c_str());
      json_insert_child(label, value);
      json_insert_child(jsonStmt, label);

      // Add schedule
      label = json_new_string("schedule");
      std::string schedule = Stmt->getScatteringStr();
      value = json_new_string(schedule.c_str());
      json_insert_child(label, value);
      json_insert_child(jsonStmt, label);

      // Add accesses
      jsonAccesses = json_new_array();
      label = json_new_string("accesses");
      json_insert_child(label, jsonAccesses);
      json_insert_child(jsonStmt, label);

      for (ScopStmt::memacc_iterator MI = Stmt->memacc_begin(),
           ME = Stmt->memacc_end(); MI != ME; ++MI) {
        json_t *jsonAccess;
        jsonAccess = json_new_object();

        // Add kind
        label = json_new_string("kind");
        if ((*MI)->isRead())
          value = json_new_string("read");
        else
          value = json_new_string("write");
        json_insert_child(label, value);
        json_insert_child(jsonAccess, label);

        // Add relation
        label = json_new_string("relation");
        std::string relation = (*MI)->getAccessFunctionStr();
        value = json_new_string(relation.c_str());
        json_insert_child(label, value);
        json_insert_child(jsonAccess, label);

        json_insert_child(jsonAccesses, jsonAccess);
      }

      json_insert_child(jsonStmts, jsonStmt);
    }

  json_tree_to_string(jsonScop, &text);
  json_free_value(&root);
  text = json_format_string(text);

  // Write to file.
  std::string ErrInfo;
  tool_output_file F(FileName.c_str(), ErrInfo);

  std::string FunctionName = R.getEntry()->getParent()->getNameStr();
  errs() << "Writing JScop '" << R.getNameStr() << "' in function '"
    << FunctionName << "' to '" << FileName << "'.\n";

  if (ErrInfo.empty()) {
    F.os() << text;
    F.os().close();
    if (!F.os().has_error()) {
      errs() << "\n";
      F.keep();
      return false;
    }
  }

  errs() << "  error opening file for writing!\n";
  F.os().clear_error();

  return false;
}

void JSONExporter::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
  AU.addRequired<ScopInfo>();
}

static RegisterPass<JSONExporter> A("polly-export-jscop",
                                    "Polly - Export Scops as JSON"
                                    " (Writes a .jscop file for each Scop)"
                                    );

Pass *polly::createJSONExporterPass() {
  return new JSONExporter();
}

char JSONImporter::ID = 0;
std::string JSONImporter::getFileName(Scop *S) const {
  std::string FunctionName =
    S->getRegion().getEntry()->getParent()->getNameStr();
  std::string FileName = FunctionName + "___" + S->getNameStr() + ".jscop";

  if (ImportPostfix != "")
    FileName += "." + ImportPostfix;

  return FileName;
}
void JSONImporter::printScop(raw_ostream &OS) const {
  S->print(OS);
}

typedef Dependences::StatementToIslMapTy StatementToIslMapTy;

bool JSONImporter::runOnScop(Scop &scop) {
  S = &scop;
  Region &R = S->getRegion();
  Dependences *D = &getAnalysis<Dependences>();


  std::string FileName = ImportDir + "/" + getFileName(S);

  std::string FunctionName = R.getEntry()->getParent()->getNameStr();
  errs() << "Reading JScop '" << R.getNameStr() << "' in function '"
    << FunctionName << "' from '" << FileName << "'.\n";
  OwningPtr<MemoryBuffer> result;
  error_code ec = MemoryBuffer::getFile(FileName, result);

  if (ec) {
    errs() << "File could not be read: " << ec.message() << "\n";
    return false;
  }

  json_t *root = NULL;
  json_parse_document(&root, const_cast<char*>(result->getBufferStart()));
  json_t *statements = json_find_first_label (root, "statements");

  json_t *stmt = statements->child->child;
  StatementToIslMapTy &NewScattering = *(new StatementToIslMapTy());

  for (Scop::iterator SI = S->begin(), SE = S->end(); SI != SE; ++SI) {
    ScopStmt *Stmt = *SI;

    if (Stmt->isFinalRead())
      continue;

    json_t *schedule = json_find_first_label (stmt, "schedule");
    isl_map *m = isl_map_read_from_str(S->getCtx(), schedule->child->text, -1);
    NewScattering[*SI] = m;
    stmt = stmt->next;
  }
  if (!D->isValidScattering(&NewScattering)) {
    errs() << "JScop file contains a scattering that changes the "
           << "dependences. Use -disable-polly-legality to continue anyways\n";
    return false;
  }

   for (Scop::iterator SI = S->begin(), SE = S->end(); SI != SE; ++SI) {
     ScopStmt *Stmt = *SI;

     if (NewScattering.find(Stmt) != NewScattering.end())
       Stmt->setScattering((NewScattering)[Stmt]);
  }

  return false;
}

void JSONImporter::getAnalysisUsage(AnalysisUsage &AU) const {
  ScopPass::getAnalysisUsage(AU);
  AU.addRequired<Dependences>();
}

static RegisterPass<JSONImporter> B("polly-import-jscop",
                                    "Polly - Import Scops from JSON"
                                    " (Reads a .jscop file for each Scop)"
                                    );

Pass *polly::createJSONImporterPass() {
  return new JSONImporter();
}
