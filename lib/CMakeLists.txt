set(LLVM_NO_RTTI 1)

if (OPENSCOP_FOUND)
  set(POLLY_EXCHANGE_FILES Exchange/SCoPImporter.cpp Exchange/SCoPExporter.cpp)
endif (OPENSCOP_FOUND)

add_polly_library(LLVMPolly
  CLooG.cpp
  CodeGeneration.cpp
  Dependences.cpp
  IndependentBlocks.cpp
  SCoPPrinter.cpp
  SCoPDetection.cpp
  SCoPInfo.cpp
  SCoPCodePrep.cpp
  TempSCoPInfo.cpp
  Support/GmpConv.cpp
  Support/AffSCEVItTester.cpp
  Support/SCoPHelper.cpp
  ${POLLY_EXCHANGE_FILES}
  )
