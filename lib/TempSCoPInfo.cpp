//===---------- TempSCoPInfo.cpp  - Extract TempSCoPs ---------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Collect information about the control flow regions detected by the SCoP
// detection, such that this information can be translated info its polyhedral
// representation.
//
//===----------------------------------------------------------------------===//

#include "polly/TempSCoPInfo.h"

#include "polly/LinkAllPasses.h"
#include "polly/Support/AffineSCEVIterator.h"
#include "polly/Support/GmpConv.h"
#include "polly/Support/SCoPHelper.h"

#include "llvm/Analysis/RegionIterator.h"

#define DEBUG_TYPE "polly-analyze-ir"
#include "llvm/Support/Debug.h"

using namespace llvm;
using namespace polly;

//===----------------------------------------------------------------------===//
/// Helper Class

SCEVAffFunc::SCEVAffFunc(const SCEV *S, SCEVAffFuncType Type,
                         ScalarEvolution *SE) : FuncType(Type) {
  assert(S && "S can not be null!");
  assert(!isa<SCEVCouldNotCompute>(S) && "Non affine function in SCoP");

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
      assert(Addr && "Why did we get a broken scev?");
      BaseAddr = Addr->getValue();
    } else // Extract other affine components.
      LnrTrans.insert(*I);
  }
}

void SCEVAffFunc::print(raw_ostream &OS, bool PrintInequality) const {
  // Print BaseAddr.
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

  if (!PrintInequality)
    return;

  if (getType() == GE)
    OS << " >= 0";
  else if (getType() == Eq)
    OS << " == 0";
  else if (getType() == Ne)
    OS << " != 0";
}

void SCEVAffFunc::dump() const {
  print(errs());
}

inline raw_ostream &operator<<(raw_ostream &OS, const SCEVAffFunc &AffFunc) {
  AffFunc.print(OS);
  return OS;
}

/// Helper function to print the condition
static void printBBCond(raw_ostream &OS, const BBCond &Cond) {
  assert(!Cond.empty() && "Unexpected empty condition!");
  Cond[0].print(OS);
  for (unsigned i = 1, e = Cond.size(); i != e; ++i) {
    OS << " && ";
    Cond[i].print(OS);
  }
}

inline raw_ostream &operator<<(raw_ostream &OS, const BBCond &Cond) {
  printBBCond(OS, Cond);
  return OS;
}


//===----------------------------------------------------------------------===//
// TempSCoP implementation

void TempSCoP::print(raw_ostream &OS, ScalarEvolution *SE, LoopInfo *LI) const {
  OS << "SCoP: " << R.getNameStr() << "\tParameters: (";
  // Print Parameters.
  for (ParamSetType::const_iterator PI = Params.begin(), PE = Params.end();
    PI != PE; ++PI)
    OS << **PI << ", ";

  OS << "), Max Loop Depth: "<< MaxLoopDepth <<"\n";

  printDetail(OS, SE, LI, &R, 0);
}

void TempSCoP::printDetail(llvm::raw_ostream &OS, ScalarEvolution *SE,
                           LoopInfo *LI, const Region *CurR,
                           unsigned ind) const {
  // Print the loop bounds,  if the current region is a loop.
  LoopBoundMapType::const_iterator at = LoopBounds.find(castToLoop(*CurR, *LI));
  if (at != LoopBounds.end()) {
    OS.indent(ind) << "Bounds of Loop: " << at->first->getHeader()->getName()
      << ":\t{ ";
    at->second.print(OS, false);
    OS << " }\n";
    ind += 2;
  }

  // Iterate over the region nodes of this SCoP to print the access functions
  // and loop bounds.
  for (Region::const_element_iterator I = CurR->element_begin(),
       E = CurR->element_end(); I != E; ++I) {
    if (I->isSubRegion()) {
      Region *subR = I->getNodeAs<Region>();

      if (const BBCond *Cond = getBBCond(subR->getEntry()))
        OS << "Constrain of Region " << subR->getNameStr() << ":\t" << *Cond
          << '\n';

      printDetail(OS, SE, LI, subR, ind + 2);
    } else {
      BasicBlock *BB = I->getNodeAs<BasicBlock>();

      if (BB != CurR->getEntry())
        if (const BBCond *Cond = getBBCond(BB))
          OS << "Constrain of BB " << BB->getName() << ":\t" << *Cond << '\n';

      if (const AccFuncSetType *AccFunc = getAccessFunctions(BB)) {
        OS.indent(ind) << "BB: " << BB->getName() << "{\n";

        for (AccFuncSetType::const_iterator FI = AccFunc->begin(),
             FE = AccFunc->end(); FI != FE; ++FI)
          OS.indent(ind + 2) << *FI << '\n';

        OS.indent(ind) << "}\n";
      }
    }
  }
}

