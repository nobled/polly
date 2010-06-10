//===- ClastCodeGeneration.cpp - Create LLVM IR from the CLooG AST --------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Create LLVM IR code using the ClastParser.
//
//===----------------------------------------------------------------------===//

#include "polly/ClastParser.h"
#include "polly/Support/GmpConv.h"
#include "polly/Support/IRHelper.h"
#include "polly/Support/BasicBlockEdge.h"
#include "polly/CLooG.h"
#include "llvm/ADT/APInt.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/IRBuilder.h"
#include "llvm/Analysis/RegionIterator.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/ScalarEvolutionExpander.h"


#define CLOOG_INT_GMP 1
#include "cloog/cloog.h"

#include <vector>

#ifdef _WINDOWS
#define snprintf _snprintf
#endif

using namespace polly;
using namespace llvm;

namespace polly {

typedef DenseMap<const Value*, Value*> ValueMapT;
typedef DenseMap<const char*, Value*> CharMapT;

//=== Code generation global state -------//
struct codegenctx : cp_ctx {
  // The SCoP we code generate.
  SCoP *S;

  DominatorTree *DT;
  ScalarEvolution *SE;

  // The Builder specifies the current location to code generate at.
  IRBuilder<> *Builder;

  // For each open condition the merge basic block.
  std::vector<BasicBlock*> GuardMergeBBs;

  // For each open loop the induction variable.
  std::vector<PHINode*> LoopIVs;

  // For each open loop the induction variable after it was incremented.
  std::vector<Value*> LoopIncrementedIVs;

  // For each open loop the basic block following the loop exit.
  std::vector<BasicBlock*> LoopAfterBBs;

  // The last value calculated by a subexpression.
  Value *exprValue;

  // Map the Values from the old code to their counterparts in the new code.
  ValueMapT ValueMap;

  // Map the textual representation of variables in clast to the actual
  // Values* created during code generation.  It is used to map every
  // appearance of such a string in the clast to the value created
  // because of a loop iv or an assignment.
  CharMapT CharMap;

  // Count the current assignment.  This is for user statements
  // to track how an IV from the old code corresponds to a value
  // or expression in the new code.
  //
  // There is one assignment with an empty LHS for every IV dimension
  // of each statement.
  unsigned assignmentCount;

  // The current statement we are working on.
  //
  // Also used to track the assignment of old IVS to new values or
  // expressions.
  SCoPStmt *stmt;

