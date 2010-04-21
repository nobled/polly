set(LLVM_NO_RTTI 1)

add_polly_library(LLVMSCoP
  SCoPInfo.cpp
  SCoPReader.cpp
  SCoPWriter.cpp
  SCoP.cpp
  )
