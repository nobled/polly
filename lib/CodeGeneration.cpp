//===------ CodeGeneration.cpp - Code generate the Scops. -----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// The CodeGeneration pass takes a Scop created by ScopInfo and translates it
// back to LLVM-IR using Cloog.
//
// The Scop describes the high level memory behaviour of a control flow region.
// Transformation passes can update the schedule (execution order) of statements
// in the Scop. Cloog is used to generate an abstract syntax tree (clast) that
// reflects the updated execution order. This clast is used to create new
// LLVM-IR that is computational equivalent to the original control flow region,
// but executes its code in the new execution order defined by the changed
// scattering.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "polly-codegen"

#include "polly/LinkAllPasses.h"
#include "polly/Support/GmpConv.h"
#include "polly/Support/ScopHelper.h"
#include "polly/Cloog.h"
#include "polly/Dependences.h"
#include "polly/ScopInfo.h"
#include "polly/TempScopInfo.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/IRBuilder.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/ScalarEvolutionExpander.h"
#include "llvm/Module.h"

#define CLOOG_INT_GMP 1
#include "cloog/cloog.h"
#include "cloog/isl/cloog.h"

#include <vector>
#include <utility>

using namespace polly;
using namespace llvm;

namespace polly {

static cl::opt<bool>
Vector("enable-polly-vector",
       cl::desc("Enable polly vector code generation"), cl::Hidden,
       cl::value_desc("Vector code generation enabled if true"),
       cl::init(false));

static cl::opt<bool>
OpenMP("enable-polly-openmp",
       cl::desc("Generate OpenMP parallel code"), cl::Hidden,
       cl::value_desc("OpenMP code generation enabled if true"),
       cl::init(false));

static cl::opt<std::string>
CodegenOnly("polly-codegen-only",
            cl::desc("Codegen only this function"), cl::Hidden,
            cl::value_desc("The function name to codegen"),
            cl::ValueRequired, cl::init(""));

typedef DenseMap<const Value*, Value*> ValueMapT;
typedef DenseMap<const char*, Value*> CharMapT;
typedef std::vector<ValueMapT> VectorValueMapT;

// Create a new loop.
//
// @param Builder The builder used to create the loop.  It also defines the
//                place where to create the loop.
// @param UB      The upper bound of the loop iv.
// @param Stride  The number by which the loop iv is incremented after every
//                iteration.
static void createLoop(IRBuilder<> *Builder, Value *LB, Value *UB, APInt Stride,
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

  // Use the type of upper and lower bound.
  assert(LB->getType() == UB->getType()
         && "Different types for upper and lower bound.");

  const IntegerType *LoopIVType = dyn_cast<IntegerType>(UB->getType());
  assert(LoopIVType && "UB is not integer?");

  // IV
  IV = Builder->CreatePHI(LoopIVType, "polly.loopiv");
  IV->addIncoming(LB, PreheaderBB);

  // IV increment.
  Stride.zext(LoopIVType->getBitWidth());
  Value *StrideValue = ConstantInt::get(Context, Stride);
  IncrementedIV = Builder->CreateAdd(IV, StrideValue, "polly.next_loopiv");

  // Exit condition.
  Value *CMP = Builder->CreateICmpSLE(IV, UB);
  Builder->CreateCondBr(CMP, BodyBB, AfterBB);
  DT->addNewBlock(BodyBB, HeaderBB);
  DT->addNewBlock(AfterBB, HeaderBB);

  Builder->SetInsertPoint(BodyBB);
}

class BlockGenerator {
  IRBuilder<> &Builder;
  ValueMapT &VMap;
  VectorValueMapT &ValueMaps;
  Scop &S;
  ScopStmt &statement;

public:
  BlockGenerator(IRBuilder<> &B, ValueMapT &vmap, VectorValueMapT &vmaps,
                 ScopStmt &Stmt)
    : Builder(B), VMap(vmap), ValueMaps(vmaps), S(*Stmt.getParent()),
    statement(Stmt) {}

  const Region &getRegion() {
    return S.getRegion();
  }

  Value* getOperand(const Value *OldOperand, ValueMapT &BBMap,
                    ValueMapT *VectorMap = 0) {
    const Instruction *OpInst = dyn_cast<Instruction>(OldOperand);

    if (!OpInst)
      return const_cast<Value*>(OldOperand);

    if (VectorMap && VectorMap->count(OldOperand))
      return (*VectorMap)[OldOperand];

    // IVS and Parameters.
    if (VMap.count(OldOperand)) {
      Value *NewOperand = VMap[OldOperand];

      // Insert a cast if types are different
      if (OldOperand->getType()->getScalarSizeInBits()
          < NewOperand->getType()->getScalarSizeInBits())
        NewOperand = Builder.CreateTruncOrBitCast(NewOperand,
                                                   OldOperand->getType());

      return NewOperand;
    }

    // Instructions calculated in the current BB.
    if (BBMap.count(OldOperand)) {
      return BBMap[OldOperand];
    }

    // Ignore instructions that are referencing ops in the old BB. These
    // instructions are unused. They where replace by new ones during
    // createIndependentBlocks().
    if (getRegion().contains(OpInst->getParent()))
      return NULL;

    return const_cast<Value*>(OldOperand);
  }

#define VECTORSIZE 4

