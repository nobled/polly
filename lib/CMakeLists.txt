set(MODULE TRUE)
set(LLVM_NO_RTTI 1)

if (OPENSCOP_FOUND)
  set(POLLY_EXCHANGE_FILES
      Exchange/OpenScopImporter.cpp Exchange/OpenScopExporter.cpp)
endif (OPENSCOP_FOUND)
if (SCOPLIB_FOUND)
  set(POLLY_SCOPLIB_FILES
      Pocc.cpp
      Exchange/ScopLib.cpp
      Exchange/ScopLibExporter.cpp
      Exchange/ScopLibImporter.cpp)
endif (SCOPLIB_FOUND)

add_polly_library(LLVMPolly
  Cloog.cpp
  CodePreparation.cpp
  CodeGeneration.cpp
  Dependences.cpp
  IndependentBlocks.cpp
  Interchange.cpp
  Pocc.cpp
  ScopPass.cpp
  ScopDetection.cpp
  ScopGraphPrinter.cpp
  ScopInfo.cpp
  TempScopInfo.cpp
  Support/GICHelper.cpp
  Support/AffSCEVItTester.cpp
  Support/ScopHelper.cpp
  json.c
  json_helper.c
  Exchange/JSONExporter.cpp
  ${POLLY_EXCHANGE_FILES}
  ${POLLY_SCOPLIB_FILES}
  )

set_target_properties(LLVMPolly
  PROPERTIES
  LINKER_LANGUAGE CXX
  PREFIX "")
