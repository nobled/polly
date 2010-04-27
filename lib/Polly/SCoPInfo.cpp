//===--------- SCoPInfo.cpp  - Create SCoPs from LLVM IR --------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Create a polyhedral description of the region.
//
//===----------------------------------------------------------------------===//

#include "polly/SCoPInfo.h"

#include "polly/Support/GmpConv.h"

#include "llvm/Analysis/AffineSCEVIterator.h"
#include "llvm/Analysis/RegionIterator.h"
#include "llvm/Analysis/ScalarEvolutionExpressions.h"
#include "llvm/Assembly/Writer.h"
#include "llvm/Support/CFG.h"
#include "llvm/Support/CommandLine.h"

#define DEBUG_TYPE "polly-scop-info"
#include "llvm/Support/Debug.h"

#include "isl_constraint.h"

using namespace llvm;
using namespace polly;


static cl::opt<bool>
PrintAccessFunctions("print-access-functions", cl::Hidden,
               cl::desc("Print the access functions of BBs."));

static cl::opt<bool>
PrintLoopBound("print-loop-bounds", cl::Hidden,
            cl::desc("Print the bounds of loops."));

//===----------------------------------------------------------------------===//
SCoPStmt::SCoPStmt(SCoP &parent, BasicBlock &bb,
                   polly_set *domain, polly_map *scat)
  : Parent(parent), BB(bb), Domain(domain), Scattering(scat) {
    assert(Domain && Scattering && "Domain and Scattering can not be null!");
}

SCoPStmt::~SCoPStmt() {
  isl_set_free(Domain);
  isl_map_free(Scattering);
}

//===----------------------------------------------------------------------===//
/// SCoP class implement
template<class It>
SCoP::SCoP(Region &r, unsigned maxLoopDepth, It ParamBegin, It ParamEnd)
           : R(r), MaxLoopDepth(maxLoopDepth) {
   // Create the context
   ctx = isl_ctx_alloc();

   // Initialize parameters
   for (It I = ParamBegin, E = ParamEnd; I != E ; ++I)
     Parameters.push_back(*I);

   // Create the dim with 0 parameters
   polly_dim *dim = isl_dim_set_alloc(ctx, 0, getNumParams());
   // TODO: Handle the constrain of parameters.
   // Take the dim.
   Context = isl_set_universe (dim);
}

SCoP::~SCoP() {
  // Free the context
  isl_set_free(Context);
  // Free the statements;
  for (StmtSet::iterator I = Stmts.begin(), E = Stmts.end(); I != E; ++I)
    delete *I;
  // We need a singleton to manage this?
  //isl_ctx_free(ctx);
}

void SCoP::print(raw_ostream &OS) const {
  OS << "SCoP: " << R.getNameStr() << "\tParameters: (";
  // Print Parameters.
  for (const_param_iterator PI = param_begin(), PE = param_end();
      PI != PE; ++PI)
    OS << **PI << ", ";

  OS << "), Max Loop Depth: "<< MaxLoopDepth <<"\n";


}

//===----------------------------------------------------------------------===//

