//===----- ScalarDataRef.cpp - Capture scalar data reference ----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implement the scalar data reference analysis, which will capture
// scalar reads/writes(use/def) in SCoP statement(BB or Region).
//
//===----------------------------------------------------------------------===//

#include "polly/ScalarDataRef.h"
#include "polly/Support/SCoPHelper.h"

#include "llvm/Use.h"
#include "llvm/Analysis/RegionIterator.h"
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
typedef std::map<Instruction*, AllocaInst*> SlotMapType;

void ScalarDataRef::clear() {
  DataRefs.clear();
}

ScalarDataRef::~ScalarDataRef() {
  clear();
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

bool ScalarDataRef::isKilledAsTempVal(const Instruction &Inst) const {
  SDRDataRefMapTy::const_iterator at = DataRefs.find(&Inst);
  return at != DataRefs.end() && at->second == 0;
}

bool ScalarDataRef::isUseExported(Instruction &Inst) const {
  // XXX: temporary value not using anything?
  if (isKilledAsTempVal(Inst))
    return false;

  DEBUG(dbgs() << "get the reading values of :" << Inst << "\n");
  Value *Ptr = 0;
  if (LoadInst *Ld = dyn_cast<LoadInst>(&Inst))
    Ptr = Ld->getPointerOperand();
  else if (StoreInst *St = dyn_cast<StoreInst>(&Inst))
    Ptr = St->getPointerOperand();
  else if (GetElementPtrInst *GEP = dyn_cast<GetElementPtrInst>(&Inst))
    Ptr = GEP->getPointerOperand();

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
      if (isKilledAsTempVal(*opInst))
        continue;

      // If operand is from others BBs, we need to read it explicitly
      if (useBB != opInst->getParent())
        return true;
      // Or the loop carried variable
      if (isCyclicUse(opInst, &Inst))
        return true;
    }
  }

  return false;
}

bool ScalarDataRef::isDefExported(Instruction &Inst) const {
  // A instruction have void type never exported
  if (Inst.getType()->isVoidTy())
    return false;
  // Do not export alloca
  if (isa<AllocaInst>(Inst))
    return false;

  SDRDataRefMapTy::const_iterator at = DataRefs.find(&Inst);

  // if all use of DataDefs is reduced, it is not export.
  if (isKilledAsTempVal(Inst))
    return false;

  BasicBlock *defBB = Inst.getParent();

  for (Value::use_iterator I = Inst.use_begin(), E = Inst.use_end();
    I != E; ++I) {
      Instruction *U = cast<Instruction>(*I);
      // Just ignore it if the Instruction using Inst had been kill as temporary value
      if(isKilledAsTempVal(*U))
        continue;

      if (defBB != U->getParent())
        return true;

      if (isCyclicUse(&Inst, U))
        return true;
  }

  return false;
}

void ScalarDataRef::killTempRefFor(const Instruction &Inst) {
  assert(Inst.hasNUsesOrMore(1) && "The instruction have no use cant be reduced!");

  // The value of load instruction is not "temporary value"
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
        killTempRefFor(*opInst);
      }
    }
  }
}

void ScalarDataRef::killAllTempValFor(const Region &R) {
  // Do not kill tempval in not valid region
  assert(SD->isSCoP(R) && "killAllTempValFor only work on valid region");

  for (Region::const_element_iterator I = R.element_begin(),
    E = R.element_end();I != E; ++I)
    if (!I->isSubRegion())
      killAllTempValFor(*I->getNodeAs<BasicBlock>());

  if (Loop *L = castToLoop(R, *LI))
    killAllTempValFor(*L);
}

void ScalarDataRef::killAllTempValFor(Loop &L) {
  PHINode *IndVar = L.getCanonicalInductionVariable();
  Instruction *IndVarInc = L.getCanonicalInductionVariableIncrement();

  assert(IndVar && IndVarInc && "Why these are null in a valid region?");

  // Take away the Induction Variable and its increment
  killTempRefFor(*IndVar);
  killTempRefFor(*IndVarInc);
}

void ScalarDataRef::killAllTempValFor(BasicBlock &BB) {
  for (BasicBlock::iterator I = BB.begin(), E = --BB.end(); I != E; ++I){
    Instruction *Inst = &*I;
    Value *Ptr = 0;
    // Try to extract the pointer operand
    if (LoadInst *Load = dyn_cast<LoadInst>(Inst))
      Ptr = Load->getPointerOperand();
    else if (StoreInst *Store = dyn_cast<StoreInst>(Inst))
      Ptr = Store->getPointerOperand();
    // If any pointer operand appear
    if (Ptr != 0) {
      DEBUG(dbgs() << "Reduce ptr of " << Inst << "\n");
      // The address express as affine function can be rewrite by SCEV.
      // FIXME: Do not kill the not affine one in irregular scop?
      if (Instruction *GEP = dyn_cast<Instruction>(Ptr))
        killTempRefFor(*GEP);
    }
  }

  // TODO: support switch
  if(BranchInst *Br = dyn_cast<BranchInst>(BB.getTerminator()))
    if (Br->getNumSuccessors() > 1)
      if(Instruction *Cond = dyn_cast<Instruction>(Br->getCondition()))
        // The affine condition also could be rewrite.
        killTempRefFor(*Cond);
}

