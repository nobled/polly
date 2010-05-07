set(LLVM_NO_RTTI 1)

add_polly_library(LLVMSCoPExchange
  SCoPImporter.cpp
  SCoPExporter.cpp
  )
