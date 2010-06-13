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

#include "polly/SCoPDetection.h"
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
#include "llvm/Support/ErrorHandling.h"

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
    assert((R.getNode(R.getEntry())->isSubRegion()
            || R.getParent()->getEntry() == entry)
           && "Expect the loop is the smaller or bigger region");
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
static bool isParameter(const SCEV *Var, Region &RefRegion, Region &CurRegion,
                        LoopInfo &LI, ScalarEvolution &SE) {
  assert(Var && "Var can not be null!");
  // Find the biggest loop that contain by RefR
  Loop *topL = 0;
  for(Region *CurR = &CurRegion, *TopR = RefRegion.getParent();
    CurR != TopR; CurR = CurR->getParent()) {
      assert(CurR && "Cur not expect to be null!");
      if (Loop *L = castToLoop(*CurR, LI))
        topL = L;
  }
  // The parameter is always loop invariant
  if (topL && !Var->isLoopInvariant(topL))
      return false;

  if (const SCEVAddRecExpr *AddRec = dyn_cast<SCEVAddRecExpr>(Var)) {
    DEBUG(dbgs() << "Find AddRec: " << *AddRec
      << " at region: " << RefRegion.getNameStr() << "\n");
    // The indvar only expect come from outer loop
    // Or from a loop whose backend taken count could not compute.
    assert((AddRec->getLoop()->contains(getScopeLoop(RefRegion, LI))
            || isa<SCEVCouldNotCompute>(
                 SE.getBackedgeTakenCount(AddRec->getLoop())))
           && "Where comes the indvar?");
    return true;
  } else if (const SCEVUnknown *U = dyn_cast<SCEVUnknown>(Var)) {
    // Some SCEVUnknown will depend on loop variant or conditions:
    // 1. Phi node depend on conditions
    if (PHINode *phi = dyn_cast<PHINode>(U->getValue()))
      // If the phinode contained in the non-entry block of current region,
      // it is not invariant but depend on conditions.
      // TODO: maybe we need special analysis for phi node?
      if (RefRegion.contains(phi) && (RefRegion.getEntry() != phi->getParent()))
        return false;
    // TODO: add others conditions.
    return true;
  }
  // FIXME: Should us accept cast?
  else if (const SCEVCastExpr *Cast = dyn_cast<SCEVCastExpr>(Var))
    return isParameter(Cast->getOperand(), RefRegion, CurRegion,LI, SE);
  // Not a SCEVUnknown.
  return false;
}

static bool isIndVar(const SCEV *Var,
                     Region &RefRegion, Region &CurRegion,
                     LoopInfo &LI, ScalarEvolution &SE) {
  assert(RefRegion.contains(&CurRegion)
         && "Expect reference region contain current region!");
  const SCEVAddRecExpr *AddRec = dyn_cast<SCEVAddRecExpr>(Var);
  // Not an Induction variable
  if (!AddRec) return false;

  // If the addrec is the indvar of anly loop that containing current region
  for(Region *CurR = &CurRegion, *TopR = RefRegion.getParent();
      CurR != TopR; CurR = CurR->getParent()) {
    assert(CurR && "RefRegion not Contain CurRegion?");
    if (Loop *L = castToLoop(*CurR, LI))
      if (AddRec->getLoop() == L)
        return true;
  }
  // If the loop of addrec is not containing current region, that maybe:
  // 1. The loop is containing reference region and this expect to
  //    recognize as parameter
  // 2. The loop is containing by reference region, but not containing the
  //    current region, this because the loop backedge taken count is could
  //    not compute because Var is expect to get by "getSCEVAtScope", and
  //    this means reference region is not valid
  assert((AddRec->getLoop()->contains(getScopeLoop(RefRegion, LI))
          || isa<SCEVCouldNotCompute>(
            SE.getBackedgeTakenCount(AddRec->getLoop())))
        && "getAtScope not work?");
  return false;
}