void TempSCoPInfo::buildAffineFunction(const SCEV *S, SCEVAffFunc &FuncToBuild,
                                       Region &R, ParamSetType &Params) const {
  assert(S && "S can not be null!");

  assert(!isa<SCEVCouldNotCompute>(S)
    && "Un Expect broken affine function in SCoP!");

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
    } else { // Extract other affine components.
      FuncToBuild.LnrTrans.insert(*I);
      // Do not add the indvar to the parameter list.
      if (!isIndVar(Var, R, *LI, *SE)) {
        DEBUG(dbgs() << "Non indvar: "<< *Var << '\n');
        assert(isParameter(Var, R, *LI, *SE)
               && "Find non affine function in scop!");
        Params.insert(Var);
      }
    }
  }
}

void TempSCoPInfo::buildAccessFunctions(Region &R, ParamSetType &Params,
                                        BasicBlock &BB) {
  AccFuncSetType Functions;
  for (BasicBlock::iterator I = BB.begin(), E = --BB.end(); I != E; ++I) {
    Instruction &Inst = *I;
    if (isa<LoadInst>(&Inst) || isa<StoreInst>(&Inst)) {
      // Create the SCEVAffFunc.
      if (isa<LoadInst>(Inst))
        Functions.push_back(SCEVAffFunc(SCEVAffFunc::ReadMem));
      else //Else it must be a StoreInst.
        Functions.push_back(SCEVAffFunc(SCEVAffFunc::WriteMem));

      Value *Ptr = getPointerOperand(Inst);
      buildAffineFunction(SE->getSCEV(Ptr), Functions.back(), R, Params);
    }
  }

  if (Functions.empty())
    return;

  AccFuncSetType &Accs = AccFuncMap[&BB];
  Accs.insert(Accs.end(), Functions.begin(), Functions.end());
}

void TempSCoPInfo::buildLoopBounds(TempSCoP &SCoP) {
  Region &R = SCoP.getMaxRegion();
  unsigned MaxLoopDepth = 0;

  for (Region::block_iterator I = R.block_begin(), E = R.block_end();
       I != E; ++I) {
    Loop *L = LI->getLoopFor(I->getNodeAs<BasicBlock>());

    if (!L || !R.contains(L))
      continue;

    if (LoopBounds.find(L) != LoopBounds.end())
      continue;

    LoopBounds[L] = SCEVAffFunc(SCEVAffFunc::GE);
    const SCEV *LoopCount = SE->getBackedgeTakenCount(L);
    buildAffineFunction(LoopCount, LoopBounds[L], SCoP.getMaxRegion(),
                        SCoP.getParamSet());

    Loop *OL = R.outermostLoopInRegion(L);
    unsigned LoopDepth = L->getLoopDepth() - OL->getLoopDepth() + 1;

    if (LoopDepth > MaxLoopDepth)
      MaxLoopDepth = LoopDepth;
  }

  SCoP.MaxLoopDepth = MaxLoopDepth;
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

    buildAffineFunction(SE->getConstant(C->getType(), 1), FuncToBuild,
                        SCoP.getMaxRegion(), SCoP.getParamSet());
    FuncToBuild.setUnsigned();

    return;
  }

  ICmpInst *ICmp = dyn_cast<ICmpInst>(&V);
  assert(ICmp && "Only ICmpInst of constant as condition supported!");
  const SCEV *LHS = SE->getSCEV(ICmp->getOperand(0)),
             *RHS = SE->getSCEV(ICmp->getOperand(1));

  ICmpInst::Predicate Pred = ICmp->getPredicate();

  // Invert the predicate if needed.
  if (inverted)
    Pred = ICmpInst::getInversePredicate(Pred);

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

  switch (Pred) {
  case ICmpInst::ICMP_UGT:
  case ICmpInst::ICMP_UGE:
  case ICmpInst::ICMP_ULT:
  case ICmpInst::ICMP_ULE:
    FuncToBuild.setUnsigned();
    break;
  default:
    break;
  }

  // Transform A >= B to A - B >= 0
  buildAffineFunction(SE->getMinusSCEV(LHS, RHS), FuncToBuild,
                      SCoP.getMaxRegion(), SCoP.getParamSet());
}

