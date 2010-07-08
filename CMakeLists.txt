#modify from clang cmakelist.txt

set(POLLY_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(POLLY_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})

# Add appropriate flags for GCC
if (CMAKE_COMPILER_IS_GNUCXX)
  # FIXME: Turn off exceptions, RTTI:
  # -fno-exceptions -fno-rtti
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-common -Woverloaded-virtual -Wno-long-long -Wall -W -Wno-unused-parameter -Wwrite-strings")
endif ()

# Add path for custom modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${POLLY_SOURCE_DIR}/cmake")

FIND_PACKAGE(Cloog REQUIRED)
FIND_PACKAGE(Isl REQUIRED)
FIND_PACKAGE(Gmp REQUIRED)

option(POLLY_ENABLE_OPENSCOP "Enalbe Openscop library for scop import/export" ON)
if (POLLY_ENABLE_OPENSCOP)
  FIND_PACKAGE(OpenScop)
endif(POLLY_ENABLE_OPENSCOP)

INCLUDE_DIRECTORIES( ${CLOOG_INCLUDE_DIR} )
INCLUDE_DIRECTORIES( ${ISL_INCLUDE_DIR} )
INCLUDE_DIRECTORIES( ${GMP_INCLUDE_DIR} )

# Support OpenScop export/import if the library is available.
if (OPENSCOP_FOUND)
  INCLUDE_DIRECTORIES( ${OPENSCOP_INCLUDE_DIR} )
endif(OPENSCOP_FOUND)

macro(add_polly_library name)
  set(srcs ${ARGN})
  if(MSVC_IDE OR XCODE)
    file( GLOB_RECURSE headers *.h *.td *.def)
    set(srcs ${srcs} ${headers})
    string( REGEX MATCHALL "/[^/]+" split_path ${CMAKE_CURRENT_SOURCE_DIR})
    list( GET split_path -1 dir)
    file( GLOB_RECURSE headers 
      ../../include/polly${dir}/*.h)
    set(srcs ${srcs} ${headers})
  endif(MSVC_IDE OR XCODE)
  if (SHARED_LIBRARY)
    set(libkind SHARED)
  else()
    set(libkind)
  endif()
  add_library( ${name} ${libkind} ${srcs} )
  if( LLVM_COMMON_DEPENDS )
    add_dependencies( ${name} ${LLVM_COMMON_DEPENDS} )
  endif( LLVM_COMMON_DEPENDS )
  if( LLVM_USED_LIBS )
    foreach(lib ${LLVM_USED_LIBS})
      target_link_libraries( ${name} ${lib} )
    endforeach(lib)
  endif( LLVM_USED_LIBS )

  target_link_libraries( ${name} ${CLOOG_LIBRARY} ${ISL_LIBRARY} ${GMP_LIBRARY})
  if (OPENSCOP_FOUND)
    target_link_libraries( ${name} ${OPENSCOP_LIBRARY})
  endif(OPENSCOP_FOUND)

  if( LLVM_LINK_COMPONENTS )
    llvm_config(${name} ${LLVM_LINK_COMPONENTS})
  endif( LLVM_LINK_COMPONENTS )
  get_system_libs(llvm_system_libs)
  if( llvm_system_libs )
    target_link_libraries(${name} ${llvm_system_libs})
  endif( llvm_system_libs )
  
  if(MSVC)
    get_target_property(cflag ${name} COMPILE_FLAGS)
    if(NOT cflag)
      set(cflag "")
    endif(NOT cflag)
    set(cflag "${cflag} /Za")
    set_target_properties(${name} PROPERTIES COMPILE_FLAGS ${cflag})
  endif(MSVC)
  install(TARGETS ${name}
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib${LLVM_LIBDIR_SUFFIX})
endmacro(add_polly_library)

macro(add_polly_executable name)
  set(srcs ${ARGN})
  if(MSVC_IDE)
    file( GLOB_RECURSE headers *.h *.td *.def)
    set(srcs ${srcs} ${headers})
  endif(MSVC_IDE)
  add_llvm_executable( ${name} ${srcs} )
endmacro(add_polly_executable)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_BINARY_DIR}/include
  )

install(DIRECTORY include
  DESTINATION .
  PATTERN ".svn" EXCLUDE
  )

add_definitions( -D_GNU_SOURCE )

add_subdirectory(include)
add_subdirectory(lib)

# Add the Scoplib export/import library if the Scoplib is available.
if (OPENSCOP_FOUND)
  add_subdirectory(lib/Exchange)
endif(OPENSCOP_FOUND)

add_subdirectory(tools)
add_subdirectory(test)
# TODO: docs.


configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/include/polly/Config/config.h.cmake
                ${LLVM_BINARY_DIR}/include/polly/Config/config.h )

