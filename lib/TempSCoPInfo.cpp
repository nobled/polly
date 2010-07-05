//===----------- TempSCoPExtraction.cpp  - Extract TempSCoPs -----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Extract loop bounds, access functions and conditions express by SCEV in SCoP.
//
//===----------------------------------------------------------------------===//

#include "polly/TempSCoPInfo.h"
#include "polly/Support/SCoPHelper.h"
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

#define DEBUG_TYPE "polly-scop-extract"
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
/// Helper Class

SCEVAffFunc::SCEVAffFunc(const SCEV *S, SCEVAffFuncType Type, ScalarEvolution *SE)
: FuncType(Type) {
  assert(S && "S can not be null!");

  assert(!isa<SCEVCouldNotCompute>(S)
    && "Broken affine function in SCoP");

  for (AffineSCEVIterator I = affine_begin(S, SE), E = affine_end();
    I != E; ++I) {
      // The constant part must be a SCEVConstant.
      // TODO: support sizeof in coefficient.
      assert(isa<SCEVConstant>(I->second) && "Expect SCEVConst in coefficient!");

      const SCEV *Var = I->first;
      // Extract the constant part
      if (isa<SCEVConstant>(Var))
        // Add the translation component
        TransComp = I->second;
      else if (Var->getType()->isPointerTy()) { // Extract the base address.
        const SCEVUnknown *Addr = dyn_cast<SCEVUnknown>(Var);
        assert(Addr && "Why we got a broken scev?");
        BaseAddr = Addr->getValue();
      } else // Extract other affine component.
        LnrTrans.insert(*I);
  }
}

static void setCoefficient(const SCEV *Coeff, mpz_t v, bool negative) {
  if (Coeff) { // If the coefficient exist
    const SCEVConstant *C = dyn_cast<SCEVConstant>(Coeff);
    const APInt &CI = C->getValue()->getValue();
    // Convert i >= expr to i - expr >= 0
    MPZ_from_APInt(v, negative ?(-CI):CI);
  } else
    isl_int_set_si(v, 0);
}

polly_constraint *SCEVAffFunc::toConditionConstrain(polly_ctx *ctx,
                         polly_dim *dim,
                         const SmallVectorImpl<const SCEVAddRecExpr*> &IndVars,
                         const SmallVectorImpl<const SCEV*> &Params) const {
   unsigned num_in = IndVars.size(),
     num_param = Params.size();

   polly_constraint *c = 0;
   if (getType() == GE)
     c = isl_inequality_alloc(isl_dim_copy(dim));
   else // We alloc equality for "!= 0" and "== 0"
     c = isl_equality_alloc(isl_dim_copy(dim));

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

polly_set *SCEVAffFunc::toConditionSet(polly_ctx *ctx,
                         polly_dim *dim,
                         const SmallVectorImpl<const SCEVAddRecExpr*> &IndVars,
                         const SmallVectorImpl<const SCEV*> &Params) const {
   polly_basic_set *bset = isl_basic_set_universe(isl_dim_copy(dim));
   polly_constraint *c = toConditionConstrain(ctx, dim, IndVars, Params);
   bset = isl_basic_set_add_constraint(bset, c);
   polly_set *ret = isl_set_from_basic_set(bset);
   if (getType() == Ne) {
     // Subtract the set from universe set to construct the inequality
     polly_basic_set *uni = isl_basic_set_universe(isl_dim_copy(dim));
     polly_set *uni_set = isl_set_from_basic_set(uni);
     ret = isl_set_subtract(uni_set, ret);
     DEBUG(dbgs() << "Ne:\n");
     DEBUG(isl_set_print(ret, stderr, 8, ISL_FORMAT_ISL));
   }

   return ret;
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

  if (getType() == GE)
    OS << " >= 0";
  else if (getType() == Eq)
    OS << " == 0";
  else if (getType() == Ne)
    OS << " != 0";
}

/// Helper function to print the condition
static void printBBCond(raw_ostream &OS, const BBCond &Cond,
                        ScalarEvolution *SE) {
  assert(!Cond.empty() && "Unexpect empty condition!");
  Cond[0].print(OS, SE);
  for (unsigned i = 1, e = Cond.size(); i != e; ++i) {
    OS << " && ";
    Cond[i].print(OS, SE);
  }
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
  // Print the loopbounds if current region is a loop,
  // In form of IV >= 0, LoopCount - IV >= 0.
  LoopBoundMapType::const_iterator at = LoopBounds.find(castToLoop(*CurR, *LI));
  if (at != LoopBounds.end()) {
    const Loop *L = at->first;
    PHINode *IV = L->getCanonicalInductionVariable();
    const SCEV *SIV = SE->getSCEV(IV);
    // IV >= LB ==> IV - LB >= 0 and LB is 0 in canonical loop.
    const SCEV *LB = SIV;
    const SCEV *UB = SE->getBackedgeTakenCount(L);
    // Match the type, the type of BackedgeTakenCount mismatch when we have
    // something like this in loop exit:
    //
    //    br i1 false, label %for.body, label %for.end
    //
    // In fact, we could do some optimization before SCoPDetecion, so we do not
    // need to worry about this.
    // Be careful of the sign of the upper bounds, if we meet iv <= -1
    // this means the iteration domain is empty since iv >= 0
    // but if we do a zero extend, this will make a non-empty domain
    UB = SE->getTruncateOrSignExtend(UB, SIV->getType());
    // IV <= UB ==> UB - IV >= 0
    UB = SE->getMinusSCEV(UB, SIV);
    //
    SCEVAffFunc lb(LB, SCEVAffFunc::GE, SE), ub(UB, SCEVAffFunc::GE, SE);

    OS.indent(ind) << "Bounds of Loop: " << at->first->getHeader()->getName()
      << ":\t{ ";
    lb.print(OS,SE);
    OS << ", ";
    ub.print(OS,SE);
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
      // Print the condition
      if (const BBCond *Cond = getBBCond(subR->getEntry())) {
        OS << "Constrain of Region " << subR->getNameStr() << ":\t";
        printBBCond(OS, *Cond, SE);
        OS << '\n';
      }
      printDetail(OS, SE, LI, subR, subInd);
    } else {
      unsigned bb_ind = ind + 2;
      BasicBlock *BB = I->getNodeAs<BasicBlock>();
      if (BB != CurR->getEntry())
        if (const BBCond *Cond = getBBCond(BB)) {
          OS << "Constrain of BB " << BB->getName() << ":\t";
          printBBCond(OS, *Cond, SE);
          OS << '\n';
        }

      if (const AccFuncSetType *AccFunc = getAccessFunctions(BB)) {
        OS.indent(ind) << "BB: " << BB->getName() << "{\n";
        for (AccFuncSetType::const_iterator FI = AccFunc->begin(),
            FE = AccFunc->end(); FI != FE; ++FI) {
          FI->print(OS.indent(bb_ind),SE);
          OS << "\n";
        }
        OS.indent(ind) << "}\n";
      }
    }
  }
}