  const Type *getVectorPtrTy(const Value *V, int vectorSize = VECTORSIZE) {
    const PointerType *pointerType = dyn_cast<PointerType>(V->getType());
    assert(pointerType && "PointerType expected");

    const Type *scalarType = pointerType->getElementType();
    VectorType *vectorType = VectorType::get(scalarType, vectorSize);

    return PointerType::getUnqual(vectorType);
  }

  /// @brief Load a vector from a set of adjacent scalars
  ///
  /// In case a set of scalars is known to be next to each other in memory,
  /// create a vector load that loads those scalars
  ///
  /// %vector_ptr= bitcast double* %p to <4 x double>*
  /// %vec_full = load <4 x double>* %vector_ptr
  ///
  Value *generateFullVectorLoad(const LoadInst *load, ValueMapT &BBMap,
                                int size = VECTORSIZE) {
    const Value *pointer = load->getPointerOperand();
    const Type *vectorPtrType = getVectorPtrTy(pointer, size);
    Value *newPointer = getOperand(pointer, BBMap);
    Value *VectorPtr = Builder.CreateBitCast(newPointer, vectorPtrType,
                                             "vector_ptr");
    Value *VecLoad = Builder.CreateLoad(VectorPtr,
                                        load->getNameStr()
                                        + "_p_vec_full");
    return VecLoad;
  }

  /// @brief Load a vector initialized from a single scalar in memory
  ///
  /// In case all elements of a vector are initialized to the same
  /// scalar value, this value is loaded and shuffeled into all elements
  /// of the vector.
  ///
  /// %splat_one = load <1 x double>* %p
  /// %splat = shufflevector <1 x double> %splat_one, <1 x
  ///       double> %splat_one, <4 x i32> zeroinitializer
  ///
  Value *generateSplatVectorLoad(const LoadInst *load, ValueMapT &BBMap,
                                 int size = VECTORSIZE) {
    const Value *pointer = load->getPointerOperand();
    const Type *vectorPtrType = getVectorPtrTy(pointer, 1);
    Value *newPointer = getOperand(pointer, BBMap);
    Value *vectorPtr = Builder.CreateBitCast(newPointer, vectorPtrType,
                                             load->getNameStr() + "_p_vec_p");
    Value *scalarLoad= Builder.CreateLoad(vectorPtr,
                                          load->getNameStr() + "_p_splat_one");
    std::vector<Constant*> splat;

    for (int i = 0; i < size; i++)
      splat.push_back (Builder.getInt32(0));

    Constant *splatVector = ConstantVector::get(splat);

    Value *vectorLoad = Builder.CreateShuffleVector(scalarLoad, scalarLoad,
                                                    splatVector,
                                                    load->getNameStr()
                                                    + "_p_splat");
    return vectorLoad;
  }

  /// @Load a vector from scalars distributed in memory
  ///
  /// In case some scalars a distributed randomly in memory. Create a vector
  /// by loading each scalar and by inserting one after the other into the
  /// vector.
  ///
  /// %scalar_1= load double* %p_1
  /// %vec_1 = insertelement <2 x double> undef, double %scalar_1, i32 0
  /// %scalar 2 = load double* %p_2
  /// %vec_2 = insertelement <2 x double> %vec_1, double %scalar_1, i32 1
  ///
  Value *generateScalarVectorLoad(const LoadInst *load, ValueMapT &BBMap,
                                  VectorValueMapT &WholeMap,
                                  int size = VECTORSIZE) {
    const Value *pointer = load->getPointerOperand();
    VectorType *vectorType = VectorType::get(
      dyn_cast<PointerType>(pointer->getType())->getElementType(), size);

    Value *vector = UndefValue::get(vectorType);

    for (int i = 0; i < size; i++) {
      Value *newPointer = getOperand(pointer, WholeMap[i]);
      Value *scalarLoad = Builder.CreateLoad(newPointer,
                                             load->getNameStr() + "_p_scalar_");
      vector = Builder.CreateInsertElement(vector, scalarLoad,
                                           Builder.getInt32(i),
                                           load->getNameStr() + "_p_vec_");
    }

    return vector;
  }

  Value *generateScalarLoad(const LoadInst *load, ValueMapT &BBMap) {
    const Value *pointer = load->getPointerOperand();
    Value *newPointer = getOperand(pointer, BBMap);
    Value *scalarLoad = Builder.CreateLoad(newPointer,
                                           load->getNameStr() + "_p_scalar_");
    return scalarLoad;
  }

