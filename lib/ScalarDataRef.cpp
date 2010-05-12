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
#include "llvm/ADT/DepthFirstIterator.h"
#include "llvm/ADT/GraphTraits.h"
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

bool ScalarDataRef::isCyclicUse(const Instruction* def,
                                const Instruction* use) const {
  assert(def && use && "Expect def and use not be null");

  const BasicBlock *defBB = def->getParent(),
             *useBB = use->getParent();

  return (DT->dominates(useBB, defBB)) && isa<PHINode>(use);
}

int &ScalarDataRef::getDataRefFor(const Instruction &Inst){
  if (!DataRefs.count(&Inst))
    DataRefs.insert(std::make_pair(&Inst, Inst.getNumUses()));

  int &ret = DataRefs[&Inst];
  assert(ret >=0 && "DataRefs broken!");

  return ret;
}

bool ScalarDataRef::killedAsTempVal(const Instruction &Inst) const {
  SDRDataRefMapTy::const_iterator at = DataRefs.find(&Inst);
  return at != DataRefs.end() && at->second == 0;
}

void ScalarDataRef::getAllUsing(Instruction &Inst,
                                SmallVectorImpl<const Value*> &Defs) {
  // XXX: temporary value not using anything?
  if (killedAsTempVal(Inst))
    return;

  DEBUG(dbgs() << "get the reading values of :" << Inst << "\n");
  Value *Ptr = 0;
  if (LoadInst *Ld = dyn_cast<LoadInst>(&Inst))
    Ptr = Ld->getPointerOperand();
  else if (StoreInst *St = dyn_cast<StoreInst>(&Inst))
    Ptr = St->getPointerOperand();

  BasicBlock *useBB = Inst.getParent();

  for (Instruction::op_iterator I = Inst.op_begin(), E = Inst.op_end();
      I != E; ++I) {
    Value *V = *I;

    // The pointer of load/Store instruction will not consider as "scalar".
    if (Ptr == V)
      continue;
    // An argument will never be changed in SSA form, so we could just treat
    // them as a constant.
    else if (isa<Argument>(V) || isa<GlobalValue>(V))
      continue;
    else if (Instruction *opInst = dyn_cast<Instruction>(*I)) {
      // Ignore the temporary values.
      if (killedAsTempVal(*opInst))
        continue;

      // If operand is from others BBs, we need to read it explicitly
      if (useBB != opInst->getParent())
        Defs.push_back(opInst);
      // Or the loop carried variable
      else if (isCyclicUse(opInst, &Inst))
        Defs.push_back(opInst);
    }
  }

  for (SmallVector<const Value*, 4>::iterator VI = Defs.begin(),
      VE = Defs.end(); VI != VE; ++VI)
    DEBUG(dbgs() << "get read: " << **VI << "\n");
  DEBUG(dbgs() << "\n");
}

bool ScalarDataRef::isDefExported(Instruction &Inst) const {
  // A instruction have void type never exported
  if (Inst.getType()->isVoidTy())
    return false;

  SDRDataRefMapTy::const_iterator at = DataRefs.find(&Inst);

  // if all use of DataDefs is reduced, it is not export.
  if (killedAsTempVal(Inst))
    return false;

  BasicBlock *defBB = Inst.getParent();

  for (Value::use_iterator I = Inst.use_begin(), E = Inst.use_end();
    I != E; ++I) {
      Instruction *U = cast<Instruction>(*I);
      // Just ignore it if the Instruction using Inst had been kill as temporary value
      if(killedAsTempVal(*U))
        continue;

      if (defBB != U->getParent())
        return true;

      if (isCyclicUse(&Inst, U))
        return true;
  }

  return false;
}

void ScalarDataRef::reduceTempRefFor(const Instruction &Inst) {
  assert(Inst.hasNUsesOrMore(1) && "The instruction have no use cant be reduced!");

  // And the value of load instruction is not "temporary value"
  if (isa<LoadInst>(Inst))
    return;

  DEBUG(dbgs() << "Try to reduce: " << Inst << " use left: ");
  DEBUG(dbgs() << getDataRefFor(Inst) << "\n");
  // Only reduce the operand of Inst if it is reduced.
  if (--getDataRefFor(Inst) > 0)
    return;


  // XXX: Stop when we reach a parameter.

  for (Instruction::const_op_iterator I = Inst.op_begin(), E = Inst.op_end();
       I != E; ++I) {
    if (const Instruction *opInst = dyn_cast<Instruction>(*I)) {
      // Do reduce cyclic use, it will cause a dead loop
      if (!isCyclicUse(&Inst, opInst)) {
        DEBUG(dbgs().indent(4) << "Going to reduce: " << *opInst << " use left: ");
        DEBUG(dbgs().indent(4) << getDataRefFor(*opInst) << "\n");
        reduceTempRefFor(*opInst);
      }
    }
  }
}

bool ScalarDataRef::runOnFunction(Function &F) {
  LI = &getAnalysis<LoopInfo>();
  DT = &getAnalysis<DominatorTree>();

  return false;
}

void ScalarDataRef::releaseMemory() {
  clear();
}

void ScalarDataRef::print(raw_ostream &OS, const Module *) const {
  for (SDRDataRefMapTy::const_iterator I = DataRefs.begin(), E = DataRefs.end();
      I != E; ++I)
    OS << *(I->first) << " use left " << I->second << "\n";
}

char ScalarDataRef::ID = 0;

RegisterPass<ScalarDataRef> X("polly-scalar-data-ref",
                              "Polly - Scalar Data Reference Analysis",
                              false, true);

Pass *polly::createScalarDataRefPass() {
  return new ScalarDataRef();
}
