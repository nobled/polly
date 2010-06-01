//===- CLooGExporter.h - Export the cloog file ------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Export a .cloog file of the SCoP.
//
//===----------------------------------------------------------------------===//
//
#ifndef POLLY_CLOOG_EXPORTER_H
#define POLLY_CLOOG_EXPORTER_H

namespace llvm {
  struct RegionPass;
}

namespace polly {
  llvm::RegionPass* createCLooGExporterPass();
}
#endif /* POLLY_CLOOG_EXPORTER_H */