bool SCoPInfo::buildAffineFunc(const SCEV *S, LLVMSCoP *SCoP,
                            SCEVAffFunc &FuncToBuild) {
  assert(SCoP && "Scope can not be null!");

  Region *R = &SCoP->R;

  if (isa<SCEVCouldNotCompute>(S))
    return false;

  Loop *Scope = getScopeLoop(R);

  // Compute S at scope first.
  //if (ScopeL)
  S = SE->getSCEVAtScope(S, Scope);

  // FIXME: Simplify these code.
  for (AffineSCEVIterator I = affine_begin(S, SE), E = affine_end();
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
      assert(I->second->isOne() && "The coefficient of pointer expect is one!");
      // Setup the base address
      FuncToBuild.BaseAddr = cast<SCEVUnknown>(Var)->getValue();
      continue;
    }


    // Build the affine function.
    FuncToBuild.LnrTrans.insert(*I);

    if (!Var->isLoopInvariant(Scope)) {
      // If var the induction variable of the loop?
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

    if (const SCEVAddRecExpr *AddRec = dyn_cast<SCEVAddRecExpr>(Var))
      assert(AddRec->getLoop()->contains(Scope) && "Where comes the indvar?");
    else if (const SCEVCastExpr *Cast = dyn_cast<SCEVCastExpr>(Var)) {
      DEBUG(dbgs() << "Warning: Ignore cast expression: " << *Cast << "\n");
      // Dirty hack. replace the cast expression by its operand.
      FuncToBuild.LnrTrans.erase(FuncToBuild.LnrTrans.find(Var));
      Var = Cast->getOperand();
      FuncToBuild.LnrTrans.insert(std::make_pair(Var, I->first));
    }
    else {
      // TODO: handle the paramer0 * parameter1 case.
      // A dirty hack
      DEBUG(dbgs() << *Var << "\n");
      assert(isa<SCEVUnknown>(Var) && "Can only process unknown right now.");
    }
    // Add the loop invariants to parameter lists.
    SCoP->Params.insert(Var);
  }

  return true;
}

// TODO: Move to LoopInfo?
static bool isPreHeader(BasicBlock *BB, LoopInfo *LI) {
  TerminatorInst *TI = BB->getTerminator();
  return (TI->getNumSuccessors() == 1) &&
         LI->isLoopHeader(TI->getSuccessor(0));
}

bool SCoPInfo::checkCFG(BasicBlock *BB, Region *R) {
  TerminatorInst *TI = BB->getTerminator();

  unsigned int numSucc = TI->getNumSuccessors();

  // Ret and unconditional branch is ok.
  if (numSucc < 2) return true;

  if (Loop *L = getScopeLoop(R)) {
    // Only allow branches that are loop exits. This stops anything
    // except loops that have just one exit and are detected by our LoopInfo
    // analysis

    // It is ok if BB is branching out of the loop
    if (L->isLoopExiting(BB))
      return true;

    assert(L->getExitingBlock() &&
      "Only can handle Loop with 1 exiting block now!");

    // Now bb is not the exit block of the loop.
    // Is BB branching to inner loop?
    for (unsigned int i = 0; i < numSucc; ++i) {
      BasicBlock *SuccBB = TI->getSuccessor(i);
      if (isPreHeader(SuccBB, LI)) {
        // If branching to inner loop
        assert(numSucc == 2 && "Not a natural loop?");
        return true;
      }
    }

    // Now bb is not branching to inner loop.
    // FIXME: Handle the branch condition
    DEBUG(dbgs() << "Bad BB in cfg: " << BB->getName() << "\n");
    return false;
  }

  // BB is not in any loop.
  return true;
  // TODO: handle the branch condition
}

bool SCoPInfo::checkBasicBlock(BasicBlock *BB, LLVMSCoP *SCoP) {
  // Iterate over the BB to check its instructions, dont visit terminator
  for (BasicBlock::iterator I = BB->begin(), E = --BB->end(); I != E; ++I){
    // Find the parameters used in load/store
    // TODO: Write the code.
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
    AccFuncSetType &AccFuncSet = AccFuncMap[BB];
    // Make the access function.
    AccFuncSet.push_back(std::make_pair(SCEVAffFunc(), isStore));

    // Is the access function affine?
    const SCEV *Addr = SE->getSCEV(Pointer);
    if (!buildAffineFunc(Addr, SCoP, AccFuncSet.back().first)) {
      DEBUG(dbgs() << "Bad memory addr " << *Addr << "\n");
      return false;
    }
  }
  // All instruction is ok.
  return true;
}

