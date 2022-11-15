# -*-cmake-*-
# ============================================================================
#       item            : CMake configuration DUECA module
#       made by         : René van Paassen
#       date            : 180326
#       copyright       : (c) 2021 TUDelft-AE-C&S
# ============================================================================

include(DuecaAddDCO)
include(DuecaAddModule)
include(FindPkgConfig)

# all local sources, replace by set(SOURCES ...) to be selective
file(GLOB SOURCES "*.cxx" "*.hxx")
# set(SOURCES ...)

# DCO communication objects used by this module, generated from the
# comm-objects.lst file
# this sets the DCO_INCLUDE_DIRS variable in this scope, which lists
# all folders with generated and original DCO source files
# this sets the DCO_DEPENDS variable in this scope, which lists
# the dependencies on dco's in comm-objects folders for this module
dueca_add_dco()

# detect any needed libraries here with CMake's mechanisms, such as
# find_package, pkg_check_modules, or low-level find_path/find_library

# if you need specific define's for the compilation, give them here
set(CUSTOM_OPTIONS )

# add current module as target, optionally add required dueca components
dueca_add_module(
  SOURCES ${SOURCES} ${CMAKE_CURRENT_BINARY_DIR}/comm-objects.h

  # specify include directories, add for used libraries as needed
  INCLUDEDIRS ${DCO_INCLUDE_DIRS}

  # include directories that should also be searched by other modules
  # that directly use code from this module (through USEMODULES)
  # INCLUDEDIRS_PUBLIC

  # optionally add more DUECA components (e.g., a graphics toolkit?)
  DUECA_COMPONENTS

  # if you directly use code from other modules here, specify these
  # in the form of project/module. Code generation order will be correct,
  # and the source location of these modules is added to the include path
  USEMODULES

  # optionally specify libraries used for this module
  LIBRARIES

  # give compiler options
  COMPILEOPTIONS ${CUSTOM_OPTIONS}

  # compile options that should also be added for other modules
  # that directly use code from this module (through USEMODULES)
  # COMPILEOPTIONS_PUBLIC
  )

# specify dependency on comm-objects
if(DCO_DEPENDS)
  add_dependencies(${CURRENT_MODULE} ${DCO_DEPENDS})
endif()
