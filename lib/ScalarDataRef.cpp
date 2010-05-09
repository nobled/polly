//===----- ScalarDataRef.cpp - Capture scalar data reference ----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implement the scalar data reference analysis, whice will capture
// scalar reads/writes(use/def) in SCoP statement(BB or Region).
//
//===----------------------------------------------------------------------===//

#include "polly/ScalarDataRef.h"

#include "llvm/Use.h"
#include "llvm/Assembly/Writer.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/ADT/SmallPtrSet.h"

#define DEBUG_TYPE "polly-scalar-data-ref"
#include "llvm/Support/Debug.h"

using namespace llvm;
using namespace polly;


//===----------------------------------------------------------------------===//
/// ScalarDataRef implement

void ScalarDataRef::clear() {
  DataRefs.clear();
}

ScalarDataRef::~ScalarDataRef() {
  clear();
}

void ScalarDataRef::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<LoopInfo>();
  AU.addRequired<DominatorTree>();
  AU.setPreservesAll();
}

bool ScalarDataRef::computeDataRefForScalar(Instruction &Inst) {
  // There is no read/write for void type.
  // And Pointers and Boolean values will be handle by "NoScalarDataRef" logic.
  if (Inst.getType()->isVoidTy()
    || Inst.getType()->isPointerTy())
    return false;

  if (const IntegerType *Ty = dyn_cast<IntegerType>(Inst.getType())) {
    if (Ty->getBitWidth() == 1)
      return false;
  }

  BasicBlock *defBB = Inst.getParent();

  Loop *L = LI->getLoopFor(defBB);

  // Do not need to remember the reference for Induction variable.
  if (L && ((&Inst) == L->getCanonicalInductionVariable()
         || (&Inst) == L->getCanonicalInductionVariableIncrement()))
    return false;

  SmallPtrSet<const BasicBlock *, 8> BBs;
  BBs.insert(defBB);

  bool hasLoopCarryDep = false;

  for (Value::use_iterator I = Inst.use_begin(), E = Inst.use_end(); I != E; ++I) {
    Instruction *U = cast<Instruction>(*I);
    // Ignore the use in conditions.
    if (const IntegerType *Ty = dyn_cast<IntegerType>(U->getType()))
      if (Ty->getBitWidth() == 1)
        continue;

    const BasicBlock *UseBB = U->getParent();

    // Check for loop carry dependence.
    hasLoopCarryDep |= isa<PHINode>(U) && (DT->dominates(UseBB, defBB));

    BBs.insert(UseBB);
  }

  unsigned useFound = BBs.size()-1;

  // Is Inst "killed" in defBB?
  if (useFound == 0 && !hasLoopCarryDep)
    return false;


  for (SmallPtrSet<const BasicBlock *, 8>::const_iterator I = BBs.begin(),
      E = BBs.end(); I != E; ++I) {
    const BasicBlock *BB = *I;
    bool isDefBB = (BB == defBB);
    DataRefs[BB].push_back(RefRec(&Inst,
                                  hasLoopCarryDep && isDefBB,
                                  isDefBB ? useFound : 0));
  }

  return true;
}

bool ScalarDataRef::runOnFunction(Function &F) {
  LI = &getAnalysis<LoopInfo>();
  DT = &getAnalysis<DominatorTree>();
  Func = &F;
  return false;
}

void ScalarDataRef::releaseMemory() {
  clear();
}

void ScalarDataRef::print(raw_ostream &OS, const Module *) const {
  for (inst_iterator I = inst_begin(Func), E = inst_end(Func); I != E; ++I)
    (void) computeDataRefForScalar(*I);

  for (DataRefMapTy::const_iterator I = DataRefs.begin(), E = DataRefs.end();
      I != E; ++I) {
    WriteAsOperand(OS, I->first, false);
    OS << " {\n";
   const  SmallVectorImpl<RefRec> &Vector = I->second;
    for (SmallVectorImpl<RefRec>::const_iterator VI = Vector.begin(),
        VE = Vector.end(); VI != VE; ++VI) {
       (*VI).print(OS.indent(2));
       OS << "\n";
    }
    OS << "}\n";
  }

}

void ScalarDataRef::RefRec::print(raw_ostream &OS) const {
  if (isCyclic()) {
    WriteAsOperand(OS, getValue(), false);
    OS << " = ... previous ";
    WriteAsOperand(OS, getValue(), false);
    OS << " ...";
    return;
  }

  if (!isDef())
    OS << "... = ... ";

  WriteAsOperand(OS, getValue(), false);

  if (isDef())
    OS << " =";

  OS << " ... ";
}

char ScalarDataRef::ID = 0;

RegisterPass<ScalarDataRef> X("polly-scalar-data-ref",
                              "Polly - Scalar Data Reference Analysis",
                              false, true);

Pass *polly::createScalarDataRefPass() {
  return new ScalarDataRef();
}
