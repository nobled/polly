//===------ SCoPDetection.cpp  - Detect SCoPs in LLVM Function ---*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Detect SCoPs in LLVM Function and extract loop bounds, access functions and
// conditions while checking.
//
//===----------------------------------------------------------------------===//

#include "SCoPDetection.h"
#include "polly/Support/GmpConv.h"
#include "polly/Support/AffineSCEVIterator.h"

#include "llvm/Intrinsics.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/RegionIterator.h"
#include "llvm/Analysis/ScalarEvolutionExpressions.h"
#include "llvm/Assembly/Writer.h"
#include "llvm/Support/CFG.h"
#include "llvm/Support/CallSite.h"
#include "llvm/Support/CommandLine.h"

#define DEBUG_TYPE "polly-scop-detect"
#include "llvm/Support/Debug.h"


#include "isl_constraint.h"

using namespace llvm;
using namespace polly;

static cl::opt<bool>
PrintTopSCoPOnly("print-top-scop-only", cl::desc("Print out subSCoP."),
              cl::Hidden);

static cl::opt<bool>
PrintTempSCoPInDetail("polly-print-temp-scop-in-detail",
                    cl::desc("Print the temporary scop information in detail"),
                    cl::Hidden);

//===----------------------------------------------------------------------===//
// Some statistic

#define BADSCOP_STAT(NAME, DESC) STATISTIC(Bad##NAME##ForSCoP, \
                                           "Number of bad regions for SCoP: "\
                                           DESC)


#define STATSCOP(NAME);              \
  ++Bad##NAME##ForSCoP;

// Note: This will make loop bounds could not compute,
// but this is checked before loop bounds.
BADSCOP_STAT(LoopNest,    "Some loop have multiple exit");

BADSCOP_STAT(CFG,         "CFG too complex");

BADSCOP_STAT(IndVar,      "Cant canonical induction variable in loop");

BADSCOP_STAT(LoopBound,   "Loop bound could not compute");

BADSCOP_STAT(FuncCall,    "Function call occur");

BADSCOP_STAT(AffFunc,     "Expression not affine");

BADSCOP_STAT(Other,       "Others");

//===----------------------------------------------------------------------===//
// Temporary Hack for extended region tree.

// Cast the region to loop if there is a loop have the same header and exit.
Loop *polly::castToLoop(const Region &R, LoopInfo &LI) {
  BasicBlock *entry = R.getEntry();

  if (!LI.isLoopHeader(entry))
    return 0;

  Loop *L = LI.getLoopFor(entry);

  BasicBlock *exit = L->getExitBlock();

  // Is the loop with multiple exits?
  if (!exit) return 0;

  if (exit != R.getExit()) {
    // SubRegion/ParentRegion with the same entry.
    assert((R.getNode(R.getEntry())->isSubRegion() ||
      R.getParent()->getEntry() == entry) &&
      "Expect the loop is the smaller or bigger region");
    return 0;
  }

  return L;
}

// Get the Loop containing all bbs of this region, for ScalarEvolution
// "getSCEVAtScope"
Loop *polly::getScopeLoop(const Region &R, LoopInfo &LI) {
  const Region *tempR = &R;
  while (tempR) {
    if (Loop *L = castToLoop(*tempR, LI))
      return L;

    tempR = tempR->getParent();
  }
  return 0;
}

//===----------------------------------------------------------------------===//
// Helper functions

// Helper function to check parameter
static bool isParameter(const SCEV *Var, Region &R,
                        LoopInfo &LI, ScalarEvolution &SE) {
  assert(Var && "Var can not be null!");

  // The parameter is always loop invariant
  if (Loop *L = castToLoop(R, LI))
    if (!Var->isLoopInvariant(L))
      return false;

  if (const SCEVAddRecExpr *AddRec = dyn_cast<SCEVAddRecExpr>(Var)) {
    // The indvar only expect come from outer loop
    // Or from a loop whose backend taken count could not compute.
    assert((AddRec->getLoop()->contains(getScopeLoop(R, LI)) ||
      isa<SCEVCouldNotCompute>(
        SE.getBackedgeTakenCount(AddRec->getLoop()))) &&
      "Where comes the indvar?");
    return true;
  }
  else if (const SCEVUnknown *U = dyn_cast<SCEVUnknown>(Var)) {
    // Some SCEVUnknown will depend on loop variant or conditions:
    // 1. Phi node depend on conditions
    if (PHINode *phi = dyn_cast<PHINode>(U->getValue()))
      // If the phinode contained in the non-entry block of current region,
      // it is not invariant but depend on conditions.
      // TODO: maybe we need special analysis for phi node?
      if (R.contains(phi) && (R.getEntry() != phi->getParent()))
        return false;
    // TODO: add others conditions.
    return true;
  }
  // Not a SCEVUnknown.
  return false;
}