void ScalarDataRef::runOnRegion(Region &R) {
  // Run on child region first
  for (Region::iterator I = R.begin(), E = R.end(); I != E; ++I)
    runOnRegion(**I);

  // Kill all temporary values that can be rewrite by SCEVExpander.
  // Only do this when R is a valid part of scop
  if (SD->isSCoP(R))
    killAllTempValFor(R);
}

void ScalarDataRef::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<RegionInfo>();
  AU.addRequired<LoopInfo>();
  AU.addRequired<DominatorTree>();
  AU.addRequired<SCoPDetection>();
  AU.setPreservesAll();
}

static Instruction *getAllocaPoint(Function &F) {
  // Insert all new allocas into entry block.
  BasicBlock *BBEntry = &F.getEntryBlock();
  assert(pred_begin(BBEntry) == pred_end(BBEntry) &&
    "Entry block to function must not have predecessors!");
  // Copy from Reg2Mem
  // Find first non-alloca instruction and create insertion point. This is
  // safe if block is well-formed: it always have terminator, otherwise
  // we'll get and assertion.
  BasicBlock::iterator I = BBEntry->begin();
  while (isa<AllocaInst>(I)) ++I;

  return
    new BitCastInst(Constant::getNullValue(Type::getInt32Ty(F.getContext())),
                    Type::getInt32Ty(F.getContext()),
                    "sdf alloca point", I);
}

static AllocaInst *CreateSlotAndStore(Instruction *I, Instruction *AllocaPoint) {
  AllocaInst *Slot = new AllocaInst(I->getType(), 0,
                                    I->getName()+".scalar", AllocaPoint);

  // Store the value to stack, after the definition of the instruction
  assert(!isa<InvokeInst>(I) && "SCoP should not have invoke inst!");
  BasicBlock::iterator InsertPt = I;
  // Don't insert before any PHI nodes.
  do
    ++InsertPt;
  while (isa<PHINode>(InsertPt));
  new StoreInst(I, Slot, InsertPt);

  return Slot;
}

bool ScalarDataRef::runOnFunction(Function &F) {
  LI = &getAnalysis<LoopInfo>();
  DT = &getAnalysis<DominatorTree>();
  SD = &getAnalysis<SCoPDetection>();

  // Run on the region tree to preform scalar data reference analyze
  RegionInfo &RI = getAnalysis<RegionInfo>();
  runOnRegion(*RI.getTopLevelRegion());

  // Find all exported scalar dependencies
  Instruction *AllocaPoint = getAllocaPoint(F);

  typedef std::vector<Instruction*>  InstVecType;
  InstVecType ExportedDef;

  std::set<Instruction*> UsingSet;
  for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I) {
    Instruction &Inst = *I;
    // Remember the export scalar definition and its uses.
    if (isDefExported(Inst)) {
      DEBUG(dbgs() << Inst << " Def export!\n");
      ExportedDef.push_back(&Inst);
    }
    if (isUseExported(Inst))
      UsingSet.insert(&Inst);
  }

  // Create the slot for all exported scalars
  typedef std::vector<User*>  UserVecType;
  UserVecType UseList;

  for (InstVecType::iterator I = ExportedDef.begin(), E = ExportedDef.end();
      I != E; ++I) {
    Instruction *Inst = *I;
    AllocaInst *Slot = CreateSlotAndStore(Inst, AllocaPoint);
    UseList.insert(UseList.end(), Inst->use_begin(), Inst->use_end());
    for (UserVecType::iterator UI = UseList.begin(), UE = UseList.end();
        UI != UE; ++UI) {
      Instruction *U = cast<Instruction>(*UI);
      // Only replace the exported use.
      if (!UsingSet.count(U))
        continue;

      if (PHINode *PN = dyn_cast<PHINode>(U)) {
        // If this is a PHI node, we can't insert a load of the value before the
        // use.  Instead, insert the load in the predecessor block corresponding
        // to the incoming value.
        //
        // Note that if there are multiple edges from a basic block to this PHI
        // node that we cannot multiple loads.  The problem is that the resultant
        // PHI node will have multiple values (from each load) coming in from the
        // same block, which is illegal SSA form.  For this reason, we keep track
        // and reuse loads we insert.
        std::map<BasicBlock*, Value*> Loads;
        for (unsigned i = 0, e = PN->getNumIncomingValues(); i != e; ++i)
          if (PN->getIncomingValue(i) == Inst) {
            Value *&V = Loads[PN->getIncomingBlock(i)];
            if (V == 0) {
              // Insert the load into the predecessor block
              V = new LoadInst(Slot, Inst->getName()+".sload", false,
                               PN->getIncomingBlock(i)->getTerminator());
            }
            PN->setIncomingValue(i, V);
          }

      } else {
        // If this is a normal instruction, just insert a load.
        Value *V = new LoadInst(Slot, Inst->getName()+".sload", false, U);
        U->replaceUsesOfWith(Inst, V);
      }
    }
    UseList.clear();
  }

  // The AllocaPoint is not use any more
  AllocaPoint->eraseFromParent();

  return false;
}

void ScalarDataRef::releaseMemory() {
  clear();
}

char ScalarDataRef::ID = 0;

RegisterPass<ScalarDataRef> X("polly-scalar-data-ref",
                              "Polly - Scalar Data Reference Analysis",
                              false, true);

Pass *polly::createScalarDataRefPass() {
  return new ScalarDataRef();
}