  /// @brief Load a value (or several values as a vector) from memory.
  void generateLoad(const LoadInst *load, ValueMapT &BBMap,
                    ValueMapT &VectorMap, VectorValueMapT &WholeMap) {

    if (WholeMap.size() == 1) {
      BBMap[load] = generateScalarLoad(load, BBMap);
      return;
    }

    Value *newLoad;

    MemoryAccess &Access = statement.getAccessFor(load);

    if (Access.isConstant(NULL))
      newLoad = generateSplatVectorLoad(load, BBMap);
    else if (Access.isStrideOne(NULL))
      newLoad = generateFullVectorLoad(load, BBMap);
    else
      newLoad = generateScalarVectorLoad(load, BBMap, WholeMap);

    VectorMap[load] = newLoad;
  }

  void copyInstruction(const Instruction *Inst, ValueMapT &BBMap,
                       ValueMapT &VectorMap, VectorValueMapT &WholeMap) {
    if (VectorMap.count(Inst))
      return;

    if (Inst->isTerminator())
      return;

    if (const LoadInst *load = dyn_cast<LoadInst>(Inst)) {
      generateLoad(load, BBMap, VectorMap, WholeMap);
      return;
    }

    if (const BinaryOperator *binaryInst = dyn_cast<BinaryOperator>(Inst)) {
      Value *VectorLHS = getOperand(Inst->getOperand(0), BBMap, &VectorMap);
      Value *VectorRHS = getOperand(Inst->getOperand(1), BBMap, &VectorMap);

      std::string newInstructionName =  Inst->getNameStr() + "_vector";
      Value *newInst = Builder.CreateBinOp(binaryInst->getOpcode(), VectorLHS,
                                        VectorRHS, newInstructionName);
      if (VectorMap.count(Inst->getOperand(0))
          || VectorMap.count(Inst->getOperand(1)))
        VectorMap[Inst] = newInst;
      else
        BBMap[Inst] = newInst;

      return;
    }

    if (const StoreInst *store = dyn_cast<StoreInst>(Inst)) {
      if (VectorMap.count(store->getValueOperand()) > 0) {
        const Value *pointer = store->getPointerOperand();
        const Type *vectorPtrType = getVectorPtrTy(pointer);
        Value *newPointer = getOperand(pointer, BBMap, &VectorMap);

        Value *VectorPtr = Builder.CreateBitCast(newPointer, vectorPtrType,
                                                 "vector_ptr");

        Value *VectorVal = getOperand(store->getValueOperand(), BBMap,
                                      &VectorMap);
        Value *newInst = Builder.CreateStore(VectorVal, VectorPtr);

        if (VectorMap.count(store->getValueOperand()))
          VectorMap[Inst] = newInst;

        return;
      }
    }

    Instruction *NewInst = Inst->clone();

    // Copy the operands in temporary vector, as an in place update
    // fails if an instruction is referencing the same operand twice.
    std::vector<Value*> Operands(NewInst->op_begin(), NewInst->op_end());

    // Replace old operands with the new ones.
    for (std::vector<Value*>::iterator UI = Operands.begin(),
         UE = Operands.end(); UI != UE; ++UI) {
      Value *newOperand = getOperand(*UI, BBMap);

      if (!newOperand) {
        assert(!isa<StoreInst>(NewInst)
               && "Store instructions are always needed!");
        delete NewInst;
        return;
      }

      NewInst->replaceUsesOfWith(*UI, newOperand);
    }

    Builder.Insert(NewInst);
    BBMap[Inst] = NewInst;

    if (!NewInst->getType()->isVoidTy())
      NewInst->setName("p_" + Inst->getName());
  }

  int getVectorSize() {
    return ValueMaps.size();
  }

  bool isVectorBlock() {
    return getVectorSize() > 1;
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
  void copyBB(BasicBlock *BB, DominatorTree *DT) {
    Function *F = Builder.GetInsertBlock()->getParent();
    LLVMContext &Context = F->getContext();
    BasicBlock *CopyBB = BasicBlock::Create(Context,
                                            "polly.stmt_" + BB->getNameStr(),
                                            F);
    Builder.CreateBr(CopyBB);
    DT->addNewBlock(CopyBB, Builder.GetInsertBlock());
    Builder.SetInsertPoint(CopyBB);

    // Create two maps that store the mapping from the original instructions of
    // the old basic block to their copies in the new basic block. Those maps
    // are basic block local.
    //
    // As vector code generation is supported there is one map for scalar values
    // and one for vector values.
    //
    // In case we just do scalar code generation, the vectorMap is not used and
    // the scalarMap has just one dimension, which contains the mapping.
    //
    // In case vector code generation is done, an instruction may either appear
    // in the vector map once (as it is calculating >vectorwidth< values at a
    // time. Or (if the values are calculated using scalar operations), it
    // appears once in every dimension of the scalarMap.
    VectorValueMapT scalarBlockMap(getVectorSize());
    ValueMapT vectorBlockMap;

    for (BasicBlock::const_iterator II = BB->begin(), IE = BB->end();
         II != IE; ++II)
      for (int i = 0; i < getVectorSize(); i++) {
        if (isVectorBlock())
          VMap = ValueMaps[i];

        copyInstruction(II, scalarBlockMap[i], vectorBlockMap,
                        scalarBlockMap);
      }
  }
};

/// Class to generate LLVM-IR that calculates the value of a clast_expr.
class ClastExpCodeGen {
  IRBuilder<> *Builder;
  const CharMapT *IVS;

