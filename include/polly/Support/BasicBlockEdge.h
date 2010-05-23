//===- Support/BasicBlockEdge.h --------- Edge of BasicBlocks ---------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Represent BasicBlock Edges, just a wrapper of SuccIterator in CFG.h, but
// allow assignment between Edges with difference Source BasicBlock
//
//===----------------------------------------------------------------------===//

#ifndef POLLY_SUPPORT_BASICBLOCK_EDGE_H
#define POLLY_SUPPORT_BASICBLOCK_EDGE_H

#include "llvm/Support/CFG.h"

using namespace llvm;

namespace polly {
/// Represent Edge of BasicBlocks in CFG
template <class Term_, class BB_>
class BBEdge {
  typedef BBEdge<Term_, BB_> Self;
  typedef SuccIterator<Term_, BB_> succ_type;
  /// The Source block of this BasicBlock
  BB_ *Src;
  /// The index of the dest BasicBlock
  unsigned DstIdx;
public:
  explicit BBEdge(SuccIterator<Term_, BB_> &It)
    : Src(It.getSource()), DstIdx(It.getSuccessorIndex()) {}

  // Cast BBEdge back to SuccIterator.
  operator succ_type() const {
    succ_type It = succ_type(Src->getTerminator());
    return (It+=DstIdx);
  }

  inline const succ_type operator->() const {
    return succ_type(*this);
  }

  inline BB_ *getSource() { return Src; }

  // Assign the succ_iterator to this edge
  inline const Self &operator=(const succ_type &It) {
    Src = const_cast<succ_type*>(&It)->getSource();
    DstIdx = It.getSuccessorIndex();
    return *this;
  }
};

typedef BBEdge<TerminatorInst*, BasicBlock> bb_edge;
typedef BBEdge<const TerminatorInst*, const BasicBlock> const_bb_edge;
}

#endif