SCoPInfo::LLVMSCoP *SCoPInfo::findSCoPs(Region* R, TempSCoPSetType &SCoPs) {
  bool isValidRegion = true;
  TempSCoPSetType SubSCoPs;

  LLVMSCoP *SCoP = new LLVMSCoP(*R);

  // Visit all sub region node.
  for (Region::element_iterator I = R->element_begin(), E = R->element_end();
      I != E; ++I){

    if (I->isSubRegion()) {
      Region *SubR = I->getNodeAs<Region>();
      if (LLVMSCoP *SubSCoP = findSCoPs(SubR, SCoPs)) {
        SubSCoPs.push_back(SubSCoP);
        // Update the loop depth.
        if (SubSCoP->MaxLoopDepth > SCoP->MaxLoopDepth)
          SCoP->MaxLoopDepth = SubSCoP->MaxLoopDepth;
      }
      else
        isValidRegion = false;
    }
    else if (isValidRegion) {
      BasicBlock *BB = I->getNodeAs<BasicBlock>();
      // We check the basic blocks only the region is valid.
      if (!checkCFG(BB, R) || // Check cfg
          !checkBasicBlock(BB, SCoP)) { // Check all non terminator instruction
        DEBUG(dbgs() << "Bad BB found:" << BB->getName() << "\n");
        // Clean up the access function map, so we get a clear dump.
        AccFuncMap.erase(BB);
        isValidRegion = false;
      }
    }
  }

  // Find the parameters used in loop bounds
  Loop *L = castToLoop(R);
  if (L) {
    // Increase the max loop depth
    ++SCoP->MaxLoopDepth;

    const SCEV *LoopCount = SE->getBackedgeTakenCount(L);

    DEBUG(dbgs() << "Backedge taken count: "<< *LoopCount <<"\n");

    if (!isa<SCEVCouldNotCompute>(LoopCount)) {

      const SCEVAddRecExpr *IndVar =
        dyn_cast<SCEVAddRecExpr>(SE->getSCEV(L->getCanonicalInductionVariable()));

      assert(IndVar && "Expect IndVar a SECVAddRecExpr");

      // The loop will aways start with 0?
      const SCEV *LB = IndVar->getStart();

      const SCEV *UB = SE->getAddExpr(LB, LoopCount);

      AffBoundType &affbounds = LoopBounds[L];

      // Build the lower bound.
      if (!buildAffineFunc(LB, SCoP, affbounds.first) ||
          !buildAffineFunc(UB, SCoP, affbounds.second))
          isValidRegion = false;
    }
    else // We can handle the loop if its loop bounds could not compute.
      isValidRegion = false;
  }

  if (!isValidRegion) {
    // If this Region is not a part of SCoP,
    // add the sub scops to the SCoP vector.
    SCoPs.insert(SCoPs.begin(), SubSCoPs.begin(), SubSCoPs.end());
    // Erase the Loop bounds, so it looks better when we are dumping
    // the loop bounds
    if (L)
      LoopBounds.erase(L);
    // discard the SCoP
    delete SCoP;

    return 0;
  }

  mergeSubSCoPs(SCoP, SubSCoPs);

  return SCoP;
}

//===----------------------------------------------------------------------===//
/// Help function to build isl objects


static void setCoefficient(const SCEV *Coeff, mpz_t v, bool isLower) {
  if (Coeff) { // If the coefficient exist
    const SCEVConstant *C = dyn_cast<SCEVConstant>(Coeff);
    const APInt &CI = C->getValue()->getValue();
    DEBUG(errs() << "Setting Coeff: " << CI);
    DEBUG(errs() << " neg: " << -CI <<"\n" );
    // Convert i >= expr to i - expr >= 0
    MPZ_from_APInt(v, isLower?(-CI):CI);
  }
  else
    isl_int_set_si(v, 0);
}