  codegenctx(SCoP *scop, DominatorTree *DomTree, ScalarEvolution *ScEv,
             IRBuilder<> *builder):
    S(scop), DT(DomTree), SE(ScEv), Builder(builder) {}
};

// Create a new loop.
//
// @param Builder The builder used to create the loop.  It also defines the
//                place where to create the loop.
// @param UB      The upper bound of the loop iv.
// @param Stride  The number by which the loop iv is incremented after every
//                iteration.
void createLoop(IRBuilder<> *Builder, Value *LB, Value *UB, APInt Stride,
                PHINode*& IV, BasicBlock*& AfterBB, Value*& IncrementedIV,
                DominatorTree *DT) {
  Function *F = Builder->GetInsertBlock()->getParent();
  LLVMContext &Context = F->getContext();

  BasicBlock *PreheaderBB = Builder->GetInsertBlock();
  BasicBlock *HeaderBB = BasicBlock::Create(Context, "polly.loop_header", F);
  BasicBlock *BodyBB = BasicBlock::Create(Context, "polly.loop_body", F);
  AfterBB = BasicBlock::Create(Context, "polly.after_loop", F);

  Builder->CreateBr(HeaderBB);
  DT->addNewBlock(HeaderBB, PreheaderBB);

  Builder->SetInsertPoint(BodyBB);

  Builder->SetInsertPoint(HeaderBB);

  // Use the type of UB, because IV will compare to UB.
  const IntegerType *LoopIVType = dyn_cast<IntegerType>(UB->getType());
  assert(LoopIVType && "UB is not integer?");

  // IV
  IV = Builder->CreatePHI(LoopIVType, "polly.loopiv");
  IV->addIncoming(LB, PreheaderBB);

  // IV increment.
  Stride.zext(64);
  Value *StrideValue = ConstantInt::get(Context, Stride);
  IncrementedIV = Builder->CreateAdd(IV, StrideValue, "polly.next_loopiv");

  // Exit condition.
  Value *CMP = Builder->CreateICmpSLE(IV, UB);
  Builder->CreateCondBr(CMP, BodyBB, AfterBB);
  DT->addNewBlock(BodyBB, HeaderBB);
  DT->addNewBlock(AfterBB, HeaderBB);

  Builder->SetInsertPoint(BodyBB);
}

// Insert a copy of a basic block in the newly generated code.
//
// @param Builder The builder used to insert the code. It also specifies
//                where to insert the code.
// @param BB      The basic block to copy
// @param VMap    A map returning for any old value its new equivalent. This
//                is used to update the operands of the statements.
//                For new statements a relation old->new is inserted in this
//                map.
//
void copyBB (IRBuilder<> *Builder, BasicBlock *BB, ValueMapT &VMap,
             DominatorTree *DT, const Region *R, ScalarEvolution *SE) {
  Function *F = Builder->GetInsertBlock()->getParent();
  LLVMContext &Context = F->getContext();

  BasicBlock *CopyBB = BasicBlock::Create(Context,
                                          "polly.stmt_" + BB->getNameStr(), F);
  Builder->CreateBr(CopyBB);
  DT->addNewBlock(CopyBB, Builder->GetInsertBlock());
  Builder->SetInsertPoint(CopyBB);

  std::vector<const Instruction*> ToCopy;

  ValueMapT BBMap;

  for (BasicBlock::const_iterator II = BB->begin(), IE = BB->end();
       II != IE; ++II)
    if (!II->isTerminator()) {
    const Instruction *Inst = &*II;
    bool Add = true;

    Instruction *NewInst = Inst->clone();

    // Replace old operands with the new ones.
    for (Instruction::op_iterator UI = NewInst->op_begin(),
         UE = NewInst->op_end(); UI != UE; ++UI)
      if (Instruction *OpInst = dyn_cast<Instruction>((*UI).get())) {
        // IVS and Parameters.
        if (VMap.find(OpInst) != VMap.end()) {
          Value *NewOp = VMap[*UI];

          // Insert a cast if the types differ.
          if ((*UI)->getType()->getScalarSizeInBits()
              < VMap[*UI]->getType()->getScalarSizeInBits())
            NewOp = Builder->CreateTruncOrBitCast(NewOp, (*UI)->getType());

          NewInst->replaceUsesOfWith(*UI, NewOp);

        // Instructions calculated in the current BB.
        } else if (BBMap.find(OpInst) != BBMap.end()) {
          Value *NewOp = BBMap[*UI];
          NewInst->replaceUsesOfWith(*UI, NewOp);

        // Ignore instructions that are referencing ops in the old BB. These
        // instructions are unused. They where replace by new ones during
        // createIndependentBlocks().
        } else if (R->contains(OpInst->getParent()))
          Add = false;
      }

    if (Add) {
      Builder->Insert(NewInst);
      BBMap[Inst] = NewInst;
    } else
      delete NewInst;

  }
}

class ClastExpCodeGen {
  IRBuilder<> *Builder;
  CharMapT *IVS;

  Value *print(clast_name *e) {
    CharMapT::iterator I = IVS->find(e->name);

    if (I != IVS->end())
      return I->second;
    else
      llvm_unreachable("Value not found");
  }

  Value *print(clast_term *e) {
    APInt a = APInt_from_MPZ(e->val);
    a.zext(64);
    Value *ConstOne = ConstantInt::get(Builder->getContext(), a);

    if (e->var) {
      Value *var = print(e->var);
      ConstOne = Builder->CreateMul(ConstOne, var);
    }

    return ConstOne;
  }

