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

struct codegenctx : cp_ctx {
  //=== Code generation global state -------//

  // The open merge basic blocks of Guards/Conditions.
  std::vector<BasicBlock*> GuardMergeBBs;

  std::vector<BasicBlock*> ab;
  ValueMapT ValueMap;
  std::vector<PHINode*> loop_ivs;
  std::vector<Value*> NV;
  Value *exprValue;
  CharMapT CharMap;
  SCoP *S;
  SCoPStmt *stmt;
  BasicBlock *BB;
  unsigned assignmentCount;
  IRBuilder<> *B;
  codegenctx(SCoP *scop, IRBuilder<> *builder):
   S(scop), B(builder) {}
};

// Create a new loop.
//
// @param Builder The builder used to create the loop.  It also defines the
//                place where to create the loop.
// @param UB      The upper bound of the loop iv.
// @param Stride  The number by which the loop iv is incremented after every
//                iteration.
void createLoop(IRBuilder<> *Builder, Value *LB, Value *UB, APInt Stride,
                PHINode **IV, BasicBlock** AB, Value** NV) {
  Function *F = Builder->GetInsertBlock()->getParent();
  LLVMContext &Context = F->getContext();

  BasicBlock *PreheaderBB = Builder->GetInsertBlock();
  BasicBlock *HeaderBB = BasicBlock::Create(Context, "polly.loop_header", F);
  BasicBlock *BodyBB = BasicBlock::Create(Context, "polly.loop_body", F);
  BasicBlock *AfterBB = BasicBlock::Create(Context, "polly.after_loop", F);

  Builder->CreateBr(HeaderBB);

  Builder->SetInsertPoint(BodyBB);

  Builder->SetInsertPoint(HeaderBB);

  // Use the type of UB, because IV will compare to UB.
  const IntegerType *LoopIVType = dyn_cast<IntegerType>(UB->getType());
  assert(LoopIVType && "UB is not integer?");

  // IV
  PHINode *Variable = Builder->CreatePHI(LoopIVType, "polly.loopiv");
  Variable->addIncoming(LB, PreheaderBB);

  // IV increment.
  Stride.zext(64);
  Value *StrideValue = ConstantInt::get(Context, Stride);
  Value *NextVar = Builder->CreateAdd(Variable, StrideValue,
                                      "polly.next_loopiv");

  // Exit condition.
  Value *CMP = Builder->CreateICmpSLE(Variable, UB);
  Builder->CreateCondBr(CMP, BodyBB, AfterBB);

  //Variable->addIncoming(NextVar, BodyBB);
  Builder->SetInsertPoint(BodyBB);

  *IV = Variable;
  *AB = AfterBB;
  *NV = NextVar;
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
void copyBB (IRBuilder<> *Builder, BasicBlock *BB, ValueMapT &VMap) {
  Function *F = Builder->GetInsertBlock()->getParent();
  LLVMContext &Context = F->getContext();

  BasicBlock *CopyBB = BasicBlock::Create(Context,
                                          "polly.stmt_" + BB->getNameStr(), F);
  Builder->CreateBr(CopyBB);
  Builder->SetInsertPoint(CopyBB);

  for (BasicBlock::const_iterator II = BB->begin(), IE = BB->end();
       II != IE; ++II) {
    if (II->isTerminator() || dyn_cast<PHINode>(II))
      continue;

    Instruction *NewInst = II->clone();
    VMap[II] = NewInst;

    // Replace old operands with the new ones.
    for (Instruction::op_iterator UI = NewInst->op_begin(),
         UE = NewInst->op_end(); UI != UE; ++UI)
      if (VMap.find(*UI) != VMap.end()) {
        Value *NewOp = VMap[*UI];

        // Insert a cast if the types differ.
        if ((*UI)->getType()->getScalarSizeInBits()
            < VMap[*UI]->getType()->getScalarSizeInBits())
          NewOp = Builder->CreateTruncOrBitCast(NewOp, (*UI)->getType());

        NewInst->replaceUsesOfWith(*UI, NewOp);
      }

    Builder->Insert(NewInst);
  }
}

class CPCodeGenerationActions : public CPActions {
  protected:
  void print(struct clast_assignment *a, codegenctx *ctx) {
    switch(ctx->dir) {
      case DFS_IN:
	eval(a->RHS, ctx);

        if (!a->LHS) {
          PHINode *PN = ctx->stmt->IVS[ctx->assignmentCount];
          ctx->assignmentCount++;
          Value *V = PN;
          if (PN->getNumOperands() == 2)
            V = *(PN->use_begin());
          ctx->ValueMap[V] = ctx->exprValue;
        }
	break;
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
        ctx->BB = BB;
        ctx->stmt = stmt;
        ctx->assignmentCount = 0;
	break;
      case DFS_OUT:
        copyBB(ctx->B, BB, ctx->ValueMap);
        ctx->BB = 0;
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
    switch(ctx->dir) {
      case DFS_IN: {
	APInt stride = APInt_from_MPZ(f->stride);
        PHINode *IV;
        Value *NV;
        BasicBlock *AB;
	eval(f->LB, ctx);
        Value *LB = ctx->exprValue;
	eval(f->UB, ctx);
        Value *UB = ctx->exprValue;
        createLoop(ctx->B, LB, UB, stride, &IV, &AB, &NV);
        (ctx->CharMap)[f->iterator] = IV;
        ctx->NV.push_back(NV);
        ctx->ab.push_back(AB);
        ctx->loop_ivs.push_back(IV);
	break;
      }
      case DFS_OUT:
        ctx->B->CreateBr(*pred_begin(ctx->ab.back()));
        ctx->loop_ivs.back()->addIncoming(ctx->NV.back(),
                                          ctx->B->GetInsertBlock());
        ctx->B->SetInsertPoint(ctx->ab.back());
        ctx->loop_ivs.pop_back();
        ctx->ab.pop_back();
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

    Result = ctx->B->CreateICmp(P, LHS, RHS);

    ctx->exprValue = Result;
  }

  void print(struct clast_guard *g, codegenctx *ctx) {
    IRBuilder<> *Builder = ctx->B;

    switch(ctx->dir) {
      case DFS_IN:
        {
          Function *F = Builder->GetInsertBlock()->getParent();
          LLVMContext &Context = F->getContext();
          BasicBlock *ThenBB = BasicBlock::Create(Context, "polly.then", F);
          BasicBlock *MergeBB = BasicBlock::Create(Context, "polly.merge", F);

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

  void print(clast_name *e, codegenctx *ctx) {
    if(ctx->dir == DFS_IN) {
      CharMapT::iterator I = ctx->CharMap.find(e->name);

      if (I != ctx->CharMap.end())
        ctx->exprValue = I->second;
      else
        llvm_unreachable("Value not found");
    }
  }

  void print(clast_term *e, codegenctx *ctx) {
    APInt a = APInt_from_MPZ(e->val);
    if (ctx->dir == DFS_IN) {
      a.zext(64);
      Value *ConstOne = ConstantInt::get(ctx->B->getContext(), a);
      ctx->exprValue = ConstOne;
    }
  }

  void print(clast_binary *e, codegenctx *ctx) {
    switch(ctx->dir) {
    case DFS_IN:
      {
        eval(e->LHS, ctx);
        Value *LHS = ctx->exprValue;

        APInt RHS_AP = APInt_from_MPZ(e->RHS);
        RHS_AP.zext(64);
        Value *RHS = ConstantInt::get(ctx->B->getContext(), RHS_AP);

        Value *Result;

        switch (e->type) {
        case clast_bin_mod:
          Result = ctx->B->CreateURem(LHS, RHS);
          llvm_unreachable("mod binary expression not supported");
          break;
        case clast_bin_fdiv:
          llvm_unreachable("fdiv binary expression not supported");
          break;
        case clast_bin_cdiv:
          llvm_unreachable("cdiv binary expression not supported");
          break;
        case clast_bin_div:
          llvm_unreachable("div binary expression not supported");
          break;
        default:
          llvm_unreachable("Unknown binary expression type");
        };

        ctx->exprValue = Result;
        break;
      }
      case DFS_OUT:
	break;
    }
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

  void print(clast_reduction *r, codegenctx *ctx) {
    if (ctx->dir == DFS_IN) {
      assert((   r->type == clast_red_min
              || r->type == clast_red_max
              || r->type == clast_red_sum)
             && "Reduction type not supported");
      eval(r->elts[0], ctx);

      Value *old = ctx->exprValue;

      // TODO: Support reductions with more than one element.
      for (int i=1; i < r->n; ++i) {
        eval(r->elts[i], ctx);
        if (r->type == clast_red_min)
          old = redmin(old, ctx->exprValue, ctx->B);
        else if (r->type == clast_red_max)
          old = redmax(old, ctx->exprValue, ctx->B);
        else if (r->type == clast_red_sum)
          old = redsum(old, ctx->exprValue, ctx->B);
      }

      ctx->exprValue = old;
    }
  }

  void print(clast_expr *e, codegenctx *ctx) {
    switch(e->type) {
      case clast_expr_name:
	print((struct clast_name *)e, ctx);
	break;
      case clast_expr_term:
	print((struct clast_term *)e, ctx);
	break;
      case clast_expr_bin:
	print((struct clast_binary *)e, ctx);
	break;
      case clast_expr_red:
	print((struct clast_reduction *)e, ctx);
	break;
    }
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
    ctx->dir = DFS_IN;
    print(e, static_cast<codegenctx*>(ctx));
  }

  virtual void out(clast_expr *e, cp_ctx *ctx) {
    ctx->dir = DFS_OUT;
    print(e, static_cast<codegenctx*>(ctx));
  }
};
}

namespace {
class ClastCodeGeneration : public RegionPass {
  Region *region;
  SCoP *S;
  DominatorTree *DT;
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

  bool runOnRegion(Region *R, RGPassManager &RGM) {
    region = R;
    S = getAnalysis<SCoPInfo>().getSCoP();
    DT = &getAnalysis<DominatorTree>();

    if (!S) {
      C = 0;
      return false;
    }

    createSeSeEdges(R);

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

    codegenctx ctx (S, &Builder);
    cp.parse(C->getClast(), &ctx);
    Builder.CreateBr(R->getExit());

    // Update old PHI nodes to pass LLVM verification.
    for (BasicBlock::iterator SI = succ_begin(R->getEntry())->begin(),
         SE = succ_begin(R->getEntry())->end(); SI != SE; ++SI)
      if (PHINode::classof(&(*SI))) {
        PHINode *PN = static_cast<PHINode *>(&*SI);
        PN->removeIncomingValue(R->getEntry());
      }

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
