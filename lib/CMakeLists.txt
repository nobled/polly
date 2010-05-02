set(LLVM_NO_RTTI 1)

add_polly_library(LLVMPolly
  CLooG.cpp
  ClastCodeGeneration.cpp
  ClastParser.cpp
  ClastPrinter.cpp
  CodeGen.cpp
  SCoPDetection.cpp
  SCoPInfo.cpp
  Support/GmpConv.cpp
  Support/IRHelper.cpp
  Support/AffSCEVItTester.cpp
  )
