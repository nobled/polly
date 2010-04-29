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

#include "llvm/Analysis/AffineSCEVIterator.h"
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
// SCEVAffFunc Implement
void SCEVAffFunc::print(raw_ostream &OS, ScalarEvolution *SE) const {
  // Print BaseAddr
  if (BaseAddr) {
    WriteAsOperand(OS, BaseAddr, false);
    OS << "[";
  }

  for (LnrTransSet::const_iterator I = LnrTrans.begin(), E = LnrTrans.end();
    I != E; ++I) {
      // Print the coefficient (constant part)
      I->second->print(OS);

      OS << " * ";

      // Print the variable
      const SCEV* S = I->first;

      if (const SCEVUnknown *U = dyn_cast<SCEVUnknown>(S))
        WriteAsOperand(OS, U->getValue(), false);
      else if(const SCEVAddRecExpr *AddRec = dyn_cast<SCEVAddRecExpr>(S)) {
        PHINode *V = AddRec->getLoop()->getCanonicalInductionVariable();
        // Get the induction variable of the loop.
        const SCEV *IndVar = SE->getSCEV(V);

        if (IndVar == AddRec)
          // Print out the llvm value if AddRec is the induction variable.
          WriteAsOperand(OS, V, false);
        else
          S->print(OS);
      }
      else
        S->print(OS);

      OS << " + ";
  }

  if (TransComp)
    OS << *TransComp;

  if (BaseAddr)
    OS << "]";

}