static void setCoefficient(const SCEV *Coeff, mpz_t v, bool negative) {
  if (Coeff) { // If the coefficient exist
    const SCEVConstant *C = dyn_cast<SCEVConstant>(Coeff);
    const APInt &CI = C->getValue()->getValue();
    // Convert i >= expr to i - expr >= 0
    MPZ_from_APInt(v, negative ?(-CI):CI);
  }
  else
    isl_int_set_si(v, 0);
}
// TODO: Move to LoopInfo?
static bool isPreHeader(BasicBlock *BB, LoopInfo *LI) {
  TerminatorInst *TI = BB->getTerminator();
  return (TI->getNumSuccessors() == 1)
    && LI->isLoopHeader(TI->getSuccessor(0));
}

static SCEVAffFunc::MemAccTy extractMemoryAccess(Instruction &Inst) {
  assert((isa<LoadInst>(&Inst) || isa<StoreInst>(&Inst))
    && "Only accept Load or Store!");

  // Try to handle the load/store.
  Value *Pointer = 0;
  SCEVAffFunc::AccessType AccType = SCEVAffFunc::None;

  if (LoadInst *load = dyn_cast<LoadInst>(&Inst)) {
    Pointer = load->getPointerOperand();
    AccType = SCEVAffFunc::Read;
  } else if (StoreInst *store = dyn_cast<StoreInst>(&Inst)) {
    Pointer = store->getPointerOperand();
    AccType = SCEVAffFunc::Write;
  } // else unreachable because of the previous assert!

  assert(Pointer && "Why pointer is null?");
  return SCEVAffFunc::MemAccTy(Pointer, AccType);
}

//===----------------------------------------------------------------------===//
// SCEVAffFunc Implement

bool SCEVAffFunc::buildAffineFunc(const SCEV *S, Region &R, ParamSetType &Params,
                                  SCEVAffFunc *FuncToBuild,
                                  LoopInfo &LI, ScalarEvolution &SE,
                                  AccessType AccType) {
  assert(S && "S can not be null!");

  bool PtrExist = false;

  if (isa<SCEVCouldNotCompute>(S))
    return false;

  Loop *Scope = getScopeLoop(R, LI);

  // Compute S at scope first.
  S = SE.getSCEVAtScope(S, Scope);

  // FIXME: Simplify these code.
  for (AffineSCEVIterator I = affine_begin(S, &SE), E = affine_end();
      I != E; ++I){
    // The constant part must be a SCEVConstant.
    // TODO: support sizeof in coefficient.
    if (!isa<SCEVConstant>(I->second)) return false;

    const SCEV *Var = I->first;

    // Ignore the constant offset.
    if(isa<SCEVConstant>(Var)) {
      // Add the translation component
      if (FuncToBuild)
        FuncToBuild->TransComp = I->second;
      continue;
    }

    // Ignore the pointer.
    if (Var->getType()->isPointerTy()) {
      // If this is not expect a memory access
      if (AccType == SCEVAffFunc::None)
        return false;

      DEBUG(dbgs() << "Find pointer: " << *Var <<"\n");
      assert(I->second->isOne() && "The coefficient of pointer expect is one!");
      const SCEVUnknown *BaseAddr = dyn_cast<SCEVUnknown>(Var);

      if (!BaseAddr) return false;

      PtrExist = true;
      // Setup the base address
      if (FuncToBuild)
        FuncToBuild->BaseAddr.setPointer(BaseAddr->getValue());
      continue;
    }

    // Dirty Hack: Some pre-processing for parameter.
    if (const SCEVCastExpr *Cast = dyn_cast<SCEVCastExpr>(Var)) {
      DEBUG(dbgs() << "Warning: Ignore cast expression: " << *Cast << "\n");
      // Dirty hack. replace the cast expression by its operand.
      Var = Cast->getOperand();
    }

    // Build the affine function.
    if (FuncToBuild)
      FuncToBuild->LnrTrans.insert(*I);

    // Check if the parameter valid.
    if (!isParameter(Var, R, LI, SE)) {
      // If Var not a parameter, it may be the indvar of current loop
      if (const SCEVAddRecExpr *AddRec = dyn_cast<SCEVAddRecExpr>(Var)){
        assert((AddRec->getLoop() == Scope ||
          isa<SCEVCouldNotCompute>(
            SE.getBackedgeTakenCount(AddRec->getLoop())))
          && "getAtScope not work?");
        continue;
      }
      // A bad SCEV found.
      DEBUG(dbgs() << "Bad SCEV: " << *Var << " in " << *S << " at "
        << (Scope?Scope->getHeader()->getName():"Top Level")
        << "\n");
      return false;
    }

    // Add the loop invariants to parameter lists.
    Params.insert(Var);
  }

  // The SCEV is valid if it is not a memory access
  return AccType == SCEVAffFunc::None ||
    // Otherwise, there must a pointer in exist in the expression.
    PtrExist;
}