//===----------------------------------------------------------------------===//
/// Helper Class
static void setCoefficient(const SCEV *Coeff, mpz_t v, bool negative) {
  if (Coeff) { // If the coefficient exist
    const SCEVConstant *C = dyn_cast<SCEVConstant>(Coeff);
    const APInt &CI = C->getValue()->getValue();
    // Convert i >= expr to i - expr >= 0
    MPZ_from_APInt(v, negative ?(-CI):CI);
  } else
    isl_int_set_si(v, 0);
}

static SCEVAffFunc::MemAccTy extractMemoryAccess(Instruction &Inst) {
  assert((isa<LoadInst>(&Inst) || isa<StoreInst>(&Inst))
    && "Only accept Load or Store!");

  // Try to handle the load/store.
  Value *Pointer = 0;
  SCEVAffFunc::SCEVAffFuncType AccType = SCEVAffFunc::None;

  if (LoadInst *load = dyn_cast<LoadInst>(&Inst)) {
    Pointer = load->getPointerOperand();
    AccType = SCEVAffFunc::ReadMem;
  } else if (StoreInst *store = dyn_cast<StoreInst>(&Inst)) {
    Pointer = store->getPointerOperand();
    AccType = SCEVAffFunc::WriteMem;
  } else
    llvm_unreachable("Already check in the assert");

  assert(Pointer && "Why pointer is null?");
  return SCEVAffFunc::MemAccTy(Pointer, AccType);
}

polly_constraint *SCEVAffFunc::toLoopBoundConstrain(polly_ctx *ctx,
                                   polly_dim *dim,
                                   const SmallVectorImpl<const SCEV*> &IndVars,
                                   const SmallVectorImpl<const SCEV*> &Params)
                                   const {
   unsigned num_in = IndVars.size(),
     num_param = Params.size();

   polly_constraint *c = isl_inequality_alloc(isl_dim_copy(dim));
   isl_int v;
   isl_int_init(v);

   for (unsigned i = 0, e = num_in; i != e; ++i) {
     setCoefficient(getCoeff(IndVars[i]), v, false);
     isl_constraint_set_coefficient(c, isl_dim_set, i, v);
   }

   // Setup the coefficient of parameters
   for (unsigned i = 0, e = num_param; i != e; ++i) {
     setCoefficient(getCoeff(Params[i]), v, false);
     isl_constraint_set_coefficient(c, isl_dim_param, i, v);
   }

   // Set the const.
   setCoefficient(TransComp, v, false);
   isl_constraint_set_constant(c, v);
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

  // Do not touch the current iterator.
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
    WriteAsOperand(OS, getBaseAddr(), false);
    OS << "[";
  }

  for (LnrTransSet::const_iterator I = LnrTrans.begin(), E = LnrTrans.end();
    I != E; ++I)
    OS << *I->second << " * " << *I->first << " + ";

  if (TransComp)
    OS << *TransComp;

  if (isDataRef())
    OS << "]";

  if (getType() == SCEVAffFunc::GE)
    OS << " >= 0";
  else if (getType() == SCEVAffFunc::Eq)
    OS << " == 0";
  else if (getType() == SCEVAffFunc::Ne)
    OS << " != 0";

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
    (at->second)[0].print(OS,SE);
    OS << ", ";
    (at->second)[1].print(OS,SE);
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
// SCoPDetection Implementation.