//===----------------------------------------------------------------------===//
// TempSCoP information extraction pass implement
char TempSCoPInfo::ID = 0;

static RegisterPass<TempSCoPInfo>
X("polly-scop-extract", "Polly - Extract TempSCoPs");

void TempSCoPInfo::buildAffineFunction(const SCEV *S, SCEVAffFunc &FuncToBuild,
                                       TempSCoP &SCoP) const {
  assert(S && "S can not be null!");

  assert(!isa<SCEVCouldNotCompute>(S)
    && "Un Expect broken affine function in SCoP!");

  Region &CurRegion = SCoP.getMaxRegion();
  BasicBlock *CurBB = CurRegion.getEntry();

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
    if (isa<SCEVConstant>(Var))
      // Add the translation component
      FuncToBuild.TransComp = I->second;
    else if (Var->getType()->isPointerTy()) { // Extract the base address
      const SCEVUnknown *BaseAddr = dyn_cast<SCEVUnknown>(Var);
      assert(BaseAddr && "Why we got a broken scev?");
      FuncToBuild.BaseAddr = BaseAddr->getValue();
    } else { // Extract others affine component
      FuncToBuild.LnrTrans.insert(*I);
      // Do not add the indvar to the parameter list.
      if (!isIndVar(Var, CurRegion, CurBB, *LI, *SE)) {
        DEBUG(dbgs() << "Non indvar: "<< *Var << '\n');
        assert(isParameter(Var, CurRegion, CurBB, *LI, *SE)
               && "Find non affine function in scop!");
        SCoP.getParamSet().insert(Var);
      }
    }
  }
}