bool SCEVAffFunc::buildMemoryAccess(MemAccTy MemAcc, Region &R, ParamSetType &Params,
                                    SCEVAffFunc *FuncToBuild,
                                    LoopInfo &LI, ScalarEvolution &SE) {
  return buildAffineFunc(SE.getSCEV(MemAcc.getPointer()),
                         R, Params, FuncToBuild, LI, SE, MemAcc.getInt());
}

polly_constraint *SCEVAffFunc::toLoopBoundConstrain(polly_ctx *ctx,
                                   polly_dim *dim,
                                   const SmallVectorImpl<const SCEV*> &IndVars,
                                   const SmallVectorImpl<const SCEV*> &Params,
                                   bool isLower) const {
   unsigned num_in = IndVars.size(),
     num_param = Params.size();

   polly_constraint *c = isl_inequality_alloc(isl_dim_copy(dim));
   isl_int v;
   isl_int_init(v);

   // Dont touch the current iterator.
   for (unsigned i = 0, e = num_in - 1; i != e; ++i) {
     setCoefficient(getCoeff(IndVars[i]), v, isLower);
     isl_constraint_set_coefficient(c, isl_dim_set, i, v);
   }

   assert(!getCoeff(IndVars[num_in - 1]) &&
     "Current iterator should not have any coff.");
   // Set the coefficient of current iterator, convert i <= expr to expr - i >= 0
   isl_int_set_si(v, isLower?1:(-1));
   isl_constraint_set_coefficient(c, isl_dim_set, num_in - 1, v);

   // Setup the coefficient of parameters
   for (unsigned i = 0, e = num_param; i != e; ++i) {
     setCoefficient(getCoeff(Params[i]), v, isLower);
     isl_constraint_set_coefficient(c, isl_dim_param, i, v);
   }

   // Set the const.
   setCoefficient(TransComp, v, isLower);
   isl_constraint_set_constant(c, v);
   // Free v
   isl_int_clear(v);
   return c;
}

