set(LLVM_NO_RTTI 1)

add_polly_library(LLVMPolly
  CLooG.cpp
  ClastCodeGeneration.cpp
  ClastParser.cpp
  ClastPrinter.cpp
  CodeGen.cpp
  SCoPCondition.cpp
  SCoPDetection.cpp
  SCoPInfo.cpp
  ScalarDataRef.cpp
  Support/GmpConv.cpp
  Support/IRHelper.cpp
  Support/AffSCEVItTester.cpp
  )

if (OPENSCOP_FOUND)
  target_link_libraries (LLVMPolly LLVMSCoPExchange)
endif (OPENSCOP_FOUND)