void TempSCoPInfo::buildScalarDataRef(Instruction &Inst,
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

void TempSCoPInfo::buildAccessFunctions(TempSCoP &SCoP, BasicBlock &BB,
                                        AccFuncSetType &Functions) {
  for (BasicBlock::iterator I = BB.begin(), E = --BB.end(); I != E; ++I) {
    Instruction &Inst = *I;
    buildScalarDataRef(Inst, Functions);
    if (isa<LoadInst>(&Inst) || isa<StoreInst>(&Inst)) {
      // Create the SCEVAffFunc.
      if (isa<LoadInst>(Inst))
        Functions.push_back(SCEVAffFunc(SCEVAffFunc::ReadMem));
      else //Else it must be a StoreInst.
        Functions.push_back(SCEVAffFunc(SCEVAffFunc::WriteMem));

      Value *Ptr = getPointerOperand(Inst);
      buildAffineFunction(SE->getSCEV(Ptr), Functions.back(), SCoP);
    }
  }
}

void TempSCoPInfo::buildLoopBound(TempSCoP &SCoP) {
  if (Loop *L = castToLoop(SCoP.getMaxRegion(), *LI)) {
    const SCEV *LoopCount = SE->getBackedgeTakenCount(L);
    std::pair<LoopBoundMapType::iterator, bool> at =
            LoopBounds.insert(std::make_pair(L, SCEVAffFunc(SCEVAffFunc::GE)));
    // Build the affine function for loop count.
    buildAffineFunction(LoopCount, at.first->second, SCoP);

    // Increase the loop depth because we found a loop.
    ++SCoP.MaxLoopDepth;
  }
}

void TempSCoPInfo::buildAffineCondition(Value &V, bool inverted,
                                         SCEVAffFunc &FuncToBuild,
                                         TempSCoP &SCoP) const {
  if (ConstantInt *C = dyn_cast<ConstantInt>(&V)) {
    // If this is always true condition, we will create 1 >= 0,
    // otherwise we will create 1 == 0.
    if (C->isOne() == inverted)
      FuncToBuild.FuncType = SCEVAffFunc::Eq;
    else
      FuncToBuild.FuncType = SCEVAffFunc::GE;

    buildAffineFunction(SE->getConstant(C->getType(), 1), FuncToBuild, SCoP);

    return;
  }

  ICmpInst *ICmp = dyn_cast<ICmpInst>(&V);
  // FIXME: we should check this in isValidSCoP
  assert(ICmp && "Only ICmpInst of constant as condition supported!");
  const SCEV *LHS = SE->getSCEV(ICmp->getOperand(0)),
             *RHS = SE->getSCEV(ICmp->getOperand(1));

  ICmpInst::Predicate Pred = ICmp->getPredicate();

  // Invert the predicate if needed.
  if (inverted)
    Pred = ICmpInst::getInversePredicate(Pred);

  // FIXME: In fact, this is only loop exit condition, try to use
  // getBackedgeTakenCount.
  switch (Pred) {
  case ICmpInst::ICMP_EQ:
    FuncToBuild.FuncType = SCEVAffFunc::Eq;
    break;
  case ICmpInst::ICMP_NE:
    FuncToBuild.FuncType = SCEVAffFunc::Ne;
    break;
  case ICmpInst::ICMP_SLT:
  case ICmpInst::ICMP_ULT:
    // A < B => B > A
    std::swap(LHS, RHS);
    // goto case ICmpInst::ICMP_UGT:
  case ICmpInst::ICMP_SGT:
  case ICmpInst::ICMP_UGT:
    // A > B ==> A >= B + 1
    // FIXME: NSW or NUW?
    RHS = SE->getAddExpr(RHS, SE->getConstant(RHS->getType(), 1));
    FuncToBuild.FuncType = SCEVAffFunc::GE;
    break;
  case ICmpInst::ICMP_SLE:
  case ICmpInst::ICMP_ULE:
    // A <= B ==> B => A
    std::swap(LHS, RHS);
    // goto case ICmpInst::ICMP_UGE:
  case ICmpInst::ICMP_SGE:
  case ICmpInst::ICMP_UGE:
    FuncToBuild.FuncType = SCEVAffFunc::GE;
    break;
  default:
    llvm_unreachable("Unknown Predicate!");
  }
  // Build the condition with FuncType
  // Transform A >= B to A - B >= 0
  buildAffineFunction(SE->getMinusSCEV(LHS, RHS), FuncToBuild, SCoP);
}

void TempSCoPInfo::buildCondition(BasicBlock *BB, BasicBlock *RegionEntry,
                                   BBCond &Cond, TempSCoP &SCoP) {
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
    bool inverted = DT->dominates(Br->getSuccessor(1), CurBB);
    // Build the condition in SCEV form
    Cond.push_back(SCEVAffFunc());
    buildAffineCondition(*(Br->getCondition()), inverted, Cond.back(), SCoP);
    DEBUG(
      Cond.back().print(dbgs(), SE);
      dbgs() << " && ";
    );
  }
  DEBUG(dbgs() << '\n');
}


