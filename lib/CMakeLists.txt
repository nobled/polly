set(LLVM_NO_RTTI 1)

add_polly_library(LLVMPolly
  CLooG.cpp
  CodeGeneration.cpp
  ClastParser.cpp
  ClastPrinter.cpp
  SCoPPrinter.cpp
  SCoPDetection.cpp
  SCoPInfo.cpp
  ScalarDataRef.cpp
  TempSCoPInfo.cpp
  Support/GmpConv.cpp
  Support/AffSCEVItTester.cpp
  Support/SCoPHelper.cpp
  )

if (OPENSCOP_FOUND)
  target_link_libraries (LLVMPolly LLVMSCoPExchange)
endif (OPENSCOP_FOUND)