  Value *codegen(const clast_name *e, const Type *Ty) {
    CharMapT::const_iterator I = IVS->find(e->name);

    if (I != IVS->end())
      return Builder->CreateSExtOrBitCast(I->second, Ty);
    else
      llvm_unreachable("Clast name not found");
  }

  Value *codegen(const clast_term *e, const Type *Ty) {
    APInt a = APInt_from_MPZ(e->val);

    Value *ConstOne = ConstantInt::get(Builder->getContext(), a);
    ConstOne = Builder->CreateSExtOrBitCast(ConstOne, Ty);

    if (e->var) {
      Value *var = codegen(e->var, Ty);
      return Builder->CreateMul(ConstOne, var);
    }

    return ConstOne;
  }

  Value *codegen(const clast_binary *e, const Type *Ty) {
    Value *LHS = codegen(e->LHS, Ty);

    APInt RHS_AP = APInt_from_MPZ(e->RHS);

    Value *RHS = ConstantInt::get(Builder->getContext(), RHS_AP);
    RHS = Builder->CreateSExtOrBitCast(RHS, Ty);

    switch (e->type) {
    case clast_bin_mod:
      return Builder->CreateSRem(LHS, RHS);
    case clast_bin_fdiv:
      {
        // floord(n,d) ((n < 0) ? (n - d + 1) : n) / d
        Value *One = ConstantInt::get(Builder->getInt1Ty(), 1);
        Value *Zero = ConstantInt::get(Builder->getInt1Ty(), 0);
        One = Builder->CreateZExtOrBitCast(One, Ty);
        Zero = Builder->CreateZExtOrBitCast(Zero, Ty);
        Value *Sum1 = Builder->CreateSub(LHS, RHS);
        Value *Sum2 = Builder->CreateAdd(Sum1, One);
        Value *isNegative = Builder->CreateICmpSLT(LHS, Zero);
        Value *Dividend = Builder->CreateSelect(isNegative, Sum2, LHS);
        return Builder->CreateSDiv(Dividend, RHS);
      }
    case clast_bin_cdiv:
      {
        // ceild(n,d) ((n < 0) ? n : (n + d - 1)) / d
        Value *One = ConstantInt::get(Builder->getInt1Ty(), 1);
        Value *Zero = ConstantInt::get(Builder->getInt1Ty(), 0);
        One = Builder->CreateZExtOrBitCast(One, Ty);
        Zero = Builder->CreateZExtOrBitCast(Zero, Ty);
        Value *Sum1 = Builder->CreateAdd(LHS, RHS);
        Value *Sum2 = Builder->CreateSub(Sum1, One);
        Value *isNegative = Builder->CreateICmpSLT(LHS, Zero);
        Value *Dividend = Builder->CreateSelect(isNegative, LHS, Sum2);
        return Builder->CreateSDiv(Dividend, RHS);
      }
    case clast_bin_div:
      return Builder->CreateSDiv(LHS, RHS);
    default:
      llvm_unreachable("Unknown clast binary expression type");
    };
  }

  Value *codegen(const clast_reduction *r, const Type *Ty) {
    assert((   r->type == clast_red_min
            || r->type == clast_red_max
            || r->type == clast_red_sum)
           && "Clast reduction type not supported");
    Value *old = codegen(r->elts[0], Ty);

    for (int i=1; i < r->n; ++i) {
      Value *exprValue = codegen(r->elts[i], Ty);

      switch (r->type) {
      case clast_red_min:
        {
          Value *cmp = Builder->CreateICmpSLT(old, exprValue);
          old = Builder->CreateSelect(cmp, old, exprValue);
          break;
        }
      case clast_red_max:
        {
          Value *cmp = Builder->CreateICmpSGT(old, exprValue);
          old = Builder->CreateSelect(cmp, old, exprValue);
          break;
        }
      case clast_red_sum:
        old = Builder->CreateAdd(old, exprValue);
        break;
      default:
        llvm_unreachable("Clast unknown reduction type");
      }
    }

    return old;
  }

public:

  // A generator for clast expressions.
  //
  // @param B The IRBuilder that defines where the code to calculate the
  //          clast expressions should be inserted.
  // @param IVMAP A Map that translates strings describing the induction
  //              variables to the Values* that represent these variables
  //              on the LLVM side.
  ClastExpCodeGen(IRBuilder<> *B, CharMapT *IVMap) : Builder(B), IVS(IVMap) {}

