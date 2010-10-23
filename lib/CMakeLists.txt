set(MODULE TRUE)
set(LLVM_NO_RTTI 1)

if (OPENSCOP_FOUND)
  set(POLLY_EXCHANGE_FILES
      Exchange/OpenSCoPImporter.cpp Exchange/OpenSCoPExporter.cpp)
endif (OPENSCOP_FOUND)
if (SCOPLIB_FOUND)
  set(POLLY_SCOPLIB_FILES
      Exchange/SCoPLibExporter.cpp
      Exchange/SCoPLibImporter.cpp)
endif (SCOPLIB_FOUND)

add_polly_library(LLVMPolly
  CLooG.cpp
  CodePreparation.cpp
  CodeGeneration.cpp
  Dependences.cpp
  IndependentBlocks.cpp
  SCoPPrinter.cpp
  SCoPDetection.cpp
  SCoPGraphPrinter.cpp
  SCoPInfo.cpp
  TempSCoPInfo.cpp
  Support/GmpConv.cpp
  Support/AffSCEVItTester.cpp
  Support/SCoPHelper.cpp
  ${POLLY_EXCHANGE_FILES}
  ${POLLY_SCOPLIB_FILES}
  )

set_target_properties(LLVMPolly
  PROPERTIES
  LINKER_LANGUAGE CXX
  PREFIX "")
