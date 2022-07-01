#.rst:
# dueca_active_dco
# ----------------
#
# Determine which dco objects in a comm-objects folder are active
#
# Inputs:
#
# ``SOURCES`` (single value)
#     Name for list with sources
#
# Outputs:
#
# ``${SOURCES}``
#     List of DCO sources (.dco files) that need to be converted with
#     the code generator

include(CMakeParseArguments)

function(DUECA_ACTIVE_DCO)

  cmake_parse_arguments(ADCO
    "" "TARGETNAME;SOURCES" "" ${ARGN})

  # figure out which project folder this is
  if (CMAKE_CURRENT_SOURCE_DIR MATCHES ".*/\([^/]+\)/\([^/]+\)$")

    if (NOT CMAKE_MATCH_2 STREQUAL "comm-objects")
      message(FATAL_ERROR
        "Not a DCO dir from ${CMAKE_CURRENT_SOURCE_DIR}")
    endif()

    set(PRJ "${CMAKE_MATCH_1}")
    message(STATUS "DCO objects in folder ${PRJ}/comm-objects")

    # get active DCO objects
    get_property(DCO_SOURCES GLOBAL PROPERTY
      DCO_${PRJ}_SOURCES_PROPERTY)

    foreach(DCO ${DCO_SOURCES})
      message(STATUS "Adding object ${DCO}.dco")
      list(APPEND DCO_SOURCES2 ${DCO}.dco)
    endforeach()

    #message(STATUS "ARGN ${ARGN}")
    if (ADCO_SOURCES)
      set(${ADCO_SOURCES} ${DCO_SOURCES2} PARENT_SCOPE)
    endif()
  else()

    message(FATAL_ERROR
      "Cannot determine DCO dir from ${CMAKE_CURRENT_SOURCE_DIR}")

  endif()

endfunction()