TempSCoP *TempSCoPInfo::buildTempSCoP(Region &R) {
  TempSCoP *SCoP = new TempSCoP(R, LoopBounds, BBConds, AccFuncMap);
  AccFuncSetType AccFuncs;
  BBCond Cond;
  for (Region::element_iterator I = R.element_begin(), E = R.element_end();
    I != E; ++I) {
      Cond.clear();
      // Build the condition
      buildCondition(I->getEntry(), R.getEntry(), Cond, *SCoP);
      // Remember it if there is any condition extracted
      if (!Cond.empty()) {
        // A same BB will be visit multiple time if we iterate on the
        // element_iterator down the region tree and get the BB by "getEntry",
        // if this BB is the entry block of some region.
        // one time(s, because multiple region can share one entry block)
        // is when BB is some regions entry block, and we get the region node
        // from the region(The region that have BB as entry block)'s parent
        // region, this time, we will build the condition from the parent's entry
        // to the BB.
        // another time is when we get the region node from the the smallest
        // region that containing this BB, but now I->getEntry() and R.getEntry()
        // are the same bb, so no condition expect to extract.
        assert(!BBConds.count(I->getEntry())
          && "Why we got more than one conditions for a BB?");
        BBConds.insert(
          std::make_pair<const BasicBlock*, BBCond>(I->getEntry(), Cond));
      }
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
  buildLoopBound(*SCoP);
  return SCoP;
}

void TempSCoPInfo::mergeParams(Region &R, ParamSetType &Params,
                                   ParamSetType &SubParams) const {
  Loop *L = castToLoop(R, *LI);
  // Merge the parameters.
  for (ParamSetType::iterator I = SubParams.begin(),
    E = SubParams.end(); I != E; ++I) {
      const SCEV *Param = *I;
      // The valid parameter in subregion may not valid in its parameter
      if (isParameter(Param, R, R.getEntry(), *LI, *SE)) {
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

TempSCoP *TempSCoPInfo::getTempSCoP(Region& R) {
  assert(SD->isSCoP(R) && "R is expect to be found!");
  // Did we already compute the SCoP for R?
  TempSCoPMapType::const_iterator at = RegionToSCoPs.find(&R);
  if (at != RegionToSCoPs.end())
    return at->second;

  // Otherwise, we had to extract the temporary SCoP information.
  TempSCoP *tempSCoP = buildTempSCoP(R);
  RegionToSCoPs[&R] = tempSCoP;
  return tempSCoP;
}


TempSCoP *TempSCoPInfo::getTempSCoP() const {
  // Only extract the TempSCoP infor for valid region
  if (!SD->isSCoP(*CurR)) return 0;

  // Only analyse the maximal SCoPs.
  if (!SD->isMaxRegionInSCoP(*CurR)) return 0;

  // Recalculate the temporary SCoP info.
  return const_cast<TempSCoPInfo*>(this)->getTempSCoP(*CurR);
}

void TempSCoPInfo::print(raw_ostream &OS, const Module *) const {
  if (TempSCoP *TS = getTempSCoP())
      TS->print(OS, SE, LI);
  else
    OS << "Region: " << CurR->getNameStr() << " is Not Valid SCoP!\n";
}


bool TempSCoPInfo::runOnRegion(Region *R, RGPassManager &RGM) {
  DT = &getAnalysis<DominatorTree>();
  PDT = &getAnalysis<PostDominatorTree>();
  SE = &getAnalysis<ScalarEvolution>();
  LI = &getAnalysis<LoopInfo>();
  SD = &getAnalysis<SCoPDetection>();
  SDR = &getAnalysis<ScalarDataRef>();
  CurR = R;
  return false;
}

void TempSCoPInfo::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequiredTransitive<DominatorTree>();
  AU.addRequiredTransitive<PostDominatorTree>();
  AU.addRequiredTransitive<LoopInfo>();
  AU.addRequiredTransitive<ScalarEvolution>();
  AU.addRequiredTransitive<SCoPDetection>();
  // Request ScalarDataRef pass run
  AU.addRequiredTransitive<ScalarDataRef>();
  AU.setPreservesAll();
}

TempSCoPInfo::~TempSCoPInfo() {
  clear();
}

void TempSCoPInfo::clear() {
  BBConds.clear();
  LoopBounds.clear();
  AccFuncMap.clear();

  // Delete all tempSCoP entry in the maps.
  while (!RegionToSCoPs.empty()) {
    TempSCoPMapType::iterator I = RegionToSCoPs.begin();
    if (I->second) delete I->second;

    RegionToSCoPs.erase(I);
  }
}
