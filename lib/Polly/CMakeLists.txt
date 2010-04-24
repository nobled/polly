set(LLVM_NO_RTTI 1)

add_polly_library(LLVMPolly
  CodeGen.cpp
  SCoP.cpp
  SCoPDetection.cpp
  SCoPInfo.cpp
  )
