//===- GraphPrinter.cpp - Create a DOT output describing the SCoP. --------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Create a DOT output describing the SCoP.
//
// For each function a dot file is created that shows the control flow graph of
// the function and highlights the detected SCoPs.
//
//===----------------------------------------------------------------------===//

#include "polly/LinkAllPasses.h"
#include "polly/SCoPDetection.h"

#include "llvm/Analysis/DOTGraphTraitsPass.h"
#include "llvm/Analysis/RegionInfo.h"
#include "llvm/Analysis/RegionIterator.h"

using namespace polly;
using namespace llvm;

namespace llvm {
  template <> struct GraphTraits<SCoPDetection*>
    : public GraphTraits<RegionInfo*> {

    static NodeType *getEntryNode(SCoPDetection *SD) {
      return GraphTraits<RegionInfo*>::getEntryNode(SD->getRI());
    }
    static nodes_iterator nodes_begin(SCoPDetection* SD) {
      return nodes_iterator::begin(getEntryNode(SD));
    }
    static nodes_iterator nodes_end(SCoPDetection *SD) {
      return nodes_iterator::end(getEntryNode(SD));
    }
  };

template<>
struct DOTGraphTraits<RegionNode*> : public DefaultDOTGraphTraits {

  DOTGraphTraits (bool isSimple=false)
    : DefaultDOTGraphTraits(isSimple) {}

  std::string getNodeLabel(RegionNode *Node, RegionNode *Graph) {

    if (!Node->isSubRegion()) {
      BasicBlock *BB = Node->getNodeAs<BasicBlock>();

      if (isSimple())
        return DOTGraphTraits<const Function*>
          ::getSimpleNodeLabel(BB, BB->getParent());
      else
        return DOTGraphTraits<const Function*>
          ::getCompleteNodeLabel(BB, BB->getParent());
    }

    return "Not implemented";
  }
};

template<>
struct DOTGraphTraits<SCoPDetection*> : public DOTGraphTraits<RegionNode*> {
  DOTGraphTraits (bool isSimple=false)
    : DOTGraphTraits<RegionNode*>(isSimple) {}
  static std::string getGraphName(SCoPDetection *SD) {
    return "SCoP Graph";
  }
  std::string getNodeLabel(RegionNode *Node, SCoPDetection *SD) {
    return DOTGraphTraits<RegionNode*>::getNodeLabel(Node,
                                                     SD->getRI()->getTopLevelRegion());
  }
  // Print the cluster of the subregions. This groups the single basic blocks
  // and adds a different background color for each group.
  static void printRegionCluster(const SCoPDetection *SD, const Region *R,
                                 raw_ostream &O, unsigned depth = 0) {
    O.indent(2 * depth) << "subgraph cluster_" << static_cast<const void*>(R)
      << " {\n";
    O.indent(2 * (depth + 1)) << "label = \"\";\n";

    if (SD->isMaxRegionInSCoP(*R))
      O.indent(2 * (depth + 1)) << "style = filled;\n";
    else
      O.indent(2 * (depth + 1)) << "style = solid;\n";
    O.indent(2 * (depth + 1)) << "color = "
      << ((R->getDepth() * 2 % 12) + 1) << "\n";

    for (Region::const_iterator RI = R->begin(), RE = R->end(); RI != RE; ++RI)
      printRegionCluster(SD, *RI, O, depth + 1);

    RegionInfo *RI = R->getRegionInfo();

    for (Region::const_block_iterator BI = R->block_begin(),
         BE = R->block_end(); BI != BE; ++BI) {
      BasicBlock *BB = (*BI)->getNodeAs<BasicBlock>();
      if (RI->getRegionFor(BB) == R)
        O.indent(2 * (depth + 1)) << "Node"
          << static_cast<const void*>(RI->getTopLevelRegion()->getBBNode(BB))
          << ";\n";
    }

    O.indent(2 * depth) << "}\n";
  }
  static void addCustomGraphFeatures(const SCoPDetection* SD,
                                     GraphWriter<SCoPDetection*> &GW) {
    raw_ostream &O = GW.getOStream();
    O << "\tcolorscheme = \"paired12\"\n";
    printRegionCluster(SD, SD->getRI()->getTopLevelRegion(), O, 4);
  }
};

} //end namespace llvm

struct SCoPViewer
  : public DOTGraphTraitsViewer<SCoPDetection, false> {
  static char ID;
  SCoPViewer() : DOTGraphTraitsViewer<SCoPDetection, false>("scops", ID){}
};
char SCoPViewer::ID = 0;

struct SCoPOnlyViewer
  : public DOTGraphTraitsViewer<SCoPDetection, true> {
  static char ID;
  SCoPOnlyViewer() : DOTGraphTraitsViewer<SCoPDetection, true>("scopsonly", ID){}
};
char SCoPOnlyViewer::ID = 0;

struct SCoPPrinter
  : public DOTGraphTraitsPrinter<SCoPDetection, false> {
  static char ID;
  SCoPPrinter() :
    DOTGraphTraitsPrinter<SCoPDetection, false>("scops", ID) {}
};
char SCoPPrinter::ID = 0;

struct SCoPOnlyPrinter
  : public DOTGraphTraitsPrinter<SCoPDetection, true> {
  static char ID;
  SCoPOnlyPrinter() :
    DOTGraphTraitsPrinter<SCoPDetection, true>("scopsonly", ID) {}
};
char SCoPOnlyPrinter::ID = 0;

static RegisterPass<SCoPViewer>
X("polly-view","Polly - View SCoPs of function");

static RegisterPass<SCoPOnlyViewer>
Y("polly-view-only",
  "Polly - View SCoPs of function (with no function bodies)");

static RegisterPass<SCoPPrinter>
M("polly-dot", "Polly - Print SCoPs of function");

static RegisterPass<SCoPOnlyPrinter>
N("polly-dot-only",
  "Polly - Print SCoPs of function (with no function bodies)");

Pass* polly::createDOTViewerPass() {
  return new SCoPViewer();
}

Pass* polly::createDOTOnlyViewerPass() {
  return new SCoPOnlyViewer();
}

Pass* polly::createDOTPrinterPass() {
  return new SCoPPrinter();
}

Pass* polly::createDOTOnlyPrinterPass() {
  return new SCoPOnlyPrinter();
}