polly_constraint *SCEVAffFunc::toAccessFunction(polly_ctx *ctx, polly_dim* dim,
                                    const SmallVectorImpl<Loop*> &NestLoops,
                                    const SmallVectorImpl<const SCEV*> &Params,
                                    ScalarEvolution &SE) const {
  polly_constraint *c = isl_equality_alloc(isl_dim_copy(dim));
  isl_int v;
  isl_int_init(v);

  isl_int_set_si(v, 1);
  isl_constraint_set_coefficient(c, isl_dim_out, 0, v);

  // Dont touch the current iterator.
  for (unsigned i = 0, e = NestLoops.size(); i != e; ++i) {
    Loop *L = NestLoops[i];
    Value *IndVar = L->getCanonicalInductionVariable();
    setCoefficient(getCoeff(SE.getSCEV(IndVar)), v, true);
    isl_constraint_set_coefficient(c, isl_dim_in, i, v);
  }

  // Setup the coefficient of parameters
  for (unsigned i = 0, e = Params.size(); i != e; ++i) {
    setCoefficient(getCoeff(Params[i]), v, true);
    isl_constraint_set_coefficient(c, isl_dim_param, i, v);
  }

  // Set the const.
  setCoefficient(TransComp, v, true);
  isl_constraint_set_constant(c, v);

  isl_int_clear(v);
  return c;
}

void SCEVAffFunc::print(raw_ostream &OS, ScalarEvolution *SE) const {
  // Print BaseAddr
  if (isDataRef()) {
    OS << (isRead() ? "Reads" : "Writes") << " ";
    WriteAsOperand(OS, BaseAddr.getPointer(), false);
    OS << "[";
  }

  for (LnrTransSet::const_iterator I = LnrTrans.begin(), E = LnrTrans.end();
    I != E; ++I)
    OS << *I->second << " * " << *I->first << " + ";

  if (TransComp)
    OS << *TransComp;

  if (isDataRef())
    OS << "]";

}

//===----------------------------------------------------------------------===//
// LLVMSCoP Implement

void TempSCoP::print(raw_ostream &OS, ScalarEvolution *SE, LoopInfo *LI) const {
  OS << "SCoP: " << R.getNameStr() << "\tParameters: (";
  // Print Parameters.
  for (ParamSetType::const_iterator PI = Params.begin(), PE = Params.end();
    PI != PE; ++PI)
    OS << **PI << ", ";

  OS << "), Max Loop Depth: "<< MaxLoopDepth <<"\n";

  if (!PrintTempSCoPInDetail)
    return;

  printDetail(OS, SE, LI, &R, 0);
}

void TempSCoP::printDetail(llvm::raw_ostream &OS, ScalarEvolution *SE,
                           LoopInfo *LI, const Region *CurR,
                           unsigned ind) const {
  // Print the loopbounds if current region is a loop
  BoundMapType::const_iterator at = LoopBounds.find(castToLoop(*CurR, *LI));
  if (at != LoopBounds.end()) {
    // Print the loop bounds if these is an loops
    OS.indent(ind) << "Bounds of Loop: " << at->first->getHeader()->getName()
      << ":\t{ ";
    at->second.first.print(OS,SE);
    OS << ", ";
    at->second.second.print(OS,SE);
    OS << "}\n";
    // Increase the indent
    ind += 2;
  }
  // Iterate the region nodes of this SCoP to print
  // the access function and loop bounds
  for (Region::const_element_iterator I = CurR->element_begin(),
      E = CurR->element_end(); I != E; ++I) {
    unsigned subInd = ind;
    if (I->isSubRegion()) {
      Region *subR = I->getNodeAs<Region>();
      printDetail(OS, SE, LI, subR, subInd);
    } else {
      unsigned bb_ind = ind + 2;
      AccFuncMapType::const_iterator at =
        AccFuncMap.find(I->getNodeAs<BasicBlock>());
      // Try to print the access functions
      if (at != AccFuncMap.end()) {
        OS.indent(ind) << "BB: " << at->first->getName() << "{\n";
        for (AccFuncSetType::const_iterator FI = at->second.begin(),
            FE = at->second.end(); FI != FE; ++FI) {
          FI->print(OS.indent(bb_ind),SE);
          OS << "\n";
        }
        OS.indent(ind) << "}\n";
      }
    }
  }
}

//===----------------------------------------------------------------------===//
// SCoPDetection Implement