void SCoPDetection::buildAffineFunction(const SCEV *S, SCEVAffFunc &FuncToBuild,
                                        TempSCoP &SCoP) const {
  assert(S && "S can not be null!");

  assert(!isa<SCEVCouldNotCompute>(S)
    && "Un Expect broken affine function in SCoP!");

  Loop *Scope = getScopeLoop(SCoP.getMaxRegion(), *LI);

  // Compute S at the smallest loop so the addrec from other loops may
  // evaluate to constant.
  S = SE->getSCEVAtScope(S, Scope);

  for (AffineSCEVIterator I = affine_begin(S, SE), E = affine_end();
      I != E; ++I) {
    // The constant part must be a SCEVConstant.
    // TODO: support sizeof in coefficient.
    assert(isa<SCEVConstant>(I->second) && "Expect SCEVConst in coefficient!");

    const SCEV *Var = I->first;
    // Extract the constant part
    if(isa<SCEVConstant>(Var))
      // Add the translation component
      FuncToBuild.TransComp = I->second;
    else if (Var->getType()->isPointerTy()) { // Extract the base address
      const SCEVUnknown *BaseAddr = dyn_cast<SCEVUnknown>(Var);
      assert(BaseAddr && "Why we got a broken scev?");
      FuncToBuild.BaseAddr = BaseAddr->getValue();
    } else { // Extract others affine component
      FuncToBuild.LnrTrans.insert(*I);
      // Do not add the indvar to the parameter list
      if (!isIndVar(Var, SCoP.getMaxRegion(), SCoP.getMaxRegion(), *LI, *SE)) {
        assert(isParameter(Var, SCoP.getMaxRegion(),
                           SCoP.getMaxRegion(), *LI, *SE)
               && "Find non affine function in scop!");
        SCoP.getParamSet().insert(Var);
      }
    }
  }
}

bool SCoPDetection::isValidAffineFunction(const SCEV *S, Region &RefRegion,
                                          Region &CurRegion,
                                          bool isMemAcc) const {
  bool PtrExist = false;
  assert(S && "S can not be null!");

  if (isa<SCEVCouldNotCompute>(S))
    return false;

  Loop *Scope = getScopeLoop(CurRegion, *LI);

  // Compute S at the smallest loop so the addrec from other loops may
  // evaluate to constant.
  S = SE->getSCEVAtScope(S, Scope);

  for (AffineSCEVIterator I = affine_begin(S, SE), E = affine_end();
      I != E; ++I) {
    // The constant part must be a SCEVConstant.
    // TODO: support sizeof in coefficient.
    if (!isa<SCEVConstant>(I->second))
      return false;

    const SCEV *Var = I->first;
    // The constant offset is affine.
    if(isa<SCEVConstant>(Var))
      continue;

    // The pointer is ok.
    if (Var->getType()->isPointerTy()) {
      // If this is not expect a memory access
      if (!isMemAcc) return false;

      DEBUG(dbgs() << "Find pointer: " << *Var <<"\n");
      assert(I->second->isOne()
        && "The coefficient of pointer expect is one!");
      const SCEVUnknown *BaseAddr = dyn_cast<SCEVUnknown>(Var);

      if (!BaseAddr) return false;

      assert(!PtrExist && "We got two pointer?");
      PtrExist = true;
      continue;
    }

    // Check if it is the parameter of reference region.
    // Or it is some induction variable
    if (isParameter(Var, RefRegion, CurRegion, *LI, *SE)
      || isIndVar(Var, RefRegion, CurRegion, *LI, *SE))
      continue;

    // A bad SCEV found.
    DEBUG(dbgs() << "Bad SCEV: " << *Var << " at loop"
      << (getScopeLoop(CurRegion, *LI) ?
        getScopeLoop(CurRegion, *LI)->getHeader()->getName():"Top Level")
      << "Cur Region: " << CurRegion.getNameStr()
      << "Ref Region: " << RefRegion.getNameStr()
      << "\n");
    return false;
  }
  return !isMemAcc || PtrExist;
}