static __isl_give
polly_constraint *toLoopBoundConstrain(__isl_keep polly_ctx *ctx,
                                       __isl_keep polly_dim *dim, SCEVAffFunc &func,
                                       const SmallVectorImpl<const SCEV*> &IndVars,
                                       const SmallVectorImpl<const SCEV*> &Params,
                                       bool isLower) {
  unsigned num_in = IndVars.size(),
           num_param = Params.size();

  polly_constraint *c = isl_inequality_alloc(isl_dim_copy(dim));
  isl_int v;
  isl_int_init(v);

  // Dont touch the current iterator.
  for (unsigned i = 0, e = num_in - 1; i != e; ++i) {
    setCoefficient(func.getCoeff(IndVars[i]), v, isLower);
    isl_constraint_set_coefficient(c, isl_dim_set, i, v);
  }

  assert(!func.getCoeff(IndVars[num_in - 1]) &&
          "Current iterator should not have any coff.");
  // Set the coefficient of current iterator, convert i <= expr to expr - i >= 0
  isl_int_set_si(v, isLower?1:(-1));
  isl_constraint_set_coefficient(c, isl_dim_set, num_in - 1, v);

  // Set the value of indvar of inner loops?

  // Setup the coefficient of parameters
  for (unsigned i = 0, e = num_param; i != e; ++i) {
    setCoefficient(func.getCoeff(Params[i]), v, isLower);
    isl_constraint_set_coefficient(c, isl_dim_param, i, v);
  }

  // Set the const.
  setCoefficient(func.TransComp, v, isLower);
  isl_constraint_set_constant(c, v);
  // Free v
  isl_int_clear(v);
  return c;
}

static __isl_give
polly_map *buildScattering(__isl_keep polly_ctx *ctx, unsigned ParamDim,
                           SmallVectorImpl<unsigned> &Scatter,
                           unsigned ScatDim, unsigned CurLoopDepth) {
  // FIXME: No parameter need?
  polly_dim *dim = isl_dim_alloc(ctx, 0/*ParamDim*/, CurLoopDepth, ScatDim);
  polly_basic_map *bmap = isl_basic_map_universe(isl_dim_copy(dim));
  isl_int v;
  isl_int_init(v);

  // Loop dimensions.
  for (unsigned i = 0; i < CurLoopDepth; ++i) {
    polly_constraint *c = isl_equality_alloc(isl_dim_copy(dim));
    isl_int_set_si(v, 1);
    isl_constraint_set_coefficient(c, isl_dim_out, 2 * i + 1, v);
    isl_int_set_si(v, -1);
    isl_constraint_set_coefficient(c, isl_dim_in, i, v);

    bmap = isl_basic_map_add_constraint(bmap, c);
  }

  // Constant dimensions
  for (unsigned i = 0; i < CurLoopDepth + 1; ++i) {
    polly_constraint *c = isl_equality_alloc(isl_dim_copy(dim));
    isl_int_set_si(v, -1);
    isl_constraint_set_coefficient(c, isl_dim_out, 2 * i, v);
    isl_int_set_si(v, Scatter[i]);
    isl_constraint_set_constant(c, v);

    bmap = isl_basic_map_add_constraint(bmap, c);
  }

  // Fill scattering dimensions.
  for (unsigned i = 2 * CurLoopDepth + 1; i < ScatDim ; ++i) {

    polly_constraint *c = isl_equality_alloc(isl_dim_copy(dim));
    isl_int_set_si(v, 1);
    isl_constraint_set_coefficient(c, isl_dim_out, i, v);
    isl_int_set_si(v, 0);
    isl_constraint_set_constant(c, v);

    bmap = isl_basic_map_add_constraint(bmap, c);
  }

  isl_int_clear(v);
  isl_dim_free(dim);
  return isl_map_from_basic_map(bmap);
}