bool SCoPDetection::isValidCFG(BasicBlock &BB, Region &R) {
  TerminatorInst *TI = BB.getTerminator();

  unsigned int numSucc = TI->getNumSuccessors();

  // Return and unconditional branch is ok.
  if (numSucc < 2) return true;

  BranchInst *Br = dyn_cast<BranchInst>(TI);
  // We dose not support switch at the moment.
  if (!Br) return false;

  // XXX: Should us preform some optimization to eliminate this?
  // It is the same as unconditional branch if the condition is constant.
  if (isa<Constant>(Br->getCondition())) return true;

  Instruction *Cond = dyn_cast<Instruction>(Br->getCondition());
  // And we only support instruction as condition now
  if (!Cond) return false;

  // Hide the Instructions for computing conditions.
  killAllTempValFor(*Cond);

  if (Loop *L = getScopeLoop(R, *LI)) {
    // Only allow branches that are loop exits. This stops anything
    // except loops that have just one exit and are detected by our LoopInfo
    // analysis
    // It is ok if BB is branching out of the loop
    if (L->isLoopExiting(&BB))
      return true;

    assert(L->getExitingBlock()
           && "The Loop return from getScopeLoop will always have 1 exit!");

    // Now bb is not the exit block of the loop.
    // Is BB branching to inner loop?
    for (unsigned int i = 0; i < numSucc; ++i) {
      BasicBlock *SuccBB = TI->getSuccessor(i);
      if (isPreHeader(SuccBB, LI) || LI->isLoopHeader(SuccBB)) {
        // If branching to inner loop
        // FIXME: We can only handle a bb branching to preheader or not.
        if (numSucc > 2) {
          STATSCOP(CFG);
          return false;
        }
        return true;
      }
    }

    // Now bb is not branching to inner loop.
    // FIXME: Handle the branch condition
    DEBUG(dbgs() << "Bad BB in cfg: " << BB.getName() << "\n");
    STATSCOP(CFG);
    return false;
  }

  // BB is not in any loop.
  // TODO: handle the branch condition
  return true;
}

bool SCoPDetection::isValidCallInst(CallInst &CI) {
  if (CI.mayHaveSideEffects() || CI.doesNotReturn())
    return false;

  if (CI.doesNotAccessMemory())
    return true;

  Function *CalledFunction = CI.getCalledFunction();

  // Indirect call is not support now.
  if (CalledFunction == 0)
    return false;

  // TODO: Intrinsics.
  return false;
}

bool SCoPDetection::isValidMemoryAccess(Instruction &Inst,
                                        Region &R, ParamSetType &Params) {
  SCEVAffFunc::MemAccTy MemAcc = extractMemoryAccess(Inst);

  if (!SCEVAffFunc::buildMemoryAccess(MemAcc, R, Params, 0, *LI, *SE)) {
    DEBUG(dbgs() << "Bad memory addr "
                 << SE->getSCEV(MemAcc.getPointer()) << "\n");
    STATSCOP(AffFunc);
    return false;
  }
  // FIXME: Why expression like int *j = **k; where k has int ** type can pass
  //        affine function check?

  DEBUG(dbgs() << "Reduce ptr of " << Inst << "\n");
  // Try to remove the temporary value for address computation
  // Do this in the Checking phase, so we will get the final result
  // when we try to get SCoP by "getTempSCoPFor";
  if (Instruction *GEP = dyn_cast<Instruction>(MemAcc.getPointer()))
    killAllTempValFor(*GEP);

  return true;
}

void SCoPDetection::captureScalarDataRef(Instruction &Inst,
                                         AccFuncSetType &ScalarAccs) {
  SmallVector<Value*, 4> Defs;
  SDR->getAllUsing(Inst, Defs);
  // Capture scalar read access.
  for (SmallVector<Value*, 4>::iterator VI = Defs.begin(),
    VE = Defs.end(); VI != VE; ++VI)
      ScalarAccs.push_back(SCEVAffFunc(SCEVAffFunc::Read, *VI));
  // And write access.
  if (SDR->isDefExported(Inst))
    ScalarAccs.push_back(SCEVAffFunc(SCEVAffFunc::Write, &Inst));
}

