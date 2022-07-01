# ============================================================================
#       item            : CMake configuration DUECA module
#       made by         : René van Paassen
#       date            : 180326
#       copyright       : (c) 2018 TUDelft-AE-C&S
# ============================================================================

include(DuecaAddDCO)
include(DuecaAddModule)

# all local sources, replace by set(SOURCES ...) to be selective
file(GLOB SOURCES "*.hxx" "*.cxx" "*.h" "*.c")

# DCO communication objects used by this module
dueca_add_dco(project/TestObject
  INCLUDEDIRS DCO_INCLUDE_DIRECTORIES)

# add current module as target, optionally add required dueca components
dueca_add_module(${SOURCES} 
  TARGETNAME MODULE
  DUECA_COMPONENTS hdf5
  INCLUDEDIRS ${DCO_INCLUDE_DIRECTORIES}
  )

# if applicable, specify libraries, compile options or include dirs
#include(FindPkgConfig)
#pkg_check_modules(PUGIXML REQUIRED pugixml)
find_package(pugixml REQUIRED)  # imports a target

target_link_libraries(${MODULE} PUBLIC pugixml)
# target_compile_options(${MODULE} PUBLIC ...)
# target_include_directories(${MODULE} PUBLIC ${DCO_INCLUDE_DIRECTORIES})
