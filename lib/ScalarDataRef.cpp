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
#include "polly/LinkAllPasses.h"
#include "polly/Support/SCoPHelper.h"

#include "llvm/Instruction.h"
#include "llvm/Pass.h"
#include "llvm/Assembly/Writer.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/RegionInfo.h"
#include "llvm/Analysis/Dominators.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/ScalarEvolutionExpander.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/OwningPtr.h"
#include "llvm/ADT/PointerIntPair.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Utils/Local.h"

#define DEBUG_TYPE "polly-scalar-data-ref"
#include "llvm/Support/Debug.h"


using namespace llvm;
using namespace polly;

namespace {
//===----------------------------------------------------------------------===//
/// @brief Scalar data reference - Translate scalar data dependencies to one
/// element array access.
///
class ScalarDataRef : public FunctionPass {
  // DO NOT IMPLEMENT.
  ScalarDataRef(const ScalarDataRef &);
  // DO NOT IMPLEMENT.
  const ScalarDataRef &operator=(const ScalarDataRef &);

  // LoopInfo to find canonical induction variable.
  LoopInfo *LI;
  // ScalarEvolution to help us rewrite the necessary scalar dependencies.
  ScalarEvolution *SE;

  // load/store address to remember.
  typedef std::map<Value*, const SCEV*> AddrMapType;
  AddrMapType AddrToRewrite;
  // Conditions to remember, with the information in this map, we can
  // reconstruct and ICmp instruction.
  typedef std::map<BranchInst*, // The user of this condition.
                  std::pair<ICmpInst::Predicate, // The predicate.
                  // Left hand side operand and right hand side operand.
                  std::pair<const SCEV*, const SCEV*> > > 
                  CndMapType;
  CndMapType CndToRewrite;
  // Clear the context.
  void clear();

  // Translate scalar dependencies to memory dependencies.
  void translateRegToMem(Function &F);

  // Rewrite the address and condition computation back to scalar form in SCoP.
  void rewriteAddrAndCnd(Function &F);

  // Should us translate this scalar to one element array?
  static bool valueEscapes(const Instruction *Inst, Loop *L);

  // Remember the address of a load or store instruction, so we could rewrite
  // it later.
  void rememberAddrOf(Instruction *I);

  // Remember the condition of a branch instruction, so we can rewrite it later.
  void rememberCndOf(BranchInst *Br);

public:
  static char ID;

  explicit ScalarDataRef() : FunctionPass(&ID) {}
  ~ScalarDataRef();

  /// @name FunctionPass interface
  //@{
  virtual void getAnalysisUsage(AnalysisUsage &AU) const;
  virtual void releaseMemory();
  virtual bool runOnFunction(Function &F);
  virtual void print(raw_ostream &OS, const Module *) const;
  //@}

};
}

//===----------------------------------------------------------------------===//
/// ScalarDataRef implement

void ScalarDataRef::clear() {
  CndToRewrite.clear();
  AddrToRewrite.clear();
}

ScalarDataRef::~ScalarDataRef() {
  clear();
}

void ScalarDataRef::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<ScalarEvolution>();
  AU.addRequired<LoopInfo>();
  //AU.addRequiredID(BreakCriticalEdgesID);
  //AU.addPreservedID(BreakCriticalEdgesID);
  AU.setPreservesCFG();
}

bool ScalarDataRef::runOnFunction(Function &F) {
  if (F.isDeclaration())
    return false;

  LI = &getAnalysis<LoopInfo>();
  SE = &getAnalysis<ScalarEvolution>();

  // Step 1: Translate almost all inter BasicBlock scalar dependencies to
  // memory dependencies, So the later SCoP extraction pass will detected
  // these dependencies. This step also remember the SCEV of address and
  // condition computation, so we can write them back to scalar form later.
  translateRegToMem(F);

  // Step 2: In step 1, we break the computation of address and condition to
  // small fragment, start and end by load and store instruction.
  // This form of address and condition will prevent us from extract the access
  // function and iterate domain correctly.
  // So we had to rewrite them to
  // For example, we can extract the affine function correctly if it was
  // computed by:
  // a = 2 * c + 3 * d
  // But after step 1, we may get computation of a like this:
  // c2 = 2 * c
  // store c2
  // d3 = 3 * d
  // store d3
  // c22 = load c2
  // d33 = load d3
  // a = c22 + d33
  // and we can not extract the affine function correctly.
  rewriteAddrAndCnd(F);

  // Step 3: Perform Aggressive Dead Code Elimination to remove the dead load
  // and store instruction after we rewrite address and condition computation
  // back to scalar.
  OwningPtr<FunctionPass> ADCE(createAggressiveDCEPass());
  ADCE->run(F);

  return true;
}