void SCoPDetection::extractAccessFunctions(TempSCoP &SCoP, BasicBlock &BB,
                                           AccFuncSetType &AccessFunctions) {
  for (BasicBlock::iterator I = BB.begin(), E = --BB.end(); I != E; ++I) {
    Instruction &Inst = *I;
    // Extract scalar "memory access" functions.
    captureScalarDataRef(Inst, AccessFunctions);
    // and the non-scalar one.
    if (isa<LoadInst>(&Inst) || isa<StoreInst>(&Inst)) {
      SCEVAffFunc::MemAccTy MemAcc = extractMemoryAccess(Inst);
      // Make the access function.
      AccessFunctions.push_back(SCEVAffFunc(MemAcc.getInt()));
      // And build the access function
      bool buildSuccessful = SCEVAffFunc::buildMemoryAccess(MemAcc,
        SCoP.R, SCoP.getParamSet(), &AccessFunctions.back(), *LI, *SE);
      assert(buildSuccessful && "Expect memory access is valid!");
    }
  }
}

bool SCoPDetection::isValidInstruction(Instruction &Inst, Region &R,
                                       ParamSetType &Params) {
  // We only check the call instruction but not invoke instruction.
  if (CallInst *CI = dyn_cast<CallInst>(&Inst)) {
    if (isValidCallInst(*CI))
      return true;

    DEBUG(dbgs() << "Bad call Inst!\n");
    STATSCOP(FuncCall);
    return false;
  }

  if (!Inst.mayWriteToMemory() && !Inst.mayReadFromMemory()) {
    // Handle cast instruction
    if (isa<IntToPtrInst>(Inst) || isa<BitCastInst>(Inst)) {
      DEBUG(dbgs() << "Bad cast Inst!\n");
      STATSCOP(Other);
      return false;
    }

    return true;
  }

  if (isa<LoadInst>(&Inst) || isa<StoreInst>(&Inst))
    return isValidMemoryAccess(Inst, R, Params);

  // We do not know this instruction, therefore we assume it is invalid.
  STATSCOP(Other);
  return false;
}

bool SCoPDetection::isValidBasicBlock(BasicBlock &BB,
                                      Region &R, ParamSetType &Params) {

  // Check all instructions, except the terminator instruction.
  for (BasicBlock::iterator I = BB.begin(), E = --BB.end(); I != E; ++I)
    if (!isValidInstruction(*I, R, Params))
      return false;

  return true;
}

bool SCoPDetection::hasValidLoopBounds(Region &R, ParamSetType &Params) {
  // Find the parameters used in loop bounds
  if (Loop *L = castToLoop(R, *LI)) {

    DEBUG(dbgs() << "Region : " << R.getNameStr()
      << " also is a loop: " <<(L ? L->getHeader()->getName() : "Not a loop")
      << "\n");

    // We can only handle loops whose induction variables in are in canonical
    // form.
    PHINode *IndVar = L->getCanonicalInductionVariable();
    Instruction *IndVarInc = L->getCanonicalInductionVariableIncrement();

    if ((IndVar == 0) || (IndVarInc == 0)) {
      DEBUG(dbgs() << "No CanIV for loop : " << L->getHeader()->getName() <<"?\n");
      STATSCOP(IndVar);
      return false;
    }

    // Take away the Induction Variable and its increment
    killAllTempValFor(*IndVar);
    killAllTempValFor(*IndVarInc);

    const SCEV *LoopCount = SE->getBackedgeTakenCount(L);

    DEBUG(dbgs() << "Backedge taken count: "<< *LoopCount <<"\n");

    // We can not handle the loop if its loop bounds can not be computed.
    if (isa<SCEVCouldNotCompute>(LoopCount)) {
      STATSCOP(LoopBound);
      return false;
    }

    // The AffineSCEVIterator will always return the induction variable
    // which start from 0, and step by 1.
    const SCEV *LB = SE->getIntegerSCEV(0, LoopCount->getType()),
               *UB = LoopCount;

    // Build the lower bound.
    if (!SCEVAffFunc::buildAffineFunc(LB, R, Params, 0, *LI, *SE)
        || !SCEVAffFunc::buildAffineFunc(UB, R, Params, 0, *LI, *SE)) {
      STATSCOP(AffFunc);
      return false;
    }
  }

  return true;
}