void TempSCoPInfo::buildCondition(BasicBlock *BB, BasicBlock *RegionEntry,
                                  TempSCoP &SCoP) {
  BBCond Cond;
  DomTreeNode *BBNode = DT->getNode(BB), *EntryNode = DT->getNode(RegionEntry);
  assert(BBNode && EntryNode && "Get null node while building condition!");

  // Walk up the dominance tree until reaching the entry node. Add all
  // conditions on the path to BB except if BB postdominates the block
  // containing the condition.
  while (BBNode != EntryNode) {
    BasicBlock *CurBB = BBNode->getBlock();
    BBNode = BBNode->getIDom();
    assert(BBNode && "BBNode should not reach the root node!");

    if (PDT->dominates(CurBB, BBNode->getBlock()))
      continue;

    BranchInst *Br = dyn_cast<BranchInst>(BBNode->getBlock()->getTerminator());
    assert(Br && "A Valid SCoP should only contain branch instruction");

    if (Br->isUnconditional())
      continue;

    // Is BB on the ELSE side of the branch?
    bool inverted = DT->dominates(Br->getSuccessor(1), BB);

    Cond.push_back(SCEVAffFunc());
    buildAffineCondition(*(Br->getCondition()), inverted, Cond.back(), SCoP);
  }

  if (!Cond.empty())
    BBConds[BB] = Cond;
}

TempSCoP *TempSCoPInfo::buildTempSCoP(Region &R) {
  TempSCoP *TSCoP = new TempSCoP(R, LoopBounds, BBConds, AccFuncMap);

  for (Region::block_iterator I = R.block_begin(), E = R.block_end();
       I != E; ++I) {
    BasicBlock *BB =  I->getNodeAs<BasicBlock>();
    buildAccessFunctions(R, TSCoP->getParamSet(), *BB);
    buildCondition(BB, R.getEntry(), *TSCoP);
  }

  buildLoopBounds(*TSCoP);
  return TSCoP;
}

TempSCoP *TempSCoPInfo::getTempSCoP() const {
  return TSCoP;
}

void TempSCoPInfo::print(raw_ostream &OS, const Module *) const {
  if (TSCoP)
    TSCoP->print(OS, SE, LI);
}

bool TempSCoPInfo::runOnRegion(Region *R, RGPassManager &RGM) {
  DT = &getAnalysis<DominatorTree>();
  PDT = &getAnalysis<PostDominatorTree>();
  SE = &getAnalysis<ScalarEvolution>();
  LI = &getAnalysis<LoopInfo>();
  SD = &getAnalysis<SCoPDetection>();

  TSCoP = NULL;

  // Only analyse maximal SCoPs.
  if (!SD->isMaxRegionInSCoP(*R)) return false;

  TSCoP = buildTempSCoP(*R);

  return false;
}

void TempSCoPInfo::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequiredTransitive<DominatorTree>();
  AU.addRequiredTransitive<PostDominatorTree>();
  AU.addRequiredTransitive<LoopInfo>();
  AU.addRequiredTransitive<ScalarEvolution>();
  AU.addRequiredTransitive<SCoPDetection>();
  AU.addRequiredID(IndependentBlocksID);
  AU.setPreservesAll();
}

TempSCoPInfo::~TempSCoPInfo() {
  clear();
}

void TempSCoPInfo::clear() {
  BBConds.clear();
  LoopBounds.clear();
  AccFuncMap.clear();
  if (TSCoP)
    delete TSCoP;
  TSCoP = 0;
}

//===----------------------------------------------------------------------===//
// TempSCoP information extraction pass implement
char TempSCoPInfo::ID = 0;

static RegisterPass<TempSCoPInfo>
X("polly-analyze-ir", "Polly - Analyse the LLVM-IR in the detected regions");

