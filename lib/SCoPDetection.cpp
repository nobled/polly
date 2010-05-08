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
#include "llvm/Analysis/RegionIterator.h"
#include "llvm/Analysis/ScalarEvolutionExpressions.h"
#include "llvm/Assembly/Writer.h"
#include "llvm/Support/CFG.h"
#include "llvm/Support/CallSite.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/ADT/Statistic.h"

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
                                           "The # of bad regions for SCoP: "\
                                           DESC)


#define STATSCOP(X);      \
  do { if (::llvm::DebugFlag && checkSCoPOnly &&\
           ::llvm::isCurrentDebugType("polly-scop-detect-stat")) { X; } \
  } while (0)

#define STATBAD(NAME);    STATSCOP(++Bad##NAME##ForSCoP);

STATISTIC(ValidRegion,"The # of regions that a valid part of SCoP");

#if 0
STATISTIC(ValidSCoP,  "The # of valid SCoP");

STATISTIC(RichSCoP,   "The # of valid SCoP with loop");
#endif

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

//===----------------------------------------------------------------------===//
// SCEVAffFunc Implement

bool SCEVAffFunc::buildAffineFunc(const SCEV *S, TempSCoP &SCoP,
                                  SCEVAffFunc *FuncToBuild,
                                  LoopInfo &LI, ScalarEvolution &SE,
                                  AccessType AccType) {
  assert(S && "S can not be null!");

  bool PtrExist = false;

  Region &R = SCoP.getMaxRegion();

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
        assert(AddRec->getLoop() == Scope && "getAtScope not work?");
        continue;
      }
      // A bad SCEV found.
      DEBUG(dbgs() << "Bad SCEV: " << *Var << " in " << *S << " at "
        << (Scope?Scope->getHeader()->getName():"Top Level")
        << "\n");
      return false;
    }

    // Add the loop invariants to parameter lists.
    SCoP.getParamSet().insert(Var);
  }

  // The SCEV is valid if it is not a memory access
  return AccType == SCEVAffFunc::None ||
    // Otherwise, there must a pointer in exist in the expression.
    PtrExist;
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

void TempSCoP::print(raw_ostream &OS, ScalarEvolution *SE) const {
  OS << "SCoP: " << R.getNameStr() << "\tParameters: (";
  // Print Parameters.
  for (ParamSetType::const_iterator PI = Params.begin(), PE = Params.end();
    PI != PE; ++PI)
    OS << **PI << ", ";

  OS << "), Max Loop Depth: "<< MaxLoopDepth <<"\n";

  if (!PrintTempSCoPInDetail)
    return;

  printBounds(OS, SE);

  printAccFunc(OS, SE);
}

void TempSCoP::printAccFunc(llvm::raw_ostream &OS, ScalarEvolution *SE) const {
  for (AccFuncMapType::const_iterator I = AccFuncMap.begin(),
    E = AccFuncMap.end(); I != E; ++I) {
      if (!R.contains(I->first))
        continue;

      OS << "BB: " << I->first->getName() << "{\n";
      for (AccFuncSetType::const_iterator FI = I->second.begin(),
          FE = I->second.end(); FI != FE; ++FI) {
        FI->print(OS,SE);
        OS << "\n";
      }
      OS << "}\n";
  }
}

void TempSCoP::printBounds(raw_ostream &OS, ScalarEvolution *SE) const {
  if (LoopBounds.empty()) {
    OS << "No good loops found!\n";
    return;
  }
  for (BoundMapType::const_iterator I = LoopBounds.begin(),
      E = LoopBounds.end(); I != E; ++I){
    if (!R.contains(I->first->getHeader()))
      continue;

    OS << "Bounds of Loop: " << I->first->getHeader()->getName()
      << ":\t{ ";
    I->second.first.print(OS,SE);
    OS << ", ";
    I->second.second.print(OS,SE);
    OS << "}\n";
  }
}

//===----------------------------------------------------------------------===//
// SCoPDetection Implement

// TODO: Move to LoopInfo?
static bool isPreHeader(BasicBlock *BB, LoopInfo *LI) {
  TerminatorInst *TI = BB->getTerminator();
  return (TI->getNumSuccessors() == 1)
    && LI->isLoopHeader(TI->getSuccessor(0));
}

