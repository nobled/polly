//===- Pocc.cpp - Pocc interface ----------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Pocc[1] interface.
//
// Pocc, the polyhedral compilation collection is a collection of polyhedral
// tools. It is used as an optimizer in polly
//
// [1] http://www-roc.inria.fr/~pouchet/software/pocc/
//
//===----------------------------------------------------------------------===//

#include "polly/Cloog.h"
#include "polly/LinkAllPasses.h"

#ifdef SCOPLIB_FOUND
#include "polly/ScopInfo.h"
#include "polly/Dependences.h"

#include "llvm/Support/Path.h"
#include "llvm/Support/Program.h"
#include "llvm/Support/MemoryBuffer.h"

#include "polly/ScopLib.h"

using namespace llvm;
using namespace polly;

namespace {

  class Pocc : public ScopPass {
    sys::Path plutoStderr;
    sys::Path plutoStdout;

  public:
    static char ID;
    explicit Pocc() : ScopPass(ID) {}

    std::string getFileName(Region *R) const;
    virtual bool runOnScop(Scop &S);
    void printScop(llvm::raw_ostream &OS) const;
    void getAnalysisUsage(AnalysisUsage &AU) const;
  };

}

char Pocc::ID = 0;
bool Pocc::runOnScop(Scop &S) {
  Dependences *D = &getAnalysis<Dependences>();

  // Create the scop file.
  sys::Path tempDir = sys::Path::GetTemporaryDirectory();
  sys::Path scopFile = tempDir;
  scopFile.appendComponent("polly.scop");
  scopFile.createFileOnDisk();

  FILE *F = fopen(scopFile.c_str(), "w");

  if (!F) {
    errs() << "Cannot open file: " << tempDir.c_str() << "\n";
    errs() << "Skipping export.\n";
    return false;
  }

  ScopLib scoplib(&S);
  scoplib.print(F);
  fclose(F);

  // Execute pocc
  sys::Program program;

  sys::Path pocc = sys::Program::FindProgramByName("pocc");

  std::vector<const char*> arguments;
  arguments.push_back("pocc");
  arguments.push_back("--read-scop");
  arguments.push_back(scopFile.c_str());
  arguments.push_back("--pluto-tile-scat");
  arguments.push_back("--cloogify-scheds");
  arguments.push_back("--output-scop");
  arguments.push_back(0);

  plutoStdout = tempDir;
  plutoStdout.appendComponent("pluto.stdout");
  plutoStderr = tempDir;
  plutoStderr.appendComponent("pluto.stderr");

  std::vector<sys::Path*> redirect;
  redirect.push_back(0);
  redirect.push_back(&plutoStdout);
  redirect.push_back(&plutoStderr);

  program.ExecuteAndWait(pocc, &arguments[0], 0,
                         (sys::Path const **) &redirect[0]);

  // Read the created scop file
  sys::Path newScopFile = tempDir;
  newScopFile.appendComponent("polly.pocc.c.scop");

  FILE *poccFile = fopen(scopFile.c_str(), "r");
  ScopLib newScoplib(&S, poccFile, D);
  scoplib.updateScattering();
  fclose(poccFile);

  return false;
}

void Pocc::printScop(raw_ostream &OS) const {
  MemoryBuffer *stdoutBuffer = MemoryBuffer::getFile(plutoStdout.c_str());
  MemoryBuffer *stderrBuffer = MemoryBuffer::getFile(plutoStderr.c_str());

  errs() << "Pocc output\n";

  if (stdoutBuffer) {
    OS << "pocc stdout: " << stdoutBuffer->getBufferIdentifier() << "\n";
    OS << stdoutBuffer->getBuffer() << "\n";
  }

  if (stderrBuffer) {
    OS << "pocc stderr: " << plutoStderr.c_str() << "\n";
    OS << stderrBuffer->getBuffer() << "\n";
  }

}

void Pocc::getAnalysisUsage(AnalysisUsage &AU) const {
  ScopPass::getAnalysisUsage(AU);
  AU.addRequired<ScopInfo>();
  AU.addRequired<Dependences>();
}

static RegisterPass<Pocc> A("polly-pocc",
                            "Polly - Optimize the scop using pocc");

Pass* polly::createPoccPass() {
  return new Pocc();
}
#endif /* SCOPLIB_FOUND */