void ScalarDataRef::releaseMemory() {
  clear();
}

void ScalarDataRef::print(raw_ostream &OS, const Module *) const {
}

void ScalarDataRef::rememberCndOf(BranchInst *Br) {
  if (!Br->isConditional())
    return;

  ICmpInst *ICmp = dyn_cast<ICmpInst>(Br->getCondition());
  if (ICmp == 0)
    return;

  const SCEV *LHS = SE->getSCEVAtScope(ICmp->getOperand(0),
                                       LI->getLoopFor(ICmp->getParent())),
             *RHS = SE->getSCEVAtScope(ICmp->getOperand(1),
                                       LI->getLoopFor(ICmp->getParent()));

  CndToRewrite.insert(std::make_pair(Br,
                                     std::make_pair(ICmp->getPredicate(),
                                                    std::make_pair(LHS, RHS))));
}

void ScalarDataRef::translateRegToMem(Function &F) {
  // Insert all new allocas into entry block.
  BasicBlock *BBEntry = &F.getEntryBlock();

  // Find first non-alloca instruction and create insertion point. This is
  // safe if block is well-formed: it always have terminator, otherwise
  // we'll get and assertion.
  BasicBlock::iterator I = BBEntry->begin();
  while (isa<AllocaInst>(I)) ++I;

  // Create the insert point marker.
  CastInst *AllocaInsertionPoint =
    new BitCastInst(Constant::getNullValue(Type::getInt32Ty(F.getContext())),
    Type::getInt32Ty(F.getContext()),
    "reg2mem alloca point", I);

  std::vector<Instruction*> WorkList;
  std::vector<PHINode*> IVs;

  // Find all phi's and iv's
  for (Function::iterator ibb = F.begin(), ibe = F.end();
      ibb != ibe; ++ibb) {
    Loop *L = LI->getLoopFor(ibb);

    for (BasicBlock::iterator iib = ibb->begin(), iie = ibb->end();
        iib != iie; ++iib) {
      if (L && L->getCanonicalInductionVariable() == iib) {
        IVs.push_back(cast<PHINode>(iib));
        continue;
      }

      if (isa<PHINode>(iib))
        WorkList.push_back(&*iib);
    }
  }

  // Translate all non-iv phi's to memory access.
  while (!WorkList.empty()) {
    DemotePHIToStack(cast<PHINode>(WorkList.back()), AllocaInsertionPoint);
    WorkList.pop_back();
  }

  // We need to move the iv's (phi nodes) to the beginning of the BasicBlock.
  while (!IVs.empty()) {
    PHINode *PN = IVs.back();
    IVs.pop_back();

    BasicBlock *BB = PN->getParent();
    PN->removeFromParent();
    PN->insertBefore(BB->begin());
  }

  // Find instruction's that have inter BasicBlock dependencies.
  for (Function::iterator ibb = F.begin(), ibe = F.end();
      ibb != ibe; ++ibb) {
    Loop *L = LI->getLoopFor(ibb);

    for (BasicBlock::iterator iib = ibb->begin(), iie = ibb->end();
        iib != iie; ++iib) {
      if (!(isa<AllocaInst>(iib) && iib->getParent() == BBEntry) &&
          valueEscapes(iib, L))
        WorkList.push_back(&*iib);

      // Remember some value, we will rewrite them to scalar soon.
      if (isa<LoadInst>(iib) || isa<StoreInst>(iib))
        rememberAddrOf(iib);

      if (BranchInst *Br = dyn_cast<BranchInst>(iib))
        rememberCndOf(Br);
    }
  }

  // Translate them to memory access.
  while (!WorkList.empty()) {
    DemoteRegToStack(*WorkList.back(), false, AllocaInsertionPoint);
    WorkList.pop_back();
  }

  // Remove the marker.
  AllocaInsertionPoint->eraseFromParent();
}