bool SCoPDetection::isValidCFG(BasicBlock &BB, TempSCoP &SCoP) {
  Region &R = SCoP.getMaxRegion();
  TerminatorInst *TI = BB.getTerminator();

  unsigned int numSucc = TI->getNumSuccessors();

  // Return and unconditional branch is ok.
  if (numSucc < 2) return true;

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
          STATBAD(CFG);
          return false;
        }

        return true;
      }
    }

    // Now bb is not branching to inner loop.
    // FIXME: Handle the branch condition
    DEBUG(dbgs() << "Bad BB in cfg: " << BB.getName() << "\n");
    STATBAD(CFG);
    return false;
  }

  // BB is not in any loop.
  return true;
  // TODO: handle the branch condition
}

static bool isPureIntrinsic(unsigned ID) {
  switch (ID) {
  default:
    return false;
  case Intrinsic::sqrt:
  case Intrinsic::powi:
  case Intrinsic::sin:
  case Intrinsic::cos:
  case Intrinsic::pow:
  case Intrinsic::log:
  case Intrinsic::log10:
  case Intrinsic::log2:
  case Intrinsic::exp:
  case Intrinsic::exp2:
    return true;
  }
}

bool SCoPDetection::isValidCallInst(CallInst &CI, TempSCoP &SCoP) {
  if (CI.mayThrow() || CI.doesNotReturn())
    return false;

  if (CI.doesNotAccessMemory())
    return true;

  // Unfortunately some memory access information are true for intrinisic
  // function, e.g. line 239 in Intrinsics.td
  unsigned IntrID = CI.getCalledFunction()->getIntrinsicID();

  if (IntrID != Intrinsic::not_intrinsic)
    return isPureIntrinsic(IntrID);

  // XXX: Get the alias analysis result?
  return false;
}

bool SCoPDetection::isValidMemoryAccess(Instruction &Inst, TempSCoP &SCoP) {
  assert((isa<LoadInst>(&Inst) || isa<StoreInst>(&Inst))
    && "What else instruction access memory?");

  // Try to handle the load/store.
  Value *Pointer = 0;
  SCEVAffFunc::AccessType AccType = SCEVAffFunc::Read;

  if (LoadInst *load = dyn_cast<LoadInst>(&Inst)) {
    Pointer = load->getPointerOperand();
    DEBUG(dbgs() << "Read Addr " << *SE->getSCEV(Pointer) << "\n");
  } else if (StoreInst *store = dyn_cast<StoreInst>(&Inst)) {
    Pointer = store->getPointerOperand();
    DEBUG(dbgs() << "Write Addr " << *SE->getSCEV(Pointer) << "\n");
    AccType = SCEVAffFunc::Write;
  }

  // Can we get the pointer?
  if (!Pointer) {
    DEBUG(dbgs() << "Bad Inst accessing memory!\n");
    STATBAD(Other);
    return false;
  }

  SCEVAffFunc *func = 0;

  if (!checkSCoPOnly) {
    // Get the function set
    AccFuncSetType &AccFuncSet = AccFuncMap[Inst.getParent()];
    // Make the access function.
    AccFuncSet.push_back(SCEVAffFunc(AccType));
    func = &AccFuncSet.back();
  }

  // Is the access function affine?
  const SCEV *Addr = SE->getSCEV(Pointer);

  if (!SCEVAffFunc::buildAffineFunc(Addr, SCoP, func, *LI, *SE, AccType)) {
    DEBUG(dbgs() << "Bad memory addr " << *Addr << "\n");
    STATBAD(AffFunc);
    return false;
  }

  return true;
}

bool SCoPDetection::isValidInstruction(Instruction &Inst, TempSCoP &SCoP) {
  // We only check the call instruction but not invoke instruction.
  if (CallInst *CI = dyn_cast<CallInst>(&Inst)) {
    if (isValidCallInst(*CI, SCoP))
      return true;

    DEBUG(dbgs() << "Bad call Inst!\n");
    STATBAD(FuncCall);
    return false;
  }

  if (!Inst.mayWriteToMemory() && !Inst.mayReadFromMemory()) {
    // Handle cast instruction
    if (isa<IntToPtrInst>(Inst) || isa<BitCastInst>(Inst)) {
      DEBUG(dbgs() << "Bad cast Inst!\n");
      STATBAD(Other);
      return false;
    }

    return true;
  }

  if (isa<LoadInst>(&Inst) || isa<StoreInst>(&Inst))
    return isValidMemoryAccess(Inst, SCoP);

  // We do not know this instruction, therefore we assume it is invalid.
  STATBAD(Other);
  return false;
}

