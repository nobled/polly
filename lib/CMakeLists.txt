add_subdirectory(Analysis)
add_subdirectory(Support)

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

set(LLVM_USED_LIBS
  PollyAnalysis
  PollySupport
  )

add_polly_library(LLVMPolly
  Cloog.cpp
  CodePreparation.cpp
  CodeGeneration.cpp
  IndependentBlocks.cpp
  Interchange.cpp
  MayAliasSet.cpp
  Pocc.cpp
  RegionSimplify.cpp
  json.c
  json_helper.c
  Exchange/JSONExporter.cpp
  ${POLLY_EXCHANGE_FILES}
  ${POLLY_SCOPLIB_FILES}
  )

add_dependencies(LLVMPolly
  PollyAnalysis
  PollySupport
  )

set_target_properties(LLVMPolly
  PROPERTIES
  LINKER_LANGUAGE CXX
  PREFIX "")