  Value *print(clast_binary *e) {
    Value *LHS = print(e->LHS);

    APInt RHS_AP = APInt_from_MPZ(e->RHS);
    RHS_AP.zext(64);
    Value *RHS = ConstantInt::get(Builder->getContext(), RHS_AP);

    Value *Result;

    switch (e->type) {
    case clast_bin_mod:
      Result = Builder->CreateSRem(LHS, RHS);
      break;
    case clast_bin_fdiv:
      {
        // floord(n,d) ((n < 0) ? (n - d + 1) : n) / d
        Value *One = ConstantInt::get(Builder->getInt64Ty(), 1);
        Value *Zero = ConstantInt::get(Builder->getInt64Ty(), 1);
        Value *Sum1 = Builder->CreateSub(LHS, RHS);
        Value *Sum2 = Builder->CreateAdd(Sum1, One);
        Value *isNegative = Builder->CreateICmpSLT(LHS, Zero);
        Value *Dividend = Builder->CreateSelect(isNegative, Sum2, LHS);
        Result = Builder->CreateSDiv(Dividend, RHS);
        break;
      }
    case clast_bin_cdiv:
      {
        // ceild(n,d) ((n < 0) ? n : (n + d - 1)) / d
        Value *One =  ConstantInt::get(Builder->getInt64Ty(), 1);
        Value *Zero =  ConstantInt::get(Builder->getInt64Ty(), 1);
        Value *Sum1 = Builder->CreateAdd(LHS, RHS);
        Value *Sum2 = Builder->CreateSub(Sum1, One);
        Value *isNegative = Builder->CreateICmpSLT(LHS, Zero);
        Value *Dividend = Builder->CreateSelect(isNegative, LHS, Sum2);
        Result = Builder->CreateSDiv(Dividend, RHS);
        break;
      }
    case clast_bin_div:
      Result = Builder->CreateSDiv(LHS, RHS);
      break;
    default:
      llvm_unreachable("Unknown binary expression type");
    };

    return Result;
  }

  Value *redmin(Value *old, Value *exprValue, IRBuilder<> *Builder) {
    Value *cmp = Builder->CreateICmpSLT(old, exprValue);
    return Builder->CreateSelect(cmp, old, exprValue);
  }

  Value *redmax(Value *old, Value *exprValue, IRBuilder<> *Builder) {
    Value *cmp = Builder->CreateICmpSGT(old, exprValue);
    return Builder->CreateSelect(cmp, old, exprValue);
  }

  Value *redsum(Value *old, Value *exprValue, IRBuilder<> *Builder) {
    return Builder->CreateAdd(old, exprValue);
  }

  Value *print(clast_reduction *r) {
    assert((   r->type == clast_red_min
               || r->type == clast_red_max
               || r->type == clast_red_sum)
           && "Reduction type not supported");
    Value *old = print(r->elts[0]);

    for (int i=1; i < r->n; ++i) {
      Value *exprValue = print(r->elts[i]);
      if (r->type == clast_red_min)
        old = redmin(old, exprValue, Builder);
      else if (r->type == clast_red_max)
        old = redmax(old, exprValue, Builder);
      else if (r->type == clast_red_sum)
        old = redsum(old, exprValue, Builder);
    }

    return old;
  }

public:

  ClastExpCodeGen(IRBuilder<> *B, CharMapT *IVMap) : Builder(B), IVS(IVMap) {}

  Value *print(clast_expr *e) {
    switch(e->type) {
      case clast_expr_name:
	return print((struct clast_name *)e);
      case clast_expr_term:
	return print((struct clast_term *)e);
      case clast_expr_bin:
	return print((struct clast_binary *)e);
      case clast_expr_red:
	return print((struct clast_reduction *)e);
      default:
        llvm_unreachable("Unknown clast expression!");
    }
  }
};

class CPCodeGenerationActions : public CPActions {
  protected:
  void print(struct clast_assignment *a, codegenctx *ctx) {
    switch(ctx->dir) {
      case DFS_IN:
        {
          eval(a->RHS, ctx);
          Value *RHS = ctx->exprValue;

          if (!a->LHS) {
            PHINode *PN = ctx->stmt->IVS[ctx->assignmentCount];
            ctx->assignmentCount++;
            Value *V = PN;

            if (PN->getNumOperands() == 2)
              V = *(PN->use_begin());

            ctx->ValueMap[V] = RHS;
          }
          break;
        }
      case DFS_OUT:
	break;
    }
  }

