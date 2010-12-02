set(MODULE TRUE)
set(LLVM_NO_RTTI 1)

if (OPENSCOP_FOUND)
  set(POLLY_EXCHANGE_FILES
      Exchange/OpenScopImporter.cpp Exchange/OpenScopExporter.cpp)
endif (OPENSCOP_FOUND)
if (SCOPLIB_FOUND)
  set(POLLY_SCOPLIB_FILES
      Exchange/ScopLibExporter.cpp
      Exchange/ScopLibImporter.cpp)
endif (SCOPLIB_FOUND)

add_polly_library(LLVMPolly
  CLooG.cpp
  CodePreparation.cpp
  CodeGeneration.cpp
  Dependences.cpp
  IndependentBlocks.cpp
  ScopPass.cpp
  ScopDetection.cpp
  ScopGraphPrinter.cpp
  ScopInfo.cpp
  TempScopInfo.cpp
  Support/GmpConv.cpp
  Support/AffSCEVItTester.cpp
  Support/ScopHelper.cpp
  ${POLLY_EXCHANGE_FILES}
  ${POLLY_SCOPLIB_FILES}
  )

set_target_properties(LLVMPolly
  PROPERTIES
  LINKER_LANGUAGE CXX
  PREFIX "")
