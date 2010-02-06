set(LLVM_NO_RTTI 1)

set( LLVM_USED_LIBS
  sample
  )

set(LLVM_LINK_COMPONENTS 
    bitreader asmparser bitwriter instrumentation scalaropts ipo
)

add_polly_executable(popt
  AnalysisWrappers.cpp
  GraphPrinters.cpp
  PrintSCC.cpp
  polly_opt.cpp
  )
