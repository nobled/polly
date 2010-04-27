//===- polly/SCoP/SCoPInfo.h - Create SCoPs from LLVM IR --------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Create a polyhedral description for a static control flow region.
//
//===----------------------------------------------------------------------===//
//
#ifndef POLLY_SCOP_INFO_H
#define POLLY_SCOP_INFO_H

#include "polly/PollyType.h"

#include "llvm/Analysis/RegionInfo.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Scalar.h"

#include <set>
#include <map>

using namespace llvm;

namespace polly {

//===----------------------------------------------------------------------===//
/// TODO: place them to the right place
class SCoPInfo;
class SCoP;
///
class SCoPStmt
{
  SCoP &Parent;
  BasicBlock &BB;

  /// The iteration domain describes the set of iterations for which this
  /// statement is executed.
  ///
  /// Example:
  ///     for (i = 0; i < 100 + b; ++i)
  ///       S(i);
  ///
  ///     Domain: 0 <= i <= 100 + b
  polly_set *Domain;
  /// The scattering map describes the order in which the different statements
  /// executed. The scattering is closely related to the one of CLooG. So have
  /// a look at cloog.org to find a complete description.
  polly_map *Scattering;

  SCoPStmt(SCoP &parent, BasicBlock &bb, polly_set *domain, polly_map *scat);

  friend class SCoPInfo;
public:

  polly_set *getDomain() const { return Domain; }
  polly_map *getScattering() const { return Scattering; }

  BasicBlock *getBasicBlock() {
    return &BB;
  }

	~SCoPStmt();
};


/// @brief Static Control Part in program tree.
class SCoP {
  /// The underlying Region.
  Region &R;

  /// Max loop depth.
  unsigned MaxLoopDepth;

  typedef std::set<SCoPStmt*> StmtSet;
  /// The Statments in this SCoP.
  StmtSet Stmts;

  /// Parameters of this SCoP
  typedef SmallVector<const SCEV*, 8> ParamVecType;
  ParamVecType Parameters;
  /// Context
  // Is this constraints on parameters?
  polly_set *Context;

  ///
  polly_ctx *ctx;

  ///
  friend class SCoPInfo;

  template<class It>
  explicit SCoP(Region &r, unsigned maxLoopDepth, It ParamBegin, It ParamEnd);
public:

  ~SCoP();

  inline ParamVecType::size_type getNumParams() const {
    return Parameters.size();
  }
  inline const ParamVecType &getParams() const { return Parameters; }

  //void addStatement(SCoPStmt *stmt) { Stmts.insert(stmt); }

  typedef StmtSet::iterator iterator;
  typedef StmtSet::const_iterator const_iterator;

  iterator begin() { return Stmts.begin(); }
  iterator end()   { return Stmts.end();   }

  const_iterator begin() const { return Stmts.begin(); }
  const_iterator end()   const { return Stmts.end();   }

  typedef ParamVecType::iterator param_iterator;
  typedef ParamVecType::const_iterator const_param_iterator;

  param_iterator param_begin() { return Parameters.begin(); }
  param_iterator param_end()   { return Parameters.end(); }

  const_param_iterator param_begin() const { return Parameters.begin(); }
  const_param_iterator param_end()   const { return Parameters.end(); }

  inline const Region &getRegion() const { return R; }

  inline unsigned getMaxLoopDepth() const { return MaxLoopDepth; }
  /// Scattering dimension number
  inline unsigned getScatterDim() const { return 2 * MaxLoopDepth + 1; }

  inline polly_set *getContext() const { return Context; }

  void print(raw_ostream &OS) const;
};

//===----------------------------------------------------------------------===//

//===-------------------------------------------------------------------===//
// Affine function represent with llvm objects.
// A helper class for collect affine function information
struct SCEVAffFunc {
  // The translation component
  const SCEV *TransComp;
  // { Variable, Coefficient }
  typedef std::map<const SCEV*, const SCEV*> LnrTransSet;
  LnrTransSet LnrTrans;
  // The base address of the address SCEV.
  const Value *BaseAddr;

  explicit SCEVAffFunc() : TransComp(0), BaseAddr(0) {}

  // getCoeff - Get the Coefficient of a given variable.
  const SCEV *getCoeff(const SCEV *Var) {
    LnrTransSet::iterator At = LnrTrans.find(Var);
    return At == LnrTrans.end() ? 0 : At->second;
  }

  //
  void print(raw_ostream &OS, ScalarEvolution *SE) const;
};

class SCoPInfo : public FunctionPass {
  //===-------------------------------------------------------------------===//
  /// Types
  typedef std::set<const SCEV*> ParamSetType;

  // TODO: We need a standalone file to translate llvm affine function to
  // isl object
  // { Lowerbound, Upperbound }
  typedef std::pair<SCEVAffFunc, SCEVAffFunc> AffBoundType;
  // Only map bounds to loops, we could construct the iterate domain of
  // a BB/Region by visit all its parent. We can also have some stack to
  // do that.
  typedef std::map<const Loop*, AffBoundType> BoundMapType;

  // The access function of basic block (or region)
  // { address, isStore }
  typedef std::pair<SCEVAffFunc, bool> AccessFuncType;
  typedef std::vector<AccessFuncType> AccFuncSetType;
  typedef std::map<const BasicBlock*, AccFuncSetType> AccFuncMapType;

  typedef std::map<const Region*, SCoP*> SCoPMapType;

  //===-------------------------------------------------------------------===//
  // SCoP represent with llvm objects.
  // A helper class for finding SCoP,
  struct LLVMSCoP {
    // The Region.
    Region &R;
    ParamSetType Params;
    // TODO: Constraints on parameters?
    unsigned MaxLoopDepth;
    explicit LLVMSCoP(Region &r) : R(r), MaxLoopDepth(0) {}
    void print(raw_ostream &OS) const;
  };