  void print(struct clast_user_stmt *u, codegenctx *ctx) {
    // Actually we have a list of pointers here. be careful.
    SCoPStmt *stmt = (SCoPStmt *)u->statement->usr;
    BasicBlock *BB = stmt->getBasicBlock();
    switch(ctx->dir) {
      case DFS_IN:
        ctx->stmt = stmt;
        ctx->assignmentCount = 0;
	break;
      case DFS_OUT:
        copyBB(ctx->Builder, BB, ctx->ValueMap, ctx->DT, &ctx->S->getRegion(),
               ctx->SE);
	break;
    }
  }

  void print(struct clast_block *b, codegenctx *ctx) {
    switch(ctx->dir) {
      case DFS_IN:
	break;
      case DFS_OUT:
	break;
    }
  }

  void print(struct clast_for *f, codegenctx *ctx) {
    IRBuilder<> *Builder = ctx->Builder;

    switch(ctx->dir) {
      case DFS_IN: {
	APInt stride = APInt_from_MPZ(f->stride);
        PHINode *IV;
        Value *IncrementedIV;
        BasicBlock *AfterBB;
	eval(f->LB, ctx);
        Value *LB = ctx->exprValue;
	eval(f->UB, ctx);
        Value *UB = ctx->exprValue;
        createLoop(Builder, LB, UB, stride, IV, AfterBB, IncrementedIV,
                   ctx->DT);
        (ctx->CharMap)[f->iterator] = IV;

        ctx->LoopIncrementedIVs.push_back(IncrementedIV);
        ctx->LoopAfterBBs.push_back(AfterBB);
        ctx->LoopIVs.push_back(IV);
	break;
      }
      case DFS_OUT:
        BasicBlock *AfterBB = ctx->LoopAfterBBs.back();
        BasicBlock *HeaderBB = *pred_begin(AfterBB);
        BasicBlock *LastBodyBB = Builder->GetInsertBlock();
        Value *IncrementedIV = ctx->LoopIncrementedIVs.back();
        PHINode *IV = ctx->LoopIVs.back();

        Builder->CreateBr(HeaderBB);
        IV->addIncoming(IncrementedIV, LastBodyBB);
        Builder->SetInsertPoint(AfterBB);

        ctx->LoopIncrementedIVs.pop_back();
        ctx->LoopIVs.pop_back();
        ctx->LoopAfterBBs.pop_back();
	break;
    }
  }

  void print(struct clast_equation *eq, codegenctx *ctx) {
    eval(eq->LHS, ctx);
    Value *LHS = ctx->exprValue;
    eval(eq->RHS, ctx);
    Value *RHS = ctx->exprValue;
    Value *Result;
    CmpInst::Predicate P;

    if (eq->sign == 0)
      P = ICmpInst::ICMP_EQ;
    else if (eq->sign > 0)
      P = ICmpInst::ICMP_SGE;
    else
      P = ICmpInst::ICMP_SLE;

    Result = ctx->Builder->CreateICmp(P, LHS, RHS);

    ctx->exprValue = Result;
  }

  void print(struct clast_guard *g, codegenctx *ctx) {
    IRBuilder<> *Builder = ctx->Builder;

    switch(ctx->dir) {
      case DFS_IN:
        {
          Function *F = Builder->GetInsertBlock()->getParent();
          LLVMContext &Context = F->getContext();
          BasicBlock *ThenBB = BasicBlock::Create(Context, "polly.then", F);
          BasicBlock *MergeBB = BasicBlock::Create(Context, "polly.merge", F);
          ctx->DT->addNewBlock(ThenBB, Builder->GetInsertBlock());
          ctx->DT->addNewBlock(MergeBB, Builder->GetInsertBlock());

          print(&(g->eq[0]), ctx);
          Value *Predicate = ctx->exprValue;

          for (int i = 1; i < g->n; ++i) {
            Value *TmpPredicate;
            print(&(g->eq[i]), ctx);
            TmpPredicate = ctx->exprValue;
            Predicate = Builder->CreateAnd(Predicate, TmpPredicate);
          }

          Builder->CreateCondBr(Predicate, ThenBB, MergeBB);
          Builder->SetInsertPoint(ThenBB);
          ctx->GuardMergeBBs.push_back(MergeBB);
          break;
        }
      case DFS_OUT:
        Builder->CreateBr(ctx->GuardMergeBBs.back());
        Builder->SetInsertPoint(ctx->GuardMergeBBs.back());
        ctx->GuardMergeBBs.pop_back();
	break;
    }
  }