bool SCoPDetection::isValidCFG(BasicBlock &BB,
                               Region &RefRegion, Region &CurRegion) const {
  TerminatorInst *TI = BB.getTerminator();

  // Return instructions are only valid if the region is the top level region.
  if (isa<ReturnInst>(TI) && !RefRegion.getExit() && TI->getNumOperands() == 0)
    return true;

  BranchInst *Br = dyn_cast<BranchInst>(TI);

  if (!Br) return false;
  if (Br->isUnconditional()) return true;

  // Only instructions and constant expressions are valid branch conditions.
  if (!(isa<Constant>(Br->getCondition())
        || isa<Instruction>(Br->getCondition())))
    return false;

  Loop *L = LI->getLoopFor(&BB);

  // Conditions are not yet supported, except at loop exits.
  // TODO: Allow conditions in structured CFGs.
  if (!L || L->getExitingBlock() != &BB) {
    DEBUG(dbgs() << "Bad BB in cfg: " << BB.getName() << "\n");
    STATSCOP(CFG);
    return false;
  }

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
                                        Region &RefRegion, Region &CurRegion) const {
  SCEVAffFunc::MemAccTy MemAcc = extractMemoryAccess(Inst);

  if (!isValidAffineFunction(SE->getSCEV(MemAcc.first),
                             RefRegion, CurRegion, true)) {
    DEBUG(dbgs() << "Bad memory addr "
                 << *SE->getSCEV(MemAcc.first) << "\n");
    STATSCOP(AffFunc);
    return false;
  }
  // FIXME: Why expression like int *j = **k; where k has int ** type can pass
  //        affine function check?
  return true;
}

void SCoPDetection::buildScalarDataRef(Instruction &Inst,
                                         AccFuncSetType &ScalarAccs) {
  SmallVector<Value*, 4> Defs;
  SDR->getAllUsing(Inst, Defs);
  // Capture scalar read access.
  for (SmallVector<Value*, 4>::iterator VI = Defs.begin(),
    VE = Defs.end(); VI != VE; ++VI)
      ScalarAccs.push_back(SCEVAffFunc(SCEVAffFunc::ReadMem, *VI));
  // And write access.
  if (SDR->isDefExported(Inst))
    ScalarAccs.push_back(SCEVAffFunc(SCEVAffFunc::WriteMem, &Inst));
}

void SCoPDetection::buildAccessFunctions(TempSCoP &SCoP, BasicBlock &BB,
                                           AccFuncSetType &Functions) {
  for (BasicBlock::iterator I = BB.begin(), E = --BB.end(); I != E; ++I) {
    Instruction &Inst = *I;
    // Extract scalar "memory access" functions.
    buildScalarDataRef(Inst, Functions);
    // and the non-scalar one.
    if (isa<LoadInst>(&Inst) || isa<StoreInst>(&Inst)) {
      SCEVAffFunc::MemAccTy MemAcc = extractMemoryAccess(Inst);
      // Make the access function.
      Functions.push_back(SCEVAffFunc(MemAcc.second));
      // And build the access function
      buildAffineFunction(SE->getSCEV(MemAcc.first), Functions.back(), SCoP);
    }
  }
}

bool SCoPDetection::isValidInstruction(Instruction &Inst,
                                       Region &RefRegion, Region &CurRegion) const {
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
    return isValidMemoryAccess(Inst, RefRegion, CurRegion);

  // We do not know this instruction, therefore we assume it is invalid.
  STATSCOP(Other);
  return false;
}

bool SCoPDetection::isValidBasicBlock(BasicBlock &BB,
                                      Region &RefRegion, Region &CurRegion) const {

  // Check all instructions, except the terminator instruction.
  for (BasicBlock::iterator I = BB.begin(), E = --BB.end(); I != E; ++I)
    if (!isValidInstruction(*I, RefRegion, CurRegion))
      return false;

  return true;
}

