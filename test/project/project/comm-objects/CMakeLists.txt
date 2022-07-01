# ============================================================================
#       item            : CMake configuration DUECA module
#       made by         : René van Paassen
#       date            : 180326
#       copyright       : (c) 2018 TUDelft-AE-C&S
# ============================================================================

include(DuecaActiveDCO)
include(DuecaAddModule)
find_package(DuecaCodegen)

# all local sources, do not include DCO-generated stuff
set(SOURCES )

# DCO communication objects used by this project
dueca_active_dco(ACTIVEDCO)

# convert the DCO sources to C++
duecacodegen_target(OUTPUT DCO DCOSOURCE ${ACTIVEDCO})

# add current module as target, optionally add required dueca components
dueca_add_module(${DCO_OUTPUT_SOURCE} 
  TARGETNAME MODULE
  DUECA_COMPONENTS
  INCLUDEDIRS ${DCO_INCLUDE_DIRECTORIES}
  )