  void print(struct clast_root *r, codegenctx *ctx) {}

  void print(clast_stmt *stmt, codegenctx *ctx) {
    if	    (CLAST_STMT_IS_A(stmt, stmt_root))
      print((struct clast_root *)stmt, ctx);
    else if (CLAST_STMT_IS_A(stmt, stmt_ass))
      print((struct clast_assignment *)stmt, ctx);
    else if (CLAST_STMT_IS_A(stmt, stmt_user))
      print((struct clast_user_stmt *)stmt, ctx);
    else if (CLAST_STMT_IS_A(stmt, stmt_block))
      print((struct clast_block *)stmt, ctx);
    else if (CLAST_STMT_IS_A(stmt, stmt_for))
      print((struct clast_for *)stmt, ctx);
    else if (CLAST_STMT_IS_A(stmt, stmt_guard))
      print((struct clast_guard *)stmt, ctx);
  }

  public:
  CPCodeGenerationActions(raw_ostream& strm) {
  }

  virtual void in(clast_stmt *s, int depth, cp_ctx *ctx) {
    ctx->dir = DFS_IN;
    print(s, static_cast<codegenctx*>(ctx));
  }

  virtual void out(clast_stmt *s, int depth, cp_ctx *ctx) {
    ctx->dir = DFS_OUT;
    print(s, static_cast<codegenctx*>(ctx));
  }

  virtual void in(clast_expr *e, cp_ctx *ctx) {
  }

  virtual void out(clast_expr *e, cp_ctx *ctx) {
    ctx->dir = DFS_OUT;
    codegenctx *pctx = static_cast<codegenctx*>(ctx);
    ClastExpCodeGen ExpGen(pctx->Builder, &pctx->CharMap);
    pctx->exprValue = ExpGen.print(e);
  }
};
}

namespace {
class ClastCodeGeneration : public RegionPass {
  Region *region;
  SCoP *S;
  DominatorTree *DT;
  ScalarEvolution *SE;
  CLooG *C;

  public:
  static char ID;

  ClastCodeGeneration() : RegionPass(&ID) {
    C = 0;
  }

  void createSeSeEdges(Region *R) {
    BasicBlock *newEntry = createSingleEntryEdge(R, this);

    for (SCoP::iterator SI = S->begin(), SE = S->end(); SI != SE; ++SI)
      if ((*SI)->getBasicBlock() == R->getEntry())
        (*SI)->setBasicBlock(newEntry);

    createSingleExitEdge(R, this);
  }

  void addParameters(CloogNames *names, CharMapT &VariableMap) {
    int i = 0;

    for (SCoP::param_iterator PI = S->param_begin(), PE = S->param_end();
         PI != PE; ++PI) {
      llvm_unreachable("SCoPs with parameters cannot be code generated");
      assert(i < names->nb_parameters && "Not enough parameter names");
      // VariableMap[names->parameters[i]] = *PI;
      ++i;
    }
  }