bool SCoPDetection::isValidBasicBlock(BasicBlock &BB, TempSCoP &SCoP) {
  // Check all instructions, except the terminator instruction.
  for (BasicBlock::iterator I = BB.begin(), E = --BB.end(); I != E; ++I)
    if (!isValidInstruction(*I, SCoP))
      return false;

  return true;
}

bool SCoPDetection::hasValidLoopBounds(TempSCoP &SCoP) {
  Region &R = SCoP.getMaxRegion();

  // Find the parameters used in loop bounds
  if (Loop *L = castToLoop(R, *LI)) {

    DEBUG(dbgs() << "Region : " << R.getNameStr()
      << " also is a loop: " <<(L ? L->getHeader()->getName() : "Not a loop")
      << "\n");

    // We can only handle loops whose induction variables in are in canonical
    // form.
    if (L->getCanonicalInductionVariable() == 0) {
      DEBUG(dbgs() << "No CanIV for loop : " << L->getHeader()->getName() <<"?\n");
      STATBAD(IndVar);
      return false;
    }

    ++SCoP.MaxLoopDepth;

    const SCEV *LoopCount = SE->getBackedgeTakenCount(L);

    DEBUG(dbgs() << "Backedge taken count: "<< *LoopCount <<"\n");

    // We can not handle the loop if its loop bounds can not be computed.
    if (isa<SCEVCouldNotCompute>(LoopCount)) {
      STATBAD(LoopBound);
      return false;
    }

    // The AffineSCEVIterator will always return the induction variable
    // which start from 0, and step by 1.
    const SCEV *LB = SE->getIntegerSCEV(0, LoopCount->getType()),
      *UB = LoopCount;

    SCEVAffFunc *lb = 0,
                *ub = 0;

    if (!checkSCoPOnly) {
      AffBoundType &affbounds = LoopBounds[L];
      lb = &affbounds.first;
      ub = &affbounds.second;
    }

    // Build the lower bound.
    if (!SCEVAffFunc::buildAffineFunc(LB, SCoP, lb, *LI, *SE)
        || !SCEVAffFunc::buildAffineFunc(UB, SCoP, ub, *LI, *SE)) {
      STATBAD(AffFunc);
      return false;
    }
  }

  return true;
}

bool SCoPDetection::mergeSubSCoP(TempSCoP &Parent, TempSCoP &SubSCoP){
  Loop *L = castToLoop(Parent.R, *LI);

  // Merge the parameters.
  for (ParamSetType::iterator I = SubSCoP.Params.begin(),
      E = SubSCoP.Params.end(); I != E; ++I) {
    const SCEV *Param = *I;
    // The valid parameter in subregion may not valid in its parameter
    if (isParameter(Param, Parent.getMaxRegion(), *LI, *SE)) {
      // Param is a valid parameter in Parent, too.
      Parent.Params.insert(Param);
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
      << " in " << Parent.getMaxRegion().getNameStr() << " at "
      << (L?L->getHeader()->getName():"Top Level")
      << "\n");
    // Bad parameter occur in expression
    STATBAD(AffFunc);
    return false;
  }

  // Update the loop depth.
  if (SubSCoP.MaxLoopDepth > Parent.MaxLoopDepth)
    Parent.MaxLoopDepth = SubSCoP.MaxLoopDepth;

  return true;
}