__isl_give
polly_basic_set *SCoPInfo::buildIterateDomain(SCoP &SCoP,
                                    SmallVectorImpl<Loop*> &NestLoops){
  // Create the basic set;
  polly_dim *dim = isl_dim_set_alloc(SCoP.ctx, SCoP.getNumParams(),
    NestLoops.size());
  polly_basic_set *bset = isl_basic_set_universe(isl_dim_copy(dim));


  SmallVector<const SCEV*, 8> IndVars;

  for (int i = 0, e = NestLoops.size(); i != e; ++i) {
    Loop *L = NestLoops[i];
    Value *IndVar = L->getCanonicalInductionVariable();
    IndVars.push_back(SE->getSCEV(IndVar));

    BoundMapType::iterator at = LoopBounds.find(L);
    assert(at != LoopBounds.end() &&
      "Can not get loop bound when building statement!");

    // Build the constrain of lower bound
    polly_constraint *lb = toLoopBoundConstrain(SCoP.ctx, dim,
      at->second.first, IndVars, SCoP.getParams(), true);

    bset = isl_basic_set_add_constraint(bset, lb);

    polly_constraint *ub = toLoopBoundConstrain(SCoP.ctx, dim,
      at->second.second, IndVars, SCoP.getParams(), false);

    bset = isl_basic_set_add_constraint(bset, ub);
  }

  isl_dim_free(dim);
  return bset;
}

SCoPStmt *SCoPInfo::buildStmt(SCoP &SCoP, BasicBlock &BB,
                              SmallVectorImpl<Loop*> &NestLoops,
                              SmallVectorImpl<unsigned> &Scatter) {

  polly_basic_set *bset = buildIterateDomain(SCoP, NestLoops);

  polly_set *Domain = isl_set_from_basic_set(bset);

  DEBUG(std::cerr << "\n\nIterate domain of BB: " << BB.getNameStr() << " is:\n");
  DEBUG(isl_set_print(Domain, stderr, 20, ISL_FORMAT_ISL));
  DEBUG(std::cerr << std::endl);

  // Scattering function
  DEBUG(dbgs() << "For BB: " << BB.getName() << " ScatDim: " << Scatter.size()
               << " LoopDepth: " << NestLoops.size() << " { ");
  DEBUG(
    for (unsigned i = 0, e = Scatter.size(); i != e; ++i)
      dbgs() << Scatter[i] << ", ";
    dbgs() << "}\n";
    );
  polly_map *Scattering = buildScattering(SCoP.ctx, SCoP.getNumParams(),
                                          Scatter,
                                          SCoP.getScatterDim(),
                                          NestLoops.size());
  DEBUG(std::cerr << "\nScattering:\n");
  DEBUG(isl_map_print(Scattering, stderr, 20, ISL_FORMAT_ISL));
  DEBUG(std::cerr << std::endl);
  // Access function

  return new SCoPStmt(SCoP, BB, Domain, Scattering);
}

void SCoPInfo::buildSCoP(SCoP &SCoP, const Region &CurRegion,
                         SmallVectorImpl<Loop*> &NestLoops,
                         SmallVectorImpl<unsigned> &Scatter) {
  Loop *L = castToLoop(&CurRegion);

  if (L)
    NestLoops.push_back(L);

  // TODO: scattering function
  unsigned loopDepth = NestLoops.size();
  assert(Scatter.size() > loopDepth && "Scatter not big enough!");

  for (Region::const_element_iterator I = CurRegion.element_begin(),
      E = CurRegion.element_end(); I != E; ++I) {
    if (I->isSubRegion())
      buildSCoP(SCoP, *(I->getNodeAs<Region>()), NestLoops, Scatter);
    else {
      // Build the statement
      SCoP.Stmts.insert(buildStmt(SCoP, *(I->getNodeAs<BasicBlock>()),
                                  NestLoops, Scatter));
      // Increasing the Scattering function is ok for at the moment, because
      // we are using a depth iterator and the program is linear
      ++Scatter[loopDepth];
    }

  }


  if (L) {
    // Clear the scatter function when leaving the loop.
    Scatter[loopDepth] = 0;
    NestLoops.pop_back();
    // To next loop
    ++Scatter[loopDepth-1];
    // TODO: scattering function
  }

}