  typedef std::vector<LLVMSCoP*> TempSCoPSetType;
  //===-------------------------------------------------------------------===//
  // DO NOT IMPLEMENT
  SCoPInfo(const SCoPInfo &);
  // DO NOT IMPLEMENT
  const SCoPInfo &operator=(const SCoPInfo &);

  // The ScalarEvolution to help building SCoP.
  ScalarEvolution* SE;
  // LoopInfo for information about loops
  LoopInfo *LI;
  // RegionInfo for regiontrees
  RegionInfo *RI;
  // Remember the bounds of loops, to help us build iterate domain of BBs.
  BoundMapType LoopBounds;
  // Access function of bbs.
  AccFuncMapType AccFuncMap;
  // SCoPs in the function
  SCoPMapType RegionToSCoPs;

  void clear();

  //===-------------------------------------------------------------------===//
  // Temporary Hack for extended regiontree.
  // Cast the region to loop if there is a loop have the same header and exit.
  Loop *castToLoop(const Region *R) const {
    BasicBlock *entry = R->getEntry();

    if (!LI->isLoopHeader(entry))
      return 0;

    Loop *L = LI->getLoopFor(entry);

    BasicBlock *exit = L->getExitBlock();

    assert(exit && "Oop! we need the extended region tree :P");

    if (exit != R->getExit()) {
      assert((RI->getRegionFor(entry) != R ||
              R->getParent()->getEntry() == entry) &&
              "Expect the loop is the smaller or bigger region");
      return 0;
    }

    return L;
  }

  // Get the Loop containing all bbs of this region, for ScalarEvolution
  // "getSCEVAtScope"
  Loop *getScopeLoop(const Region *R) {
    while (R) {
      if (Loop *L = castToLoop(R))
        return L;

      R = R->getParent();
    }
    return 0;
  }
  //===-------------------------------------------------------------------===//
  // Build affine function from SCEV expression.
  // Return true is S is affine, false otherwise.
  bool buildAffineFunc(const SCEV *S, LLVMSCoP *SCoP,
                       SCEVAffFunc &FuncToBuild);


  /*__isl_give*/ polly_basic_set *buildIterateDomain(SCoP &SCoP,
                                  SmallVectorImpl<Loop*> &NestLoops);

  // If the Region not a valid part of a SCoP,
  // return false, otherwise return true.
  LLVMSCoP *findSCoPs(Region* R, TempSCoPSetType &SCoPs);

  // Build the SCoP.
  void buildSCoP(SCoP &SCoP, const Region &CurRegion,
                 // TODO: How to handle union of Domains?
                 SmallVectorImpl<Loop*> &NestLoops,
                 // TODO: use vector?
                 SmallVectorImpl<unsigned> &Scatter);

  SCoPStmt *buildStmt(SCoP &SCoP, BasicBlock &BB,
                      SmallVectorImpl<Loop*> &NestLoops,
                      // TODO: use vector?
                      SmallVectorImpl<unsigned> &Scatter);

  // Check if the BB is a valid part of SCoP, return true and extract the
  // corresponding information, return false otherwise.
  bool checkBasicBlock(BasicBlock *BB, LLVMSCoP *SCoP);

  bool checkCFG(BasicBlock *BB, Region *R);

  // Merge the SCoP information of sub regions into MergeTo.
  void mergeSubSCoPs(LLVMSCoP *MergeTo, TempSCoPSetType &SubSCoPs) {
    while (!SubSCoPs.empty()) {
      LLVMSCoP *SubSCoP = SubSCoPs.back();
      // merge the parameters.
      MergeTo->Params.insert(SubSCoP->Params.begin(),
                             SubSCoP->Params.end());
      // Discard the merged scop.
      SubSCoPs.pop_back();
      delete SubSCoP;
    }
    // The induction variable is not parameter at this scope.
    if (Loop *L = castToLoop(&MergeTo->R))
      MergeTo->Params.erase(
        SE->getSCEV(L->getCanonicalInductionVariable()));
  }

  //typedef SCoPSetType::iterator iterator;
  //typedef SCoPSetType::const_iterator const_iterator;

  //iterator begin() { return SCoPs.begin(); }
  //const_iterator begin() { return SCoPs.begin(); }

  //iterator end() { return SCoPs.end(); }
  //iterator end() { return SCoPs.end(); }

  //
  void printBounds(raw_ostream &OS) const;
  void printAccFunc(raw_ostream &OS) const;
public:
  static char ID;
  explicit SCoPInfo() : FunctionPass(&ID) {}
  ~SCoPInfo();

  SCoP *getSCoPFor(const Region* R) const {
    SCoPMapType::const_iterator at = RegionToSCoPs.find(R);
    return at == RegionToSCoPs.end()?0:at->second;
  }

  /// Pass interface
  virtual void getAnalysisUsage(AnalysisUsage &AU) const {
    AU.addRequired<LoopInfo>();
    // Grr! These are looppasses!
    //AU.addRequiredID(IndVarSimplifyID);
    // Make loop only have 1 back-edge?
    //AU.addRequiredID(LoopSimplifyID);
    AU.addRequired<RegionInfo>();
    AU.addRequired<ScalarEvolution>();
    AU.setPreservesAll();
  }

  virtual void releaseMemory() {
    clear();
  }

  virtual bool runOnFunction(Function &F);

  virtual void print(raw_ostream &OS, const Module *) const;
};

/// Function for force linking.
FunctionPass *createSCoPInfoPass();


Pass* createScopPrinterPass();
Pass* createScopCodeGenPass();

} //end namespace polly

#endif