void SCoPDetection::extractLoopBounds(TempSCoP &SCoP) {
  if (Loop *L = castToLoop(SCoP.getMaxRegion(), *LI)) {
    // Get the loop bounds
    const SCEV *UB = SE->getBackedgeTakenCount(L);
    // FIXME: The lower bound is always 0.
    const SCEV *LB = SE->getIntegerSCEV(0, UB->getType());

    AffBoundType &affbounds = LoopBounds[L];
    // Build the affine function of loop bounds
    bool buildSuccessful =
      SCEVAffFunc::buildAffineFunc(
        LB, SCoP.getMaxRegion(), SCoP.getParamSet(), &affbounds.first, *LI, *SE)
      &&
      SCEVAffFunc::buildAffineFunc(
        UB, SCoP.getMaxRegion(), SCoP.getParamSet(), &affbounds.second, *LI, *SE);

    assert(buildSuccessful && "Expect loop bounds of SCoP are valid!");
    // Increase the loop depth because we found a loop.
    ++SCoP.MaxLoopDepth;
  }
}

TempSCoP *SCoPDetection::getTempSCoP(Region& R) {
  // Did we already compute the SCoP for R?
  TempSCoPMapType::const_iterator at = RegionToSCoPs.find(&R);
  assert(at != RegionToSCoPs.end() && "R is expect to be found!");
  if (at->second != 0)
    return at->second;

  // Otherwise, we had to extract the temporary SCoP information.
  TempSCoP *tempSCoP = buildTempSCoP(R);
  RegionToSCoPs[&R] = tempSCoP;
  return tempSCoP;
}

TempSCoP *SCoPDetection::buildTempSCoP(Region &R) {
  TempSCoP *SCoP = new TempSCoP(R, LoopBounds, AccFuncMap);
  for (Region::element_iterator I = R.element_begin(), E = R.element_end();
      I != E; ++I) {
    if (I->isSubRegion()) {
      TempSCoP *SubSCoP = getTempSCoP(*I->getNodeAs<Region>());
      // Merge parameters from sub SCoPs.
      bool mergeSuccess = tryMergeParams(R, SCoP->getParamSet(),
                                         SubSCoP->getParamSet());
      assert(mergeSuccess && "Bad scop found!");
      // Update the loop depth.
      if (SubSCoP->MaxLoopDepth > SCoP->MaxLoopDepth)
        SCoP->MaxLoopDepth = SubSCoP->MaxLoopDepth;
    } else {
      // Extract access function of BasicBlocks.
      BasicBlock &BB = *(I->getNodeAs<BasicBlock>());
      AccFuncSetType AccFuncs;
      extractAccessFunctions(*SCoP, BB, AccFuncs);
      if (!AccFuncs.empty()) {
        AccFuncSetType &Accs = AccFuncMap[&BB];
        Accs.insert(Accs.end(), AccFuncs.begin(), AccFuncs.end());
      }
    }
  }
  // Try to extract the loop bounds
  extractLoopBounds(*SCoP);
  return SCoP;
}

bool SCoPDetection::tryMergeParams(Region &R, ParamSetType &Params,
                                   ParamSetType &SubParams) {
  Loop *L = castToLoop(R, *LI);
  // Merge the parameters.
  for (ParamSetType::iterator I = SubParams.begin(),
    E = SubParams.end(); I != E; ++I) {
      const SCEV *Param = *I;
      // The valid parameter in subregion may not valid in its parameter
      if (isParameter(Param, R, *LI, *SE)) {
        Params.insert(Param);
        continue;
      }
      // Param maybe the indvar of the loop at current level.
      else if (const SCEVAddRecExpr *AddRec = dyn_cast<SCEVAddRecExpr>(Param)) {
        if ( L == AddRec->getLoop())
          continue;
        // Else it is a invalid parameter.
        assert((!L || AddRec->getLoop()->contains(L) ||
          isa<SCEVCouldNotCompute>(
          SE->getBackedgeTakenCount(AddRec->getLoop())))
          && "Where comes the indvar?");
      }

      DEBUG(dbgs() << "Bad parameter in parent: " << *Param
        << " in " << R.getNameStr() << " at "
        << (L?L->getHeader()->getName():"Top Level")
        << "\n");
      return false;
  }
  return true;
}

