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

#include "llvm/Analysis/RegionIterator.h"
#include "llvm/Analysis/ScalarEvolutionExpressions.h"
#include "llvm/Assembly/Writer.h"
#include "llvm/Support/CFG.h"
#include "llvm/Support/CommandLine.h"

#define DEBUG_TYPE "polly-scop-detect"
#include "llvm/Support/Debug.h"


#include "isl_constraint.h"

using namespace llvm;
using namespace polly;

static cl::opt<bool>
PrintSubSCoPs("print-sub-temp-scop", cl::desc("Print out subSCoP."),
              cl::Hidden);

//===----------------------------------------------------------------------===//
// Temporary Hack for extended regiontree.
// Cast the region to loop if there is a loop have the same header and exit.
Loop *polly::castToLoop(const Region &R, LoopInfo &LI) {
  BasicBlock *entry = R.getEntry();

  if (!LI.isLoopHeader(entry))
    return 0;

  Loop *L = LI.getLoopFor(entry);

  BasicBlock *exit = L->getExitBlock();

  // Is the loop with multiple exits?
  // assert(exit && "Oop! we need the extended region tree :P");
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
    // Some SCEVUnknown will depend on loop variant:
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
                                  SCEVAffFunc &FuncToBuild,
                                  LoopInfo &LI, ScalarEvolution &SE) {
  assert(S && "S can not be null!");
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
      FuncToBuild.TransComp = I->second;
      continue;
    }

    // Ignore the pointer.
    if (Var->getType()->isPointerTy()) {
      DEBUG(dbgs() << "Find pointer: " << *Var <<"\n");
      assert(I->second->isOne() && "The coefficient of pointer expect is one!");
      const SCEVUnknown *BaseAddr = dyn_cast<SCEVUnknown>(Var);

      if (!BaseAddr) return false;
      // Setup the base address
      FuncToBuild.BaseAddr = BaseAddr->getValue();
      continue;
    }

    // Dirty Hack: Some pre-processing for parameter.
    if (const SCEVCastExpr *Cast = dyn_cast<SCEVCastExpr>(Var)) {
      DEBUG(dbgs() << "Warning: Ignore cast expression: " << *Cast << "\n");
      // Dirty hack. replace the cast expression by its operand.
      Var = Cast->getOperand();
    }

    // Build the affine function.
    FuncToBuild.LnrTrans.insert(*I);

    // Check if the parameter valid.
    if (!isParameter(Var, R, LI, SE)) {
      // If Var not a parameter, it may be the indvar of current loop
      if (const SCEVAddRecExpr *AddRec = dyn_cast<SCEVAddRecExpr>(Var)){
        // Is the loop with multiple exit that miss by
        // current "extend region tree"?
        if (!AddRec->getLoop()->getExitBlock())
          return false;

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

  return true;
}

static void setCoefficient(const SCEV *Coeff, mpz_t v, bool isLower) {
  if (Coeff) { // If the coefficient exist
    const SCEVConstant *C = dyn_cast<SCEVConstant>(Coeff);
    const APInt &CI = C->getValue()->getValue();
    // Convert i >= expr to i - expr >= 0
    MPZ_from_APInt(v, isLower?(-CI):CI);
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

   // Set the value of indvar of inner loops?

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

void SCEVAffFunc::print(raw_ostream &OS, ScalarEvolution *SE) const {
  // Print BaseAddr
  if (BaseAddr) {
    WriteAsOperand(OS, BaseAddr, false);
    OS << "[";
  }

  for (LnrTransSet::const_iterator I = LnrTrans.begin(), E = LnrTrans.end();
    I != E; ++I)
    OS << *I->second << " * " << *I->first << " + ";

  if (TransComp)
    OS << *TransComp;

  if (BaseAddr)
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

  printBounds(OS, SE);

  printAccFunc(OS, SE);
}

void TempSCoP::printAccFunc(llvm::raw_ostream &OS, ScalarEvolution *SE) const {
  for (AccFuncMapType::const_iterator I = AccFuncMap.begin(),
    E = AccFuncMap.end(); I != E; ++I) {
      OS << "BB: " << I->first->getName() << "{\n";
      for (AccFuncSetType::const_iterator FI = I->second.begin(),
        FE = I->second.end(); FI != FE; ++FI) {
          OS << (FI->second?"Writes ":"Reads ");
          FI->first.print(OS,SE);
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
  return (TI->getNumSuccessors() == 1) &&
    LI->isLoopHeader(TI->getSuccessor(0));
}

bool SCoPDetection::checkCFG(BasicBlock &BB, Region &R) {
  TerminatorInst *TI = BB.getTerminator();

  unsigned int numSucc = TI->getNumSuccessors();

  // Ret and unconditional branch is ok.
  if (numSucc < 2) return true;

  if (Loop *L = getScopeLoop(R, *LI)) {
    // Only allow branches that are loop exits. This stops anything
    // except loops that have just one exit and are detected by our LoopInfo
    // analysis
    // It is ok if BB is branching out of the loop
    if (L->isLoopExiting(&BB))
      return true;

    assert(L->getExitingBlock() &&
      "The Loop return from getScopeLoop will always have 1 exit!");

    // Now bb is not the exit block of the loop.
    // Is BB branching to inner loop?
    for (unsigned int i = 0; i < numSucc; ++i) {
      BasicBlock *SuccBB = TI->getSuccessor(i);
      if (isPreHeader(SuccBB, LI) || LI->isLoopHeader(SuccBB)) {
        // If branching to inner loop
        // FIXME: We can only handle a bb branching to preheader or not.
        if (numSucc > 2)
          return false;

        return true;
      }
    }

    // Now bb is not branching to inner loop.
    // FIXME: Handle the branch condition
    DEBUG(dbgs() << "Bad BB in cfg: " << BB.getName() << "\n");
    return false;
  }

  // BB is not in any loop.
  return true;
  // TODO: handle the branch condition
}

bool SCoPDetection::checkBasicBlock(BasicBlock &BB, TempSCoP &SCoP) {
  // Iterate over the BB to check its instructions, dont visit terminator
  for (BasicBlock::iterator I = BB.begin(), E = --BB.end(); I != E; ++I){
    Instruction &Inst = *I;

    DEBUG(dbgs() << Inst <<"\n");
    // Function call is not allowed in SCoP at the moment.
    if (/*CallInst *CI = */dyn_cast<CallInst>(&Inst)) {
      // TODO: Handle CI
      DEBUG(dbgs() << "Bad call Inst!\n");
      return false;
    }

    if (!Inst.mayWriteToMemory() && !Inst.mayReadFromMemory()) {
      // Handle cast instruction
      if (isa<IntToPtrInst>(I) || isa<BitCastInst>(I)) {
        DEBUG(dbgs() << "Bad cast Inst!\n");
        return false;
      }

      continue;
    }

    // Try to handle the load/store.
    Value *Pointer = 0;
    bool isStore = false;

    if (LoadInst *load = dyn_cast<LoadInst>(&Inst)) {
      Pointer = load->getPointerOperand();
      DEBUG(dbgs() << "Read Addr " << *SE->getSCEV(Pointer) << "\n");
    }
    else if (StoreInst *store = dyn_cast<StoreInst>(&Inst)) {
      Pointer = store->getPointerOperand();
      DEBUG(dbgs() << "Write Addr " << *SE->getSCEV(Pointer) << "\n");
      isStore = true;
    }

    // Can we get the pointer?
    if (!Pointer) {
      DEBUG(dbgs() << "Bad Inst accessing memory!\n");
      return false;
    }

    // Get the function set
    AccFuncSetType &AccFuncSet = AccFuncMap[&BB];
    // Make the access function.
    AccFuncSet.push_back(std::make_pair(SCEVAffFunc(), isStore));

    // Is the access function affine?
    const SCEV *Addr = SE->getSCEV(Pointer);
    if (!SCEVAffFunc::buildAffineFunc(Addr, SCoP, AccFuncSet.back().first, *LI, *SE)) {
      DEBUG(dbgs() << "Bad memory addr " << *Addr << "\n");
      return false;
    }
  }
  // All instruction is ok.
  return true;
}

bool SCoPDetection::mergeSubSCoPs(TempSCoP &Parent, TempSCoPSetType &SubSCoPs){
  Loop *L = castToLoop(Parent.R, *LI);
  while (!SubSCoPs.empty()) {
    TempSCoP *SubSCoP = SubSCoPs.back();
    // Merge the parameters.
    for (ParamSetType::iterator I = SubSCoP->Params.begin(),
        E = SubSCoP->Params.end(); I != E; ++I) {
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
      return false;
    }
    // Discard the merged scop.
    SubSCoPs.pop_back();
  }
  return true;
}

TempSCoP *SCoPDetection::getTempSCoP(Region& R) {
  bool isValidRegion = true;
  TempSCoPSetType SubSCoPs;

  TempSCoP *SCoP = new TempSCoP(R, LoopBounds, AccFuncMap);

  // Visit all sub region node.
  for (Region::element_iterator I = R.element_begin(), E = R.element_end();
    I != E; ++I){
      if (I->isSubRegion()) {
        Region *SubR = I->getNodeAs<Region>();
        // Dirty hack for calculate temp scop on the fly
        // Dont extract the information for a hidden region.
        if (isHidden(SubR))
          continue;
        //
        if (TempSCoP *SubSCoP = getTempSCoP(*SubR)) {
          SubSCoPs.push_back(SubSCoP);
          // Update the loop depth.
          if (SubSCoP->MaxLoopDepth > SCoP->MaxLoopDepth)
            SCoP->MaxLoopDepth = SubSCoP->MaxLoopDepth;
        }
        else
          isValidRegion = false;
      }
      else if (isValidRegion) {
        // We check the basic blocks only the region is valid.
        BasicBlock &BB = *(I->getNodeAs<BasicBlock>());
        if (!checkCFG(BB, R) || // Check cfg
          !checkBasicBlock(BB, *SCoP)) {// Check all non terminator instruction
            DEBUG(dbgs() << "Bad BB found:" << BB.getName() << "\n");
            // Clean up the access function map, so we get a clear dump.
            AccFuncMap.erase(&BB);
            isValidRegion = false;
        }
      }
  }

  // Find the parameters used in loop bounds
  Loop *L = castToLoop(R, *LI);

  // We can only handle loops whose indvar in canonical form.
  if (L && L->getCanonicalInductionVariable() == 0) {
    DEBUG(dbgs() << "No CanIV for loop : " << L->getHeader()->getName() <<"?\n");
    isValidRegion = false;
  }

  if (L && isValidRegion) {
    // Increase the max loop depth
    ++SCoP->MaxLoopDepth;

    const SCEV *LoopCount = SE->getBackedgeTakenCount(L);

    DEBUG(dbgs() << "Backedge taken count: "<< *LoopCount <<"\n");

    if (!isa<SCEVCouldNotCompute>(LoopCount)) {
      // The AffineSCEVIterator will always return the induction variable
      // which start from 0, and step by 1.
      const SCEV *LB = SE->getIntegerSCEV(0, LoopCount->getType()),
                 *UB = LoopCount;

      AffBoundType &affbounds = LoopBounds[L];

      // Build the lower bound.
      if (!SCEVAffFunc::buildAffineFunc(LB, *SCoP, affbounds.first, *LI, *SE)||
        !SCEVAffFunc::buildAffineFunc(UB, *SCoP, affbounds.second, *LI, *SE))
        isValidRegion = false;
    }
    else // We can handle the loop if its loop bounds could not compute.
      isValidRegion = false;
  }

  // Merge the information from sub SCoPs.
  isValidRegion &= mergeSubSCoPs(*SCoP, SubSCoPs);

  if (!isValidRegion) {
    DEBUG(dbgs() << "Bad region found: " << R.getNameStr() << "!\n");
    // Erase the Loop bounds, so it looks better when we are dumping
    // the loop bounds
    if (L)
      LoopBounds.erase(L);
    // discard the SCoP
    delete SCoP;

    return 0;
  }

  // Insert the scop to the map.
  RegionToSCoPs.insert(std::make_pair(&(SCoP->R), SCoP));

  return SCoP;
}


bool SCoPDetection::runOnFunction(llvm::Function &F) {
  SE = &getAnalysis<ScalarEvolution>();
  LI = &getAnalysis<LoopInfo>();

  RI = &getAnalysis<RegionInfo>();

  Region *TopRegion = RI->getTopLevelRegion();

  if(TempSCoP *SCoP = getTempSCoP(*TopRegion))
    RegionToSCoPs.insert(std::make_pair(&(SCoP->R), SCoP));

  return false;
}

void SCoPDetection::clear() {
  HiddenRegions.clear();
  LoopBounds.clear();
  AccFuncMap.clear();

  while (!RegionToSCoPs.empty()) {
    TempSCoPMapType::iterator I = RegionToSCoPs.begin();
    delete I->second;
    RegionToSCoPs.erase(I);
  }
}

/// Debug/Testing function

void SCoPDetection::print(raw_ostream &OS, const Module *) const {
  if (RegionToSCoPs.empty())
    OS << "No SCoP found!\n";
  else
    // Print all SCoPs.
    for (TempSCoPMapType::const_iterator I = RegionToSCoPs.begin(),
      E = RegionToSCoPs.end(); I != E; ++I){
        if(isMaxRegionInSCoP(I->second->getMaxRegion()) || PrintSubSCoPs)
          I->second->print(OS, SE);
    }

  OS << "\n";
}

SCoPDetection::~SCoPDetection() {
  clear();
}

char SCoPDetection::ID = 0;

static RegisterPass<SCoPDetection>
X("polly-scop-detect", "Polly - Detect SCoPs");
