#.rst:
# hmilib_add_if
# -------------
#
# Converts a list of hmilib if files into a sources and build target
#
# Outputs:
#
# ``HMILIB_SOURCES``
#     List of generated source files
# ``HMILIB_IF``
#     Interface files

include(CMakeParseArguments)

function(HMILIB_ADD_IF)

  cmake_parse_arguments(HAI "" "HMILIB_SOURCES" "HMILIB_IF" ${ARGN}) 

  # list of generated sources to return later
  set(GENSOURCES )

  foreach (I ${HAI_HMILIB_IF})

    # case where the source dir is part of the given file name
    # determine the filename proper, and any additional path leading up to it
    if (I MATCHES "^${CMAKE_CURRENT_SOURCE_DIR}/(.*/)?([^/]*)\\.if$")

      set(XTRAPATH "${CMAKE_MATCH_1}" )
      set(BASE "${CMAKE_MATCH_2}" )

    # case where source dir not in the file name
    elseif(I MATCHES "^(.*/)?([^/]*)\\.if$")

      set(XTRAPATH "${CMAKE_MATCH_1}" )
      set(BASE "${CMAKE_MATCH_2}" )

    else()
      message(FATAL_ERROR "Cannot parse HMILib interface name from ${I}")

    endif()

    # less work if the additional path is empty
    if (XTRAPATH STREQUAL "")
      add_custom_command(
        COMMAND
        ifparser ${CMAKE_CURRENT_SOURCE_DIR}/${BASE}.if
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${BASE}.hxx
        ${CMAKE_CURRENT_BINARY_DIR}/${BASE}.cxx
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${BASE}.if
        COMMENT "Generating interface definitions from ${BASE}.if"
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        )

    else()
      # ensure that the additional path is created if it is not there
      add_custom_command(
        COMMAND test -d ${XTRAPATH} || install -d ${XTRAPATH} &&
        cd ${XTRAPATH} &&
        ifparser ${CMAKE_CURRENT_SOURCE_DIR}/${XTRAPATH}${BASE}.if
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${XTRAPATH}${BASE}.hxx
        ${CMAKE_CURRENT_BINARY_DIR}/${XTRAPATH}${BASE}.cxx
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${XTRAPATH}${BASE}.if
        COMMENT "Generating interface definitions from ${BASE}.if"
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        )

    endif()

    # remember generated source names
    list(APPEND GENSOURCES
      "${CMAKE_CURRENT_BINARY_DIR}/${XTRAPATH}${BASE}.hxx"
      "${CMAKE_CURRENT_BINARY_DIR}/${XTRAPATH}${BASE}.cxx")

  endforeach()

  # return the list of generated .cxx and .hxx files
  set(${HAI_HMILIB_SOURCES} ${GENSOURCES} PARENT_SCOPE)

endfunction()