  // Generates code to calculate a given clast expression.
  //
  // @param e The expression to calculate.
  // @return The Value that holds the result.
  Value *codegen(const clast_expr *e, const Type *Ty) {
    switch(e->type) {
      case clast_expr_name:
	return codegen((const clast_name *)e, Ty);
      case clast_expr_term:
	return codegen((const clast_term *)e, Ty);
      case clast_expr_bin:
	return codegen((const clast_binary *)e, Ty);
      case clast_expr_red:
	return codegen((const clast_reduction *)e, Ty);
      default:
        llvm_unreachable("Unknown clast expression!");
    }
  }

  Value *codegen(const clast_expr *e) {
    return codegen(e, Builder->getInt64Ty());
  }
};

class ClastStmtCodeGen {
  // The Scop we code generate.
  Scop *S;

  DominatorTree *DT;
  Dependences *DP;

  // The Builder specifies the current location to code generate at.
  IRBuilder<> *Builder;

  // Map the Values from the old code to their counterparts in the new code.
  ValueMapT ValueMap;

  // Codegenerator for clast expressions.
  ClastExpCodeGen ExpGen;

  // Do we currently generate parallel code?
  bool parallelCodeGeneration;


public:
  // Map the textual representation of variables in clast to the actual
  // Values* created during code generation.  It is used to map every
  // appearance of such a string in the clast to the value created
  // because of a loop iv or an assignment.
  CharMapT CharMap;

  protected:
  void codegen(const clast_assignment *a) {
    //Value *RHS = ExpGen.codegen(a->RHS);
    assert(false && "Here is something missing");
  }

  void codegen(const clast_assignment *a, ScopStmt *Statement,
               unsigned Dimension, int vectorDim,
               std::vector<ValueMapT> *VectorVMap = 0) {
    Value *RHS = ExpGen.codegen(a->RHS);

    assert(!a->LHS && "Statement assignments do not have left hand side");
    const PHINode *PN;
    PN = Statement->getInductionVariableForDimension(Dimension);
    const Value *V = PN;

    if (PN->getNumOperands() == 2)
      V = *(PN->use_begin());

    if (VectorVMap)
      (*VectorVMap)[vectorDim][V] = RHS;

    ValueMap[V] = RHS;
  }

  void codegenSubstitutions(const clast_stmt *Assignment,
                            ScopStmt *Statement, int vectorDim = 0,
                            std::vector<ValueMapT> *VectorVMap = 0) {
    int Dimension = 0;

    while (Assignment) {
      assert(CLAST_STMT_IS_A(Assignment, stmt_ass)
             && "Substitions are expected to be assignments");
      codegen((const clast_assignment *)Assignment, Statement, Dimension,
              vectorDim, VectorVMap);
      Assignment = Assignment->next;
      Dimension++;
    }
  }

  void codegen(const clast_user_stmt *u, std::vector<Value*> *IVS = NULL,
               const char *iterator = NULL) {
    ScopStmt *Statement = (ScopStmt *)u->statement->usr;
    BasicBlock *BB = Statement->getBasicBlock();

    if (u->substitutions)
      codegenSubstitutions(u->substitutions, Statement);

    int vectorDimensions = IVS ? IVS->size() : 1;

    VectorValueMapT VectorValueMap(vectorDimensions);

    if (IVS) {
      assert (u->substitutions && "Substitutions expected!");
      int i = 0;
      for (std::vector<Value*>::iterator II = IVS->begin(), IE = IVS->end();
           II != IE; ++II) {
        CharMap[iterator] = *II;
        codegenSubstitutions(u->substitutions, Statement, i, &VectorValueMap);
        i++;
      }
    }

    BlockGenerator Generator(*Builder, ValueMap, VectorValueMap, *Statement);
    Generator.copyBB(BB, DT);
  }

  void codegen(const clast_block *b) {
    if (b->body)
      codegen(b->body);
  }

  /// @brief Create a classical sequential loop.
  void codegenForSequential(const clast_for *f) {
    APInt Stride = APInt_from_MPZ(f->stride);
    PHINode *IV;
    Value *IncrementedIV;
    BasicBlock *AfterBB;
    Value *LB = ExpGen.codegen(f->LB);
    Value *UB = ExpGen.codegen(f->UB);
    createLoop(Builder, LB, UB, Stride, IV, AfterBB, IncrementedIV, DT);
    CharMap[f->iterator] = IV;

    if (f->body)
      codegen(f->body);

    BasicBlock *HeaderBB = *pred_begin(AfterBB);
    BasicBlock *LastBodyBB = Builder->GetInsertBlock();
    Builder->CreateBr(HeaderBB);
    IV->addIncoming(IncrementedIV, LastBodyBB);
    Builder->SetInsertPoint(AfterBB);
  }

