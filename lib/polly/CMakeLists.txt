set(LLVM_NO_RTTI 1)

add_polly_library(LLVMPolly
  sample.c
  PollyPass.cpp
  )