bool SCoPDetection::isValidLoop(Loop *L, Region &RefRegion, Region &CurRegion) const {
  // We can only handle loops whose induction variables in are in canonical
  // form.
  PHINode *IndVar = L->getCanonicalInductionVariable();
  Instruction *IndVarInc = L->getCanonicalInductionVariableIncrement();

  if (!IndVar || !IndVarInc) {
    DEBUG(dbgs() << "No canonical iv for loop : " << L->getHeader()->getName()
          << "\n");
    STATSCOP(IndVar);
    return false;
  }

  const SCEV *LoopCount = SE->getBackedgeTakenCount(L);

  DEBUG(dbgs() << "Backedge taken count: " << *LoopCount << "\n");

  // We can not handle the loop if its loop bounds can not be computed.
  if (isa<SCEVCouldNotCompute>(LoopCount)) {
    STATSCOP(LoopBound);
    return false;
  }

  // The AffineSCEVIterator will always return the induction variable
  // which start from 0, and step by 1.
  const SCEV *LB = SE->getIntegerSCEV(0, LoopCount->getType()),
        *UB = LoopCount;

  // Check the lower bound.
  if (!isValidAffineFunction(LB, RefRegion, CurRegion, false)
      || !isValidAffineFunction(UB, RefRegion, CurRegion, false)){
    STATSCOP(AffFunc);
    return false;
  }

  return true;
}