TempSCoP *SCoPDetection::getTempSCoP(Region& R) {

  if (!checkSCoPOnly) {
    // Did we already compute the SCoP for R?
    TempSCoPMapType::const_iterator at = RegionToSCoPs.find(&R);
    if (at != RegionToSCoPs.end() && at->second != 0)
      return at->second;
  }

  TempSCoP *SCoP = new TempSCoP(R, LoopBounds, AccFuncMap);

  bool isValidRegion = true;

  // Check if getScopeLoop work on the current loop nest and region tree,
  // if it not work, we could not handle any further
  if (getScopeLoop(R, *LI) != LI->getLoopFor(R.getEntry())) {
    STATBAD(LoopNest);
    isValidRegion = false;
  }

  // Visit all sub region node.
  for (Region::element_iterator I = R.element_begin(), E = R.element_end();
      I != E; ++I){
    if (I->isSubRegion()) {
      Region *SubR = I->getNodeAs<Region>();
      // Dirty hack for calculate temp scop on the fly
      // Do not extract the information for a hidden region.
      if (isHidden(SubR))
        continue;
      // Extract information of sub scop and merge them.
      else if (TempSCoP *SubSCoP = getTempSCoP(*SubR)) {
        isValidRegion &= mergeSubSCoP(*SCoP, *SubSCoP);

        if (checkSCoPOnly) {
          // Do not do any thing if we only check the SCoP.
          delete SubSCoP;
        }

        continue;
      }
      isValidRegion = false;
    } else if (isValidRegion) {
      // We check the basic blocks only the region is valid.
      BasicBlock &BB = *(I->getNodeAs<BasicBlock>());
      if (!isValidCFG(BB, *SCoP) // Check CFG.
          || !isValidBasicBlock(BB, *SCoP)){ // Check all non terminator inst
        DEBUG(dbgs() << "Bad BB found:" << BB.getName() << "\n");
        // Clean up the access function map, so we get a clear dump.
        AccFuncMap.erase(&BB);
        isValidRegion = false;
      }
    }
  }

  if (isValidRegion && hasValidLoopBounds(*SCoP)) {
    // Insert the SCoP into the map, if all the above was successful
    // and we are not only checking SCoPs.
    RegionToSCoPs[&(SCoP->R)] = checkSCoPOnly ? 0 : SCoP;
    STATSCOP(++ValidRegion);
    return SCoP;
  }

  DEBUG(dbgs() << "Bad region found: " << R.getNameStr() << "!\n");
  delete SCoP;

  return 0;
}

TempSCoP *SCoPDetection::getTempSCoPFor(const Region* R) const {
  TempSCoPMapType::const_iterator at = RegionToSCoPs.find(R);
  if (at == RegionToSCoPs.end())
    return 0;

  // Recalculate the temporary SCoP info.
  TempSCoP *tempSCoP =
    const_cast<SCoPDetection*>(this)->getTempSCoP(*const_cast<Region*>(R));
  assert(tempSCoP && "R should be valid if it contains in the map!");

  // Update the map.
  const_cast<SCoPDetection*>(this)->RegionToSCoPs[R] = tempSCoP;
  return tempSCoP;
}

bool SCoPDetection::runOnFunction(llvm::Function &F) {
  SE = &getAnalysis<ScalarEvolution>();
  LI = &getAnalysis<LoopInfo>();
  RI = &getAnalysis<RegionInfo>();

  Region *TopRegion = RI->getTopLevelRegion();

  checkSCoPOnly = true;

  if (TempSCoP *tempSCoP = getTempSCoP(*TopRegion)) {
    RegionToSCoPs[TopRegion] = 0;
    delete tempSCoP;
  }

#if 0
  // Statistic
  STATSCOP(for (TempSCoPMapType::const_iterator I = RegionToSCoPs.begin(),
    E = RegionToSCoPs.end(); I != E; ++I){
      TempSCoP *tempSCoP = I->second;
      if(isMaxRegionInSCoP(i->first)) {
        ++ValidSCoP;
        if (tempSCoP->getMaxLoopDepth() > 0)
          ++RichSCoP;
      }
  });
#endif

  checkSCoPOnly = false;

  return false;
}

void SCoPDetection::clear() {
  HiddenRegions.clear();
  LoopBounds.clear();
  AccFuncMap.clear();

  while (!RegionToSCoPs.empty()) {
    TempSCoPMapType::iterator I = RegionToSCoPs.begin();
    if (I->second)
      delete I->second;

    RegionToSCoPs.erase(I);
  }
}

/// Debug/Testing function

void SCoPDetection::print(raw_ostream &OS, const Module *) const {
  // Try to build the SCoPs again, this time is not check only.
  const_cast<SCoPDetection*>(this)->getTempSCoP(*(RI->getTopLevelRegion()));

  if (RegionToSCoPs.empty())
    OS << "No SCoP found!\n";
  else
    // Print all SCoPs.
    for (TempSCoPMapType::const_iterator I = RegionToSCoPs.begin(),
        E = RegionToSCoPs.end(); I != E; ++I){
      if(!PrintTopSCoPOnly || isMaxRegionInSCoP(*(I->first))) {
        I->second->print(OS, SE);
      }
    }

  OS << "\n";
}

SCoPDetection::~SCoPDetection() {
  clear();
}

char SCoPDetection::ID = 0;

static RegisterPass<SCoPDetection>
X("polly-scop-detect", "Polly - Detect SCoPs");

// Do not link this pass. This pass suppose to be only used by SCoPInfo.