bool SCoPDetection::isValidRegion(Region &R, ParamSetType &Params) {
  bool isValid = true;

  // Check if getScopeLoop work on the current loop nest and region tree,
  // if it not work, we could not handle any further
  if (getScopeLoop(R, *LI) != LI->getLoopFor(R.getEntry())) {
    STATSCOP(LoopNest);
    isValid = false;
  }
  ParamSetType SubParams;
  // Visit all sub region node.
  for (Region::element_iterator I = R.element_begin(), E = R.element_end();
      I != E; ++I){
    if (I->isSubRegion()) {
      SubParams.clear();
      Region *SubR = I->getNodeAs<Region>();
      // Extract information of sub scop and merge them.
      if (isValidRegion(*SubR, Params) ) {
        // We found a valid region.
        RegionToSCoPs.insert(std::make_pair(SubR, (TempSCoP*)0));
        if (tryMergeParams(R, Params, SubParams))
          continue;
        // Merge fail only because bad parameter occur in expression
        STATSCOP(AffFunc);
      }
      isValid = false;
    } else if (isValid) {
      // We check the basic blocks only the region is valid.
      BasicBlock &BB = *(I->getNodeAs<BasicBlock>());
      // Check CFG and all non terminator inst
      if (!isValidCFG(BB, R)
          || !isValidBasicBlock(BB, R, Params)){
          DEBUG(dbgs() << "Bad BB found:" << BB.getName() << "\n");
          isValid = false;
      }
    }
  }

  return isValid && hasValidLoopBounds(R, Params);
}

TempSCoP *SCoPDetection::getTempSCoPFor(const Region* R) const {
  TempSCoPMapType::const_iterator at = RegionToSCoPs.find(R);
  if (at == RegionToSCoPs.end())
    return 0;

  // Recalculate the temporary SCoP info.
  return const_cast<SCoPDetection*>(this)->getTempSCoP(*const_cast<Region*>(R));
}

bool SCoPDetection::runOnFunction(llvm::Function &F) {
  SE = &getAnalysis<ScalarEvolution>();
  LI = &getAnalysis<LoopInfo>();
  RI = &getAnalysis<RegionInfo>();
  SDR = &getAnalysis<ScalarDataRef>();
  Region *TopRegion = RI->getTopLevelRegion();

  ParamSetType Params;
  // Check if regions in functions is valid.
  if (isValidRegion(*TopRegion, Params))
    RegionToSCoPs.insert(std::make_pair(TopRegion, (TempSCoP*)0));

  return false;
}

void SCoPDetection::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequiredTransitive<LoopInfo>();
  AU.addRequiredTransitive<RegionInfo>();
  AU.addRequiredTransitive<ScalarEvolution>();
  AU.addRequiredTransitive<ScalarDataRef>();
  AU.setPreservesAll();
}

void SCoPDetection::clear() {
  LoopBounds.clear();
  AccFuncMap.clear();

  // Delete all tempSCoP entry in the maps.
  while (!RegionToSCoPs.empty()) {
    TempSCoPMapType::iterator I = RegionToSCoPs.begin();
    if (I->second)
      delete I->second;

    RegionToSCoPs.erase(I);
  }
}

/// Debug/Testing function

void SCoPDetection::print(raw_ostream &OS, const Module *) const {
  if (RegionToSCoPs.empty()) {
    OS << "No SCoP found!\n";
    return;
  }

  for (TempSCoPMapType::const_iterator I = RegionToSCoPs.begin(),
       E = RegionToSCoPs.end(); I != E; ++I)
    if (!PrintTopSCoPOnly || isMaxRegionInSCoP(*(I->first)))
      getTempSCoPFor(I->first)->print(OS, SE, LI);

  OS << "\n";
}

SCoPDetection::~SCoPDetection() {
  clear();
}

char SCoPDetection::ID = 0;

static RegisterPass<SCoPDetection>
X("polly-scop-detect", "Polly - Detect SCoPs");

// Do not link this pass. This pass suppose to be only used by SCoPInfo.