  /// @brief Check if a loop is parallel
  ///
  /// Detect if a clast_for loop can be executed in parallel.
  ///
  /// @param f The clast for loop to check.
  bool isParallelFor(const clast_for *f) {
    isl_set *loopDomain = isl_set_from_cloog_domain(f->domain);
    assert(loopDomain && "Cannot access domain of loop");

    bool isParallel = DP->isParallelDimension(loopDomain,
                                              isl_set_n_dim(loopDomain) - 1);

    if (isParallel)
      DEBUG(dbgs() << "Parallel loop with induction variable '" << f->iterator
            << "' found\n";);

    return isParallel;
  }

  /// @brief Add a new definition of an openmp subfunction.
  Function* addOpenMPSubfunction(Module *M) {
      LLVMContext &Context = Builder->getContext();

      // Create name for the subfunction.
      Function *F = Builder->GetInsertBlock()->getParent();
      const std::string &Name = F->getNameStr() + ".omp_subfn";

      // Prototype for "subfunction".
      std::vector<const Type*> Arguments(1, Type::getInt8PtrTy(Context));
      FunctionType *FT = FunctionType::get(Type::getVoidTy(Context),
                           Arguments, false);
      // Get unique name for the subfunction.
      Function *FN = Function::Create(FT, Function::ExternalLinkage,
                               Name, M);

      return FN;
  }

  /// @brief Create an OpenMP parallel for loop.
  ///
  /// This loop reflects a loop as if it would have been created by an OpenMP
  /// statement.
  void codegenForOpenMP(const clast_for *f) {
    Module *M = Builder->GetInsertBlock()->getParent()->getParent();

    Function *SubFunction = addOpenMPSubfunction(M);

    // Create call for GOMP_parallel_loop_runtime_start.
    Value *nullArgument = ConstantPointerNull::get(Builder->getInt8PtrTy());
    Value *numberOfThreads = Builder->getInt32(0);
    Value *lowerBound = ExpGen.codegen(f->LB);
    Value *upperBound = ExpGen.codegen(f->UB);
    APInt APStride = APInt_from_MPZ(f->stride);
    const IntegerType *strideType = Builder->getInt64Ty();
    APStride.zext(strideType->getBitWidth());
    Value *stride = ConstantInt::get(strideType, APStride);

    SmallVector<Value *, 6> Arguments;
    Arguments.push_back(SubFunction);
    Arguments.push_back(nullArgument);
    Arguments.push_back(numberOfThreads);
    Arguments.push_back(lowerBound);
    Arguments.push_back(upperBound);
    Arguments.push_back(stride);

    Function *parallelStartFunction = M->getFunction("GOMP_parallel_loop_runtime_start");
    Builder->CreateCall(parallelStartFunction, Arguments.begin(), Arguments.end());

    // Create call to the subfunction.
    Builder->CreateCall(SubFunction, nullArgument);

    // Create call for GOMP_parallel_end.
    Function *FN = M->getFunction("GOMP_parallel_end");
    Builder->CreateCall(FN);
  }

  bool isInnermostLoop(const clast_for *f) {
    return CLAST_STMT_IS_A(f->body, stmt_user);
  }

  /// @brief Create vector instructions for this loop.
  void codegenForVector(const clast_for *f) {
    DEBUG(dbgs() << "Vectorizing loop '" << f->iterator << "'\n";);

    Value *LB = ExpGen.codegen(f->LB);
    APInt Stride = APInt_from_MPZ(f->stride);
    const IntegerType *LoopIVType = dyn_cast<IntegerType>(LB->getType());
    Stride.zext(LoopIVType->getBitWidth());
    Value *StrideValue = ConstantInt::get(Builder->getContext(), Stride);

    std::vector<Value*> IVS(VECTORSIZE);
    IVS[0] = LB;

    for (int i = 1; i < VECTORSIZE; i++)
      IVS[i] = Builder->CreateAdd(IVS[i-1], StrideValue, "p_vector_iv");

    CharMap[f->iterator] = LB;
    codegen((const clast_user_stmt *)f->body, &IVS, f->iterator);

  }

  void codegen(const clast_for *f) {
    if (Vector && isInnermostLoop(f) && isParallelFor(f)) {
      codegenForVector(f);
    } else if (OpenMP && !parallelCodeGeneration && isParallelFor(f)) {
      parallelCodeGeneration = true;
      codegenForOpenMP(f);
      parallelCodeGeneration = false;
    } else
      codegenForSequential(f);
  }

  Value *codegen(const clast_equation *eq) {
    Value *LHS = ExpGen.codegen(eq->LHS);
    Value *RHS = ExpGen.codegen(eq->RHS);
    CmpInst::Predicate P;

    if (eq->sign == 0)
      P = ICmpInst::ICMP_EQ;
    else if (eq->sign > 0)
      P = ICmpInst::ICMP_SGE;
    else
      P = ICmpInst::ICMP_SLE;

    return Builder->CreateICmp(P, LHS, RHS);
  }

