#.rst:
# dueca_setup_project
# -------------------
#
# Main call to setup a DUECA project build structure. Called from the
# CMakeLists.txt file in the main project folder
#
# It performs the following:
# - Determine the list of modules
# - Add those as subdirectories
# - Determine all needed comm-objects folder
# - Add those as subdirectories
#
# Inputs:
#
# * A list of DUECA components
#
# Outputs:
#
# * ``PROJECT_LIBRARIES``
#
#   List of all libraries needed project-wide, from the config.cmake
#   setting
#
# * ``PROJECT_COMPILEOPTIONS``
#
#   List of all compile options, from the config.cmake setting
#
# * ``PROJECT_MODULES``
#
#   List all modules in this projects. Determined by reading
#   the ``.config/machine`` file, and the corresponding
#   ``.config/class/<machine>/modules.xml``
#
# * ``DUECA_LIBRARIES``
#
#   All DUECA libraries needed by the project, determined from the
#   list of DUECA components given
#
# * ``DUECA_COMPILEOPTIONS``
#
#   All globally applicable compile options

include(CMakeParseArguments)
include(DuecaGetConfig)
include(DuecaReadModules)

if (${CMAKE_VERSION} VERSION_GREATER "3.18.0")
  include(CheckLinkerFlag)
  # note: only the -whole-archive flag will fail on Linux
  check_linker_flag(CXX -Wl,--whole-archive,--no-whole-archive LD_WHOLEARCHIVE)
  check_linker_flag(CXX -all_load      LD_ALL_LOAD)
else()
  set(LD_WHOLEARCHIVE ON)
  set(LD_ALL_LOAD OFF)
endif()

function(DUECA_SETUP_PROJECT)

  # load the "machine class" configuration
  file(READ "${CMAKE_SOURCE_DIR}/.config/machine" MACHINECLASS)
  string(STRIP ${MACHINECLASS} MACHINECLASS)

  # treat modules.xml as a configure file, to get cmake rerun if
  # changed
  configure_file(
    "${CMAKE_SOURCE_DIR}/.config/class/${MACHINECLASS}/modules.xml"
    "${CMAKE_BINARY_DIR}/modules.xml" COPYONLY)

  # the arguments of this call are all selected components
  set(DUECA_COMPONENTS ${ARGN})

  # process additional cmake configuration (e.g., libraries) for this
  # class of machine
  include("${CMAKE_SOURCE_DIR}/.config/class/${MACHINECLASS}/config.cmake")
  set(PROJECT_LIBRARIES ${PROJECT_LIBRARIES} PARENT_SCOPE)
  set(PROJECT_COMPILEOPTIONS ${PROJECT_COMPILE_FLAGS} PARENT_SCOPE)

  # initial feedback to the user
  message(STATUS "Project configuration for machine class ${MACHINECLASS}")
  message(STATUS "Included components ${DUECA_COMPONENTS}")

  # read the list of modules
  dueca_read_modules(
    MACHINECLASS ${MACHINECLASS}
    MODULES MODLIST)

  # message(STATUS "modlist ${MODLIST}")
  #string(REGEX REPLACE ";" "\\\\;" MODLIST "${MODLIST}")
  #string(REGEX REPLACE "\n" ";" MODLIST "${MODLIST}")

  # list of modules as a global property
  if (LD_WHOLEARCHIVE)
    set_property(GLOBAL PROPERTY MODULES_PROPERTY "-Wl,--whole-archive")
  elseif(LD_ALL_LOAD)
    set_property(GLOBAL PROPERTY MODULES_PROPERTY "-Wl,-all_load")
  else()
    message(FATAL_ERROR "No method to ensure full loading of archive")
  endif()
  # list of DCO directories
  set_property(GLOBAL PROPERTY DCODIR_LIST_PROPERTY)
  # list of all dueca components
  set_property(GLOBAL PROPERTY DUECA_COMPONENTS_PROPERTY ${DUECA_COMPONENTS})

  # dueca flags with the current set of properties
  dueca_get_config(
    LIBRARIES DUECA_LIBRARIES
    COMPILEOPTIONS DUECA_COMPILEOPTIONS
    SCRIPTLANG DUECA_SCRIPTLANG
    ${DUECA_COMPONENTS})

  set(DUECA_LIBRARIES ${DUECA_LIBRARIES} PARENT_SCOPE)
  set(DUECA_COMPILEOPTIONS ${DUECA_COMPILEOPTIONS} PARENT_SCOPE)

  # find base project name
  get_filename_component(BASEDIR ${CMAKE_SOURCE_DIR} DIRECTORY)

  # include all listed modules
  foreach(M ${MODLIST})
    # message(STATUS "M ${M}")
    string(REGEX REPLACE "[#].*$" "" MDL ${M})
    # message(STATUS "M2 ${MDL}")
    string(STRIP "${MDL}" MDL)
    # message(STATUS "M3 ${MDL}")
    if (NOT MDL STREQUAL "")
      if (IS_DIRECTORY "${BASEDIR}/${MDL}")
        if (EXISTS "${BASEDIR}/${MDL}/CMakeLists.txt")
          add_subdirectory("${BASEDIR}/${MDL}"
            "${CMAKE_SOURCE_DIR}/build/${MDL}")
        else()
          message(STATUS "No CMakeLists.txt in ${MDL}, assuming data dir")
        endif()
      else()
        message(STATUS "No folder found ${MDL}")
      endif()
    endif()
  endforeach()

  # get the list of comm-objects libraries
  get_property(DCOPRJ_LIST GLOBAL PROPERTY DCOPRJ_LIST_PROPERTY)
  #message(STATUS "DCOPRJ_LIST ${DCOPRJ_LIST}")
  foreach(PRJ ${DCOPRJ_LIST})
    if (NOT EXISTS "${BASEDIR}/${PRJ}/comm-objects")
      message(FATAL_ERROR "No folder ${PRJ}/comm-objects")

    elseif (NOT EXISTS "${BASEDIR}/${PRJ}/comm-objects/CMakeLists.txt")
      message(FATAL_ERROR "No CMakeLists.txt in ${PRJ}/comm-objects")

    endif()

    add_subdirectory("${BASEDIR}/${PRJ}/comm-objects"
      "${CMAKE_BINARY_DIR}/${PRJ}/comm-objects")

  endforeach()

  # return list of all modules
  get_property(MODULES GLOBAL PROPERTY MODULES_PROPERTY)
  if (LD_WHOLEARCHIVE)
    list(APPEND MODULES "-Wl,--no-whole-archive")
  elseif(LD_ALL_LOAD)
    list(APPEND MODULES "-Wl,-noall_load")
  endif()
  set(PROJECT_MODULES ${MODULES} PARENT_SCOPE)

  # determine dueca library list
  file(WRITE "${CMAKE_BINARY_DIR}/empty.cxx" "// main in dueca library")

  get_property(DUECA_COMPONENTS GLOBAL PROPERTY DUECA_COMPONENTS_PROPERTY)

  add_custom_target(scriptlang
    COMMAND echo ${DUECA_SCRIPTLANG})
  #message(STATUS "pDUECA_LIBRARIES ${DUECA_LIBRARIES}")
  #message(STATUS "pDUECA_COMPILEOPTIONS ${DUECA_COMPILEOPTIONS}")

endfunction()