bool SCoPInfo::runOnFunction(llvm::Function &F) {
  SE = &getAnalysis<ScalarEvolution>();
  LI = &getAnalysis<LoopInfo>();

  RI = &getAnalysis<RegionInfo>();

  Region *TopRegion = RI->getTopLevelRegion();

  // found SCoPs.
  TempSCoPSetType TempSCoPs;

  if(LLVMSCoP *SCoP = findSCoPs(TopRegion, TempSCoPs))
    TempSCoPs.push_back(SCoP);

  // dump temp scops
  DEBUG(
    if (TempSCoPs.empty())
      dbgs() << "No SCoP found!\n";
    else
      // Print all SCoPs.
      for (TempSCoPSetType::const_iterator I = TempSCoPs.begin(), E = TempSCoPs.end();
          I != E; ++I)
        (*I)->print(dbgs());
  );

  SmallVector<Loop*, 8> NestLoops;
  SmallVector<unsigned, 8> Scatter;

  unsigned numSCoPFound = TempSCoPs.size();
  DEBUG(dbgs() << numSCoPFound << " SCoP found!\n");

  while (!TempSCoPs.empty()) {
    LLVMSCoP *TempSCoP = TempSCoPs.back();
    SCoP *scop = new SCoP(TempSCoP->R, TempSCoP->MaxLoopDepth,
                          TempSCoP->Params.begin(), TempSCoP->Params.end());
    // Add the scop to the map.
    RegionToSCoPs.insert(std::make_pair(&(scop->getRegion()), scop));

    unsigned numScatter = TempSCoP->MaxLoopDepth + 1;
    // Initialize the scattering function
    Scatter.assign(numScatter, 0);

    buildSCoP(*scop, scop->getRegion(), NestLoops, Scatter);

    assert(NestLoops.empty() && "NestLoops not empty at top level!");

    // Discard the temp scop.
    delete TempSCoP;
    TempSCoPs.pop_back();
  }

  assert(numSCoPFound == RegionToSCoPs.size() && "Where comes the scop?");

  return false;
}

void SCoPInfo::clear() {
  AccFuncMap.clear();
  LoopBounds.clear();

  for (SCoPMapType::iterator I = RegionToSCoPs.begin(),
    E = RegionToSCoPs.end(); I != E; ++I)
    delete I->second;

  RegionToSCoPs.clear();

}

/// Debug/Testing function

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

void SCoPInfo::printAccFunc(llvm::raw_ostream &OS) const {
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

void SCoPInfo::printBounds(raw_ostream &OS) const {
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

void SCoPInfo::LLVMSCoP::print(raw_ostream &OS) const {
  OS << "SCoP: " << R.getNameStr() << "\tParameters: (";
  // Print Parameters.
  for (ParamSetType::const_iterator PI = Params.begin(), PE = Params.end();
      PI != PE; ++PI)
    OS << **PI << ", ";

  OS << "), Max Loop Depth: "<< MaxLoopDepth <<"\n";
}

void SCoPInfo::print(raw_ostream &OS, const Module *) const {
   if (RegionToSCoPs.empty())
     OS << "No SCoP found!\n";
   else
     // Print all SCoPs.
     for (SCoPMapType::const_iterator I = RegionToSCoPs.begin(),
        E = RegionToSCoPs.end(); I != E; ++I){
       I->second->print(OS);
     }

  if (PrintLoopBound)
    printBounds(OS);

  if (PrintAccessFunctions)
    printAccFunc(OS);

  OS << "\n";
}

SCoPInfo::~SCoPInfo() {
  clear();

  // Do others thing.
}

char SCoPInfo::ID = 0;

static RegisterPass<SCoPInfo>
X("polly-scop-info", "Polly - Create polyhedral SCoPs Info");

FunctionPass *polly::createSCoPInfoPass() {
  return new SCoPInfo();
}