  void codegen(const clast_guard *g) {
    Function *F = Builder->GetInsertBlock()->getParent();
    LLVMContext &Context = F->getContext();
    BasicBlock *ThenBB = BasicBlock::Create(Context, "polly.then", F);
    BasicBlock *MergeBB = BasicBlock::Create(Context, "polly.merge", F);
    DT->addNewBlock(ThenBB, Builder->GetInsertBlock());
    DT->addNewBlock(MergeBB, Builder->GetInsertBlock());

    Value *Predicate = codegen(&(g->eq[0]));

    for (int i = 1; i < g->n; ++i) {
      Value *TmpPredicate = codegen(&(g->eq[i]));
      Predicate = Builder->CreateAnd(Predicate, TmpPredicate);
    }

    Builder->CreateCondBr(Predicate, ThenBB, MergeBB);
    Builder->SetInsertPoint(ThenBB);

    codegen(g->then);

    Builder->CreateBr(MergeBB);
    Builder->SetInsertPoint(MergeBB);
  }

  void codegen(const clast_root *r) {
    parallelCodeGeneration = false;
  }

public:
  void codegen(const clast_stmt *stmt) {
    if	    (CLAST_STMT_IS_A(stmt, stmt_root))
      codegen((const clast_root *)stmt);
    else if (CLAST_STMT_IS_A(stmt, stmt_ass))
      codegen((const clast_assignment *)stmt);
    else if (CLAST_STMT_IS_A(stmt, stmt_user))
      codegen((const clast_user_stmt *)stmt);
    else if (CLAST_STMT_IS_A(stmt, stmt_block))
      codegen((const clast_block *)stmt);
    else if (CLAST_STMT_IS_A(stmt, stmt_for))
      codegen((const clast_for *)stmt);
    else if (CLAST_STMT_IS_A(stmt, stmt_guard))
      codegen((const clast_guard *)stmt);

    if (stmt->next)
      codegen(stmt->next);
  }

  public:
  ClastStmtCodeGen(Scop *scop, DominatorTree *dt, Dependences *dp,
                   IRBuilder<> *B) :
    S(scop), DT(dt), DP(dp), Builder(B), ExpGen(Builder, &CharMap) {}

};
}

namespace {
class CodeGeneration : public ScopPass {
  Region *region;
  Scop *S;
  DominatorTree *DT;
  ScalarEvolution *SE;
  ScopDetection *SD;
  CloogInfo *C;
  LoopInfo *LI;

  public:
  static char ID;

  CodeGeneration() : ScopPass(ID) {}

  void createSeSeEdges(Region *R) {
    BasicBlock *newEntry = createSingleEntryEdge(R, this);

    for (Scop::iterator SI = S->begin(), SE = S->end(); SI != SE; ++SI)
      if ((*SI)->getBasicBlock() == R->getEntry())
        (*SI)->setBasicBlock(newEntry);

    createSingleExitEdge(R, this);
  }

  void addParameters(const CloogNames *names, CharMapT &VariableMap,
                     IRBuilder<> *Builder) {
    int i = 0;
    SCEVExpander Rewriter(*SE);

    // Create an instruction that specifies the location where the parameters
    // are expanded.
    CastInst::CreateIntegerCast(ConstantInt::getTrue(Builder->getContext()),
                                  Builder->getInt16Ty(), false, "insertInst",
                                  Builder->GetInsertBlock());

    for (Scop::param_iterator PI = S->param_begin(), PE = S->param_end();
         PI != PE; ++PI) {
      assert(i < names->nb_parameters && "Not enough parameter names");
      Instruction *InsertLocation;

      const SCEV *Param = *PI;
      const Type *Ty = Param->getType();

      InsertLocation = --(Builder->GetInsertBlock()->end());
      Value *V = Rewriter.expandCodeFor(Param, Ty, InsertLocation);
      VariableMap[names->parameters[i]] = V;

      ++i;
    }
  }

  // Adding prototypes required if OpenMP is enabled.
  void addOpenMPDefinitions(IRBuilder<> *Builder)
  {
    Module *M = Builder->GetInsertBlock()->getParent()->getParent();
    LLVMContext &Context = Builder->getContext();

    Function *FN = M->getFunction("GOMP_parallel_end");
    // Check if the definition is already added. Otherwise add it.
    if (!FN) {
      // Prototype for "GOMP_parallel_end".
      FunctionType *FT = FunctionType::get(Type::getVoidTy(Context),
                                           std::vector<const Type*>(), false);
      Function::Create(FT, Function::ExternalLinkage, "GOMP_parallel_end", M);
    }

    // Check if the definition is already added. Otherwise add it.
    if (!M->getFunction("GOMP_parallel_loop_runtime_start")) {
      // Creating type of first argument for GOMP_parallel_start.
      std::vector<const Type*> Arguments(1, Builder->getInt8PtrTy());
      FunctionType *FnArgTy = FunctionType::get(Builder->getVoidTy(),
                                                Arguments, false);
      PointerType *FnPtrTy = PointerType::getUnqual(FnArgTy);

      // Prototype for GOMP_parallel_loop_runtime_start.
      std::vector<const Type*> PsArguments;
      PsArguments.push_back(FnPtrTy);
      PsArguments.push_back(Builder->getInt8PtrTy());
      PsArguments.push_back(Builder->getInt32Ty());
      PsArguments.push_back(Builder->getInt64Ty());
      PsArguments.push_back(Builder->getInt64Ty());
      PsArguments.push_back(Builder->getInt64Ty());
      FunctionType *PsFT = FunctionType::get(Builder->getVoidTy(),
                                             PsArguments, false);
      Function::Create(PsFT, Function::ExternalLinkage,
                       "GOMP_parallel_loop_runtime_start", M);
    }
  }