void ScalarDataRef::rewriteAddrAndCnd(Function &F) {
  SCEVExpander Rewriter(*SE);
  Rewriter.disableInstructionHoisting();

  for (Function::iterator ibb = F.begin(), ibe = F.end();
      ibb != ibe; ++ibb) {
    for (BasicBlock::iterator iib = ibb->begin(), iie = --ibb->end();
        iib != iie; ++iib) {
      if (!(isa<LoadInst>(iib) || isa<StoreInst>(iib)))
        continue;

      AddrMapType::iterator at = AddrToRewrite.find(iib);
      if (at == AddrToRewrite.end())
        continue;

      // Try to rewrite the address compuation of back to scalar, so we can
      // perform a full scev on it.
      Value *Addr = Rewriter.expandCodeFor(at->second, 0, iib);
      if (LoadInst *Ld = dyn_cast<LoadInst>(iib)) {
        Ld->setOperand(LoadInst::getPointerOperandIndex(), Addr);
        continue;
      }

      StoreInst *St = cast<StoreInst>(iib);
      St->setOperand(StoreInst::getPointerOperandIndex(), Addr);
    }

    // Try to rewrite the condition compuation of back to scalar, so we can
    // perform a full scev on it.
    if (BranchInst *Br = dyn_cast<BranchInst>(ibb->getTerminator())) {
      CndMapType::iterator at = CndToRewrite.find(Br);
      if (at != CndToRewrite.end()) {
        ICmpInst::Predicate Pred = at->second.first;

        const SCEV *LHS = at->second.second.first,
                   *RHS = at->second.second.second;
        Value *LHSV = Rewriter.expandCodeFor(LHS, 0, Br),
              *RHSV = Rewriter.expandCodeFor(RHS, 0, Br);

        CmpInst *ICmp = CmpInst::Create(Instruction::ICmp, Pred,
                                        LHSV, RHSV, "", Br);

        Br->setCondition(ICmp);
      }
    }
    // Force the rewriter to forget the emitted instructions, so the rewriter
    // will not resue them. As a result, address and condition computation will
    // not have inter BasicBlock scalar dependencies.
    // FIXME: This is not necessary?
    Rewriter.clear();
  }
}

void ScalarDataRef::rememberAddrOf(Instruction *I) {
  Value *Addr = getPointerOperand(*I);
  if (GetElementPtrInst *GEP = dyn_cast<GetElementPtrInst>(Addr)) {
    const SCEV *S = SE->getSCEVAtScope(GEP, LI->getLoopFor(I->getParent()));
    AddrToRewrite.insert(std::make_pair(I, S));
  }
}

bool ScalarDataRef::valueEscapes(const Instruction *Inst, Loop *L) {
  // Translate induction variable and its increment to memory access is not
  // necessary.
  if (L && L->getCanonicalInductionVariable() == Inst)
    return false;

  // Translate GEP to memory access is not necessary, too.
  if (isa<GetElementPtrInst>(Inst))
    return false;

  const BasicBlock *BB = Inst->getParent();
  for (Value::const_use_iterator UI = Inst->use_begin(),E = Inst->use_end();
    UI != E; ++UI) {
      const Instruction *I = cast<Instruction>(*UI);
      // Only translate the instruction that have inter basicblock dependencies.
      if (I->getParent() != BB || isa<PHINode>(I))
        return true;
  }
  return false;
}

char ScalarDataRef::ID = 0;

RegisterPass<ScalarDataRef> X("polly-scalar-data-ref",
                              "Polly - Scalar Data Reference Analysis",
                              false, true);

Pass *polly::createScalarDataRefPass() {
  return new ScalarDataRef();
}
