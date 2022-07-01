# -*-cmake-*-
# ============================================================================
#       item            : CMake configuration DUECA module
#       made by         : René van Paassen
#       date            : 180326
#       copyright       : (c) 2018 TUDelft-AE-C&S
# ============================================================================

include(DuecaActiveDCO)
include(DuecaAddModule)
include(FindPkgConfig)
find_package(DuecaCodegen)

# additional local sources, do *NOT* include DCO-generated stuff, or
# files included by the DCO-generated stuff (MyObjectExtra.cxx,
# MyObjectExtra.hxx, etc.)
set(SOURCES )

# Retrieve DCO communication objects from this folder used by this project
# returns a list of active/needed DCO
dueca_active_dco(SOURCES ACTIVEDCO)

# specify codegen target to convert the DCO sources to C++
# sets DCO_OUTPUTS to a list of all generated source
# the INCLUDEDIRS here are used by the code generation to determine
# from where to include headers if these are needed to provide defines
# or array size constants, at the code generation stage
duecacodegen_target(OUTPUT DCO
  DCOSOURCE ${ACTIVEDCO}
  INCLUDEDIRS ${CMAKE_CURRENT_SOURCE_DIR})

# this dco folder may itself use other DCO files, listed in comm-objects.lst
# in this step tese are added resulting in DCO_INCLUD_DIRS and DCO_DEPENDS
dueca_add_dco()

# detect any needed libraries here with CMake's mechanisms, such as
# find_package, pkg_check_modules, or low-level find_path/find_library
# (rarely needed for DCO objects)

# if you need specific define's for the compilation, give them here
set(CUSTOM_OPTIONS )

# add the current module as target, with the present sources
# and generated sources
dueca_add_module(
  SOURCES ${SOURCES} ${DCO_OUTPUTS} ${CMAKE_CURRENT_BINARY_DIR}/comm-objects.h

  # specify include directories, add for used libraries as needed
  INCLUDEDIRS ${DCO_INCLUDE_DIRS}

  # optionally add more DUECA components
  # DUECA_COMPONENTS

  # if you directly use code from other modules here, specify these
  # in the form of project/module. Code generation order will be correct,
  # and the source location of these modules is added to the include path
  USEMODULES

  # optionally specify libraries
  LIBRARIES

  # optionally give compiler options
  COMPILEOPTIONS ${CUSTOM_OPTIONS}
  )

# specify dependency on (other project's) comm-objects
if(DCO_DEPENDS)
  add_dependencies(${CURRENT_MODULE} ${DCO_DEPENDS})
endif()