  bool runOnScop(Scop &scop) {
    S = &scop;
    region = &S->getRegion();
    Region *R = region;
    DT = &getAnalysis<DominatorTree>();
    Dependences *DP = &getAnalysis<Dependences>();
    SE = &getAnalysis<ScalarEvolution>();
    LI = &getAnalysis<LoopInfo>();
    C = &getAnalysis<CloogInfo>();
    SD = &getAnalysis<ScopDetection>();

    Function *F = R->getEntry()->getParent();

    if (CodegenOnly != "" && CodegenOnly != F->getNameStr()) {
      errs() << "Codegenerating only function '" << CodegenOnly
        << "' skipping '" << F->getNameStr() << "' \n";
      return false;
    }

    createSeSeEdges(R);

    // Create a basic block in which to start code generation.
    BasicBlock *PollyBB = BasicBlock::Create(F->getContext(), "pollyBB", F);
    IRBuilder<> Builder(PollyBB);
    DT->addNewBlock(PollyBB, R->getEntry());

    ClastStmtCodeGen CodeGen(S, DT, DP, &Builder);

    const clast_stmt *clast = C->getClast();

    addParameters(((const clast_root*)clast)->names, CodeGen.CharMap, &Builder);

    if (OpenMP)
      addOpenMPDefinitions(&Builder);

    CodeGen.codegen(clast);

    BasicBlock *AfterScop = *pred_begin(R->getExit());
    Builder.CreateBr(AfterScop);

    // Update old PHI nodes to pass LLVM verification.
    for (BasicBlock::iterator SI = succ_begin(R->getEntry())->begin(),
         SE = succ_begin(R->getEntry())->end(); SI != SE; ++SI)
      if (PHINode::classof(&(*SI))) {
        PHINode *PN = static_cast<PHINode *>(&*SI);
        PN->removeIncomingValue(R->getEntry());
      }

    DT->changeImmediateDominator(AfterScop, Builder.GetInsertBlock());

    BasicBlock *OldRegionEntry = *succ_begin(R->getEntry());

    // Enable the new polly code.
    R->getEntry()->getTerminator()->setSuccessor(0, PollyBB);

    // Remove old Scop nodes from dominator tree.
    std::vector<DomTreeNode*> ToVisit;
    std::vector<DomTreeNode*> Visited;
    ToVisit.push_back(DT->getNode(OldRegionEntry));

    while (!ToVisit.empty()) {
      DomTreeNode *Node = ToVisit.back();

      ToVisit.pop_back();

      if (AfterScop == Node->getBlock())
        continue;

      Visited.push_back(Node);

      std::vector<DomTreeNode*> Children = Node->getChildren();
      ToVisit.insert(ToVisit.end(), Children.begin(), Children.end());
    }

    for (std::vector<DomTreeNode*>::reverse_iterator I = Visited.rbegin(),
         E = Visited.rend(); I != E; ++I)
      DT->eraseNode((*I)->getBlock());

    R->getParent()->removeSubRegion(R);

    // And forget the Scop if we remove the region.
    SD->forgetScop(*R);

    return false;
  }

  void print(raw_ostream &OS, const Module *) const { }

  virtual void getAnalysisUsage(AnalysisUsage &AU) const {
    AU.addRequired<CloogInfo>();
    AU.addRequired<Dependences>();
    AU.addRequired<DominatorTree>();
    AU.addRequired<ScalarEvolution>();
    AU.addRequired<LoopInfo>();
    AU.addRequired<RegionInfo>();
    AU.addRequired<ScopDetection>();
    AU.addRequired<ScopInfo>();

    AU.addPreserved<Dependences>();
    AU.addPreserved<LoopInfo>();
    AU.addPreserved<DominatorTree>();
    AU.addPreserved<PostDominatorTree>();
    AU.addPreserved<ScopDetection>();
    AU.addPreserved<ScalarEvolution>();
    AU.addPreserved<RegionInfo>();
    AU.addPreserved<ScopInfo>();
    AU.addPreservedID(IndependentBlocksID);
  }
};
}

char CodeGeneration::ID = 1;

static RegisterPass<CodeGeneration>
Z("polly-codegen", "Polly - Create LLVM-IR from the polyhedral information");

Pass* polly::createCodeGenerationPass() {
  return new CodeGeneration();
}
