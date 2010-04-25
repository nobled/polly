set(LLVM_NO_RTTI 1)

add_polly_library(LLVMPolly
  CodeGen.cpp
  SCoPPass.cpp
  SCoPDetection.cpp
  SCoPInfo.cpp
  )