  // Create new code for every instruction operator that can be expressed by a
  // SCEV.  Like this there are just two types of instructions left:
  //
  // 1. Instructions that only reference loop ivs or parameters outside the
  // region.
  //
  // 2. Instructions that are not used for any memory modification. (These
  //    will be ignored later on.)
  //
  // Blocks containing only these kind of instructions are called independent
  // blocks as they can be scheduled arbitrarily.
  void createIndependentBlocks(BasicBlock *BB) {
    std::vector<Instruction*> work;
    SCEVExpander Rewriter(*SE);

    for (BasicBlock::iterator II = BB->begin(), IE = BB->end();
         II != IE; ++II)
      if (!II->isTerminator()) {
        Instruction *Inst = &*II;

        if (!SE->isSCEVable(Inst->getType()))
          work.push_back(Inst);
      }

    while (work.size() != 0) {
      Instruction *Inst = work.back();
      work.pop_back();

      for (Instruction::op_iterator UI = Inst->op_begin(),
           UE = Inst->op_end(); UI != UE; ++UI) {
        if (!SE->isSCEVable(UI->get()->getType()))
          continue;

        const SCEV *Scev = SE->getSCEV(*UI);
        Value *V = Rewriter.expandCodeFor(Scev, UI->get()->getType(),
                                          &*BB->begin());
        UI->set(V);
      }
    }
  }

  void createIndependentBlocks(SCoP *S) {
    for (SCoP::iterator SI = S->begin(), SE = S->end(); SI != SE; ++SI)
      createIndependentBlocks((*SI)->getBasicBlock());
  }

  bool runOnRegion(Region *R, RGPassManager &RGM) {
    region = R;
    S = getAnalysis<SCoPInfo>().getSCoP();
    DT = &getAnalysis<DominatorTree>();
    SE = &getAnalysis<ScalarEvolution>();

    if (!S) {
      C = 0;
      return false;
    }

    // Parameters not yet supported.
    if (std::distance(S->param_begin(), S->param_end()) > 1 ) {
      errs() << "Code generation for SCoP " << S->getRegion().getNameStr()
        << " failed. Parameters not yet supported.";
      return false;
    }

    createSeSeEdges(R);
    createIndependentBlocks(S);

    if (C)
      delete(C);

    C = new CLooG(S);
    C->generate();

    CPCodeGenerationActions cpa = CPCodeGenerationActions(dbgs());
    ClastParser cp = ClastParser(cpa);

    Function *F = R->getEntry()->getParent();

    // Create a basic block in which to start code generation.
    BasicBlock *PollyBB = BasicBlock::Create(F->getContext(), "pollyBB", F);
    IRBuilder<> Builder(PollyBB);
    DT->addNewBlock(PollyBB, R->getEntry());

    codegenctx ctx (S, DT, SE, &Builder);
    clast_stmt *clast = C->getClast();

    addParameters(((clast_root*)clast)->names, ctx.CharMap);

    cp.parse(clast, &ctx);
    Builder.CreateBr(R->getExit());

    // Update old PHI nodes to pass LLVM verification.
    for (BasicBlock::iterator SI = succ_begin(R->getEntry())->begin(),
         SE = succ_begin(R->getEntry())->end(); SI != SE; ++SI)
      if (PHINode::classof(&(*SI))) {
        PHINode *PN = static_cast<PHINode *>(&*SI);
        PN->removeIncomingValue(R->getEntry());
      }

    if (DT->dominates(R->getEntry(), R->getExit()))
      DT->changeImmediateDominator(R->getExit(), Builder.GetInsertBlock());

    // Enable the new polly code.
    R->getEntry()->getTerminator()->setSuccessor(0, PollyBB);

    return false;
  }

  void print(raw_ostream &OS, const Module *) const {
    if (C)
      C->pprint();
  }

  virtual void getAnalysisUsage(AnalysisUsage &AU) const {
    AU.addRequired<SCoPInfo>();
    AU.addRequired<DominatorTree>();
    AU.addRequired<ScalarEvolution>();
    AU.addPreserved<DominatorTree>();
    AU.addPreserved<SCoPInfo>();
    AU.addPreserved<RegionInfo>();
    AU.setPreservesAll();
  }
};
}

char ClastCodeGeneration::ID = 1;

static RegisterPass<ClastCodeGeneration>
Z("polly-codegen", "Polly - Create LLVM-IR from the polyhedral "
                         "information");

RegionPass* polly::createClastCodeGenerationPass() {
	return new ClastCodeGeneration();
}
