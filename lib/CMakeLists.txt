set(LLVM_NO_RTTI 1)

add_polly_library(LLVMPolly
  CodeGen.cpp
  SCoPDetection.cpp
  SCoPInfo.cpp
  Support/GmpConv.cpp
  )