void SCoPDetection::buildLoopBounds(TempSCoP &SCoP) {
  if (Loop *L = castToLoop(SCoP.getMaxRegion(), *LI)) {
    Value *IV = L->getCanonicalInductionVariable();
    assert(IV && "Valid SCoP will always have an IV!");
    const SCEV *SIV = SE->getSCEV(IV);
    // Get the loop bounds
    // FIXME: The lower bound is always 0.
    const SCEV *LB = SE->getIntegerSCEV(0, SIV->getType());
    // IV >= LB ==> IV - LB >= 0
    LB = SE->getMinusSCEV(SIV, LB);

    const SCEV *UB = SE->getBackedgeTakenCount(L);
    // Match the type, the type of BackedgeTakenCount mismatch when we have
    // something like this in loop exit:
    //    br i1 false, label %for.body, label %for.end
    // In fact, we could do some optimization before SCoPDetecion, so we do not
    // need to worry about this.
    // Be careful of the sign of the upper bounds, if we meet iv <= -1
    // this means the iterate domain is empty since iv >= 0
    // but if we do a zero extend, this will make a non-empty domain
    UB = SE->getTruncateOrSignExtend(UB, SIV->getType());
    // IV <= UB ==> UB - IV >= 0
    UB = SE->getMinusSCEV(UB, SIV);

    BBCond &affbounds = LoopBounds[L];
    affbounds.push_back(SCEVAffFunc(SCEVAffFunc::GE));
    // Build the affine function of loop bounds
    buildAffineFunction(LB, affbounds[0], SCoP);
    affbounds.push_back(SCEVAffFunc(SCEVAffFunc::GE));
    buildAffineFunction(UB, affbounds[1], SCoP);

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

void SCoPDetection::buildCondition(BasicBlock *BB, BasicBlock *RegionEntry,
                                   BBCond &Cond) {
  DomTreeNode *BBNode = DT->getNode(BB), *EntryNode = DT->getNode(RegionEntry);
  // FIXME: Would this happen?
  assert(BBNode && EntryNode && "Get null node while building condition!");
  DEBUG(dbgs() << "Build condition from " << RegionEntry->getName() << " to "
    << BB->getName() << ":  ");

  // Find all Br condition on the path
  while (BBNode != EntryNode) {
    BasicBlock *CurBB = BBNode->getBlock();
    // Move up
    BBNode = BBNode->getIDom();
    assert(BBNode && "BBNode should not reach the root node!");

    // Do add any condition if BB post dominate IDomBB, because
    // if we could reach IDomBB, we will always able to reach BB,
    // that means there is no any condition constrains from IDomBB to BB
    if (PDT->dominates(CurBB, BBNode->getBlock()))
      continue;

    BranchInst *Br = dyn_cast<BranchInst>(BBNode->getBlock()->getTerminator());
    assert(Br && "Valid SCoP should only contain br or ret at this moment");

    // If br is unconditional, BB will post dominates Its IDom Block, or the
    // BBNode should not be the immediately dominator of CurBB
    assert(Br->isConditional() && "Br should be conditional!");

    // Is branching to the true side will reach CurBB?
    bool inverted = DT->dominates(Br->getSuccessor(0), CurBB);
    DEBUG(
      if (inverted) dbgs() << '!';
      WriteAsOperand(dbgs(), Br->getCondition(), false);
      dbgs() << " && ";
    );
    // Build the condition in SCEV form
  }
  DEBUG(dbgs() << '\n');
}

TempSCoP *SCoPDetection::buildTempSCoP(Region &R) {
  TempSCoP *SCoP = new TempSCoP(R, LoopBounds, AccFuncMap);
  AccFuncSetType AccFuncs;
  BBCond Cond;
  for (Region::element_iterator I = R.element_begin(), E = R.element_end();
      I != E; ++I) {
    Cond.clear();
    // Build the condition
    buildCondition(I->getEntry(), R.getEntry(), Cond);
    // Remember it if there is any condition extracted

    if (I->isSubRegion()) {
      TempSCoP *SubSCoP = getTempSCoP(*I->getNodeAs<Region>());

      // Merge parameters from sub SCoPs.
      mergeParams(R, SCoP->getParamSet(), SubSCoP->getParamSet());
      // Update the loop depth.
      if (SubSCoP->MaxLoopDepth > SCoP->MaxLoopDepth)
        SCoP->MaxLoopDepth = SubSCoP->MaxLoopDepth;
    } else {
      // Extract access function of BasicBlocks.
      BasicBlock &BB = *(I->getNodeAs<BasicBlock>());
      AccFuncs.clear();
      buildAccessFunctions(*SCoP, BB, AccFuncs);
      if (!AccFuncs.empty()) {
        AccFuncSetType &Accs = AccFuncMap[&BB];
        Accs.insert(Accs.end(), AccFuncs.begin(), AccFuncs.end());
      }
    }
  }
  // Try to extract the loop bounds
  buildLoopBounds(*SCoP);
  return SCoP;
}

void SCoPDetection::mergeParams(Region &R, ParamSetType &Params,
                                   ParamSetType &SubParams) const {
  Loop *L = castToLoop(R, *LI);
  // Merge the parameters.
  for (ParamSetType::iterator I = SubParams.begin(),
    E = SubParams.end(); I != E; ++I) {
      const SCEV *Param = *I;
      // The valid parameter in subregion may not valid in its parameter
      if (isParameter(Param, R, R, *LI, *SE)) {
        Params.insert(Param);
        continue;
      }
      // Param maybe the indvar of the loop at current level.
      else if (const SCEVAddRecExpr *AddRec = dyn_cast<SCEVAddRecExpr>(Param)) {
        DEBUG(dbgs() << "Find AddRec: " << *AddRec
          << " at region: " << R.getNameStr() << "\n");
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
      llvm_unreachable("Unexpect bad parameter!");
  }
}

void SCoPDetection::runOnRegion(Region &R) {
  // FIXME: We visit the same region multiple times.
  for (Region::iterator I = R.begin(), E = R.end(); I != E; ++I)
    // TODO: Analyse the failure and hide the region.
    runOnRegion(**I);

  // Check current region.
  if (!isSCoP(R))
    return;

  RegionToSCoPs.insert(std::make_pair(&R, (TempSCoP*)0));

  // Kill all temporary values that can be rewrite by SCEVExpander.
  killAllTempValFor(R);
}

bool SCoPDetection::isSCoP(Region &R) const {
  return isValidRegion(R, R);
}

bool SCoPDetection::isValidRegion(Region &RefRegion,
                                  Region &CurRegion) const {
  // Check if getScopeLoop work on the current loop nest and region tree,
  // if it not work, we could not handle any further
  if (getScopeLoop(CurRegion, *LI)
      != LI->getLoopFor(CurRegion.getEntry())) {
    STATSCOP(LoopNest);
    return false;
  }

  // Visit all sub region node.
  for (Region::element_iterator I = CurRegion.element_begin(),
       E = CurRegion.element_end(); I != E; ++I) {
    if (I->isSubRegion()) {
      Region &subR = *(I->getNodeAs<Region>());
      if (!isValidRegion(RefRegion, subR))
        return false;
    } else {
      BasicBlock &BB = *(I->getNodeAs<BasicBlock>());

      if (isValidCFG(BB, RefRegion, CurRegion)
          && isValidBasicBlock(BB, RefRegion, CurRegion))
        continue;

      DEBUG(dbgs() << "Bad BB found:" << BB.getName() << "\n");
      return false;
    }
  }

  Loop *L = castToLoop(CurRegion, *LI);

  if (L && !isValidLoop(L, RefRegion, CurRegion))
    return false;

  return true;
}

void SCoPDetection::killAllTempValFor(const Region &R) {
  // Do not kill tempval in not valid region
  assert(RegionToSCoPs.count(&R)
    && "killAllTempValFor only work on valid region");

  for (Region::const_element_iterator I = R.element_begin(),
      E = R.element_end();I != E; ++I)
    if (!I->isSubRegion())
      killAllTempValFor(*I->getNodeAs<BasicBlock>());

  if (Loop *L = castToLoop(R, *LI))
    killAllTempValFor(*L);
}

void SCoPDetection::killAllTempValFor(Loop &L) {
  PHINode *IndVar = L.getCanonicalInductionVariable();
  Instruction *IndVarInc = L.getCanonicalInductionVariableIncrement();

  assert(IndVar && IndVarInc && "Why these are null in a valid region?");

  // Take away the Induction Variable and its increment
  SDR->killTempRefFor(*IndVar);
  SDR->killTempRefFor(*IndVarInc);
}

void SCoPDetection::killAllTempValFor(BasicBlock &BB) {
  for (BasicBlock::iterator I = BB.begin(), E = --BB.end(); I != E; ++I){
    Instruction &Inst = *I;
    if (isa<LoadInst>(&Inst) || isa<StoreInst>(&Inst)) {
      SCEVAffFunc::MemAccTy MemAcc = extractMemoryAccess(Inst);
      DEBUG(dbgs() << "Reduce ptr of " << Inst << "\n");
      // The address express as affine function can be rewrite by SCEV.
      // FIXME: Do not kill the not affine one in irregular scop?
      if (Instruction *GEP = dyn_cast<Instruction>(MemAcc.first))
        SDR->killTempRefFor(*GEP);
    }
  }

  // TODO: support switch
  if(BranchInst *Br = dyn_cast<BranchInst>(BB.getTerminator()))
    if (Br->getNumSuccessors() > 1)
      if(Instruction *Cond = dyn_cast<Instruction>(Br->getCondition()))
        // The affine condition also could be rewrite.
        SDR->killTempRefFor(*Cond);
}

TempSCoP *SCoPDetection::getTempSCoPFor(const Region* R) const {
  TempSCoPMapType::const_iterator at = RegionToSCoPs.find(R);
  if (at == RegionToSCoPs.end())
    return 0;

  // Recalculate the temporary SCoP info.
  return const_cast<SCoPDetection*>(this)->getTempSCoP(*const_cast<Region*>(R));
}

bool SCoPDetection::runOnFunction(llvm::Function &F) {
  DT = &getAnalysis<DominatorTree>();
  PDT = &getAnalysis<PostDominatorTree>();
  SE = &getAnalysis<ScalarEvolution>();
  LI = &getAnalysis<LoopInfo>();
  RI = &getAnalysis<RegionInfo>();
  SDR = &getAnalysis<ScalarDataRef>();
  Region *TopRegion = RI->getTopLevelRegion();

  ParamSetType Params;
  runOnRegion(*TopRegion);
  return false;
}

void SCoPDetection::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequiredTransitive<DominatorTree>();
  AU.addRequiredTransitive<PostDominatorTree>();
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