bool SCEVAffFunc::buildAffineFunc(const SCEV *S, TempSCoP &SCoP,
                                  SCEVAffFunc &FuncToBuild,
                                  LoopInfo &LI, ScalarEvolution &SE) {
  Region &R = SCoP.getMaxRegion();

  if (isa<SCEVCouldNotCompute>(S))
    return false;

  Loop *Scope = getScopeLoop(R, LI);

  // Compute S at scope first.
  //if (ScopeL)
  S = SE.getSCEVAtScope(S, Scope);

  // FIXME: Simplify these code.
  for (AffineSCEVIterator I = affine_begin(S, &SE), E = affine_end();
    I != E; ++I){
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
        //assert(BaseAddr && "Can only handle Unknown Addr!");
        if (!BaseAddr) return false;
        // Setup the base address
        FuncToBuild.BaseAddr = BaseAddr->getValue();
        continue;
      }

      // Build the affine function.
      FuncToBuild.LnrTrans.insert(*I);

      if (!Var->isLoopInvariant(Scope)) {
        // If var the induction variable of the loop?
        if (const SCEVAddRecExpr *AddRec = dyn_cast<SCEVAddRecExpr>(Var)){
          // Is the loop with multiple exit?
          //assert(AddRec->getLoop()->getExitBlock() &&
          //  "Oop! we need the extended region tree :P");
          if (!AddRec->getLoop()->getExitBlock()) return false;

          assert(AddRec->getLoop() == Scope && "getAtScope not work?");
          continue;
        }
        // A bad SCEV found.
        DEBUG(dbgs() << "Bad SCEV: " << *Var << " in " << *S << " at "
          << (Scope?Scope->getHeader()->getName():"Top Level")
          << "\n");
        return false;
      }

      if (const SCEVAddRecExpr *AddRec = dyn_cast<SCEVAddRecExpr>(Var)) {
        // The indvar only expect come from outer loop
        // Or from a loop whose backend taken count could not compute.
        assert((AddRec->getLoop()->contains(Scope) ||
                isa<SCEVCouldNotCompute>(
                  SE.getBackedgeTakenCount(AddRec->getLoop()))) &&
                "Where comes the indvar?");
      }
      else if (const SCEVCastExpr *Cast = dyn_cast<SCEVCastExpr>(Var)) {
        DEBUG(dbgs() << "Warning: Ignore cast expression: " << *Cast << "\n");
        // Dirty hack. replace the cast expression by its operand.
        FuncToBuild.LnrTrans.erase(FuncToBuild.LnrTrans.find(Var));
        Var = Cast->getOperand();
        FuncToBuild.LnrTrans.insert(std::make_pair(Var, I->first));
      }
      // TODO: handle the paramer0 * parameter1 case.
      // A dirty hack
      else if (!isa<SCEVUnknown>(Var)){
        DEBUG(dbgs() << "Bad SCEV: "<<*Var << " in " << *S <<"\n");
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
      if (isPreHeader(SuccBB, LI)) {
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

void SCoPDetection::mergeSubSCoPs(TempSCoP &Parent, TempSCoPSetType &SubSCoPs){
 while (!SubSCoPs.empty()) {
   TempSCoP *SubSCoP = SubSCoPs.back();
   // The scop is not maximum any more.
   SubSCoP->isMax = false;
   // Merge the parameters.
   Parent.Params.insert(SubSCoP->Params.begin(),
     SubSCoP->Params.end());
   // Discard the merged scop.
   SubSCoPs.pop_back();
 }
 // The induction variable is not parameter at this scope.
 if (Loop *L = castToLoop(Parent.R, *LI))
   Parent.Params.erase(SE->getSCEV(L->getCanonicalInductionVariable()));
}

TempSCoP *SCoPDetection::isValidSCoP(Region& R) {
  bool isValidRegion = true;
  TempSCoPSetType SubSCoPs;

  TempSCoP *SCoP = new TempSCoP(R, LoopBounds, AccFuncMap);

  // Visit all sub region node.
  for (Region::element_iterator I = R.element_begin(), E = R.element_end();
    I != E; ++I){

      if (I->isSubRegion()) {
        Region *SubR = I->getNodeAs<Region>();
        if (TempSCoP *SubSCoP = isValidSCoP(*SubR)) {
          SubSCoPs.push_back(SubSCoP);
          // Update the loop depth.
          if (SubSCoP->MaxLoopDepth > SCoP->MaxLoopDepth)
            SCoP->MaxLoopDepth = SubSCoP->MaxLoopDepth;
        }
        else
          isValidRegion = false;
      }
      else if (isValidRegion) {
        BasicBlock &BB = *(I->getNodeAs<BasicBlock>());
        // We check the basic blocks only the region is valid.
        if (!checkCFG(BB, R) || // Check cfg
          !checkBasicBlock(BB, *SCoP)) { // Check all non terminator instruction
            DEBUG(dbgs() << "Bad BB found:" << BB.getName() << "\n");
            // Clean up the access function map, so we get a clear dump.
            AccFuncMap.erase(&BB);
            isValidRegion = false;
        }
      }
  }

  // Find the parameters used in loop bounds
  Loop *L = castToLoop(R, *LI);
  if (L) {
    // Increase the max loop depth
    ++SCoP->MaxLoopDepth;

    const SCEV *LoopCount = SE->getBackedgeTakenCount(L);

    DEBUG(dbgs() << "Backedge taken count: "<< *LoopCount <<"\n");

    if (!isa<SCEVCouldNotCompute>(LoopCount)) {

      Value* V = L->getCanonicalInductionVariable();

      assert(V && "IndVarSimplify pass not work!");

      const SCEVAddRecExpr *IndVar =
        dyn_cast<SCEVAddRecExpr>(SE->getSCEV(V));

      assert(IndVar && "Expect IndVar a SECVAddRecExpr");

      // The loop will aways start with 0?
      const SCEV *LB = IndVar->getStart();

      const SCEV *UB = SE->getAddExpr(LB, LoopCount);

      AffBoundType &affbounds = LoopBounds[L];

      // Build the lower bound.
      if (!SCEVAffFunc::buildAffineFunc(LB, *SCoP, affbounds.first, *LI, *SE)||
        !SCEVAffFunc::buildAffineFunc(UB, *SCoP, affbounds.second, *LI, *SE))
        isValidRegion = false;
    }
    else // We can handle the loop if its loop bounds could not compute.
      isValidRegion = false;
  }

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

  mergeSubSCoPs(*SCoP, SubSCoPs);

  // Insert the scop to the map.
  RegionToSCoPs.insert(std::make_pair(&(SCoP->R), SCoP));

  return SCoP;
}


bool SCoPDetection::runOnFunction(llvm::Function &F) {
  SE = &getAnalysis<ScalarEvolution>();
  LI = &getAnalysis<LoopInfo>();

  RI = &getAnalysis<RegionInfo>();

  Region *TopRegion = RI->getTopLevelRegion();

  // found SCoPs.
  TempSCoPSetType TempSCoPs;

  if(TempSCoP *SCoP = isValidSCoP(*TopRegion))
    RegionToSCoPs.insert(std::make_pair(&(SCoP->R), SCoP));

  return false;
}

void SCoPDetection::clear() {
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
        if(I->second->isMaxSCoP() || PrintSubSCoPs)
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
