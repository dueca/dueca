#.rst
# dueca_add_module
# ----------------
#
# Specify a DUECA module or the code for a comm-objects folder.
#
# Takes the following input
#
# * ``DUECA_COMPONENTS`` (list)
#
#   Additional DUECA components to include in the build. See
#   ``dueca-config --help`` for the list of possible components, note that
#   the two leading hyphens ``--`` are not needed.
#
# * ``LIBRARIES`` (list)
#
#   Libraries needed for this module.
#
# * ``COMPILEOPTIONS`` (list)
#
#   Compile flags for this module, e.g., option defines
#
# * ``COMPILEOPTIONS_PUBLIC`` (list)
#
#   Public compile flags, visible to other modules using this module
#
# * ``INCLUDEDIRS``(list)
#
#   Header include directories to add
#
# * ``INCLUDEDIRS_PUBLIC``(list)
#
#   Public header include directories to add, affect other modules
#   using this module
#
# * ``SOURCES``(list)
#
#   List of sources to compile
#
# * ``USEMODULES``(list)
#
#   Modules on which this module directly depends (which should be
#   avoided if possible, of course). These modules will be run first,
#   so that possibly generated code is
#
# This call adds a static library target <PROJECT>_<MODULE>, sets
# optional dependencies on other module targets, dco files, and compile
# options. 

include(CMakeParseArguments)
include(DuecaGetConfig)
include(DuecaReadModules)

function(DUECA_ADD_MODULE)

  # decode the arguments
  cmake_parse_arguments(ADDMODULE "" ""
    "DUECA_COMPONENTS;LIBRARIES;COMPILEOPTIONS;COMPILEOPTIONS_PUBLIC;INCLUDEDIRS;INCLUDEDIRS_PUBLIC;SOURCES;USEMODULES" ${ARGN})

  # create module name on basis of current source dir name
  if (CMAKE_CURRENT_SOURCE_DIR MATCHES ".*/\([^/]+\)/\([^/]+\)$")
    set(MODULETARGET "${CMAKE_MATCH_1}_${CMAKE_MATCH_2}")
    set(MODULENAME "${CMAKE_MATCH_1}/${CMAKE_MATCH_2}")
    message(STATUS "Adding module ${MODULETARGET}")
   else()
    message(FATAL_ERROR
      "Cannot determine module target from $(CMAKE_CURRENT_SOURCE_DIR}")
  endif()

  # add target to list of targets
  get_property(MODULES GLOBAL PROPERTY MODULES_PROPERTY)
  list(FIND MODULES ${MODULETARGET} MODULE_FOUND)
  if (MODULE_FOUND GREATER -1)
    message(FATAL_ERROR "Duplicate module ${MODULENAME}, existing ${MODULES}")
  endif()

  # update global list
  list(APPEND MODULES ${MODULETARGET})
  set_property(GLOBAL PROPERTY MODULES_PROPERTY ${MODULES})

  # determine DUECA libraries
  if (ADDMODULE_DUECA_COMPONENTS)

    message(STATUS
      "module ${MODULENAME} adds component(s) ${ADDMODULE_DUECA_COMPONENTS}")
    dueca_get_config(
      LIBRARIES DUECA_LIBRARIES
      COMPILEOPTIONS DUECA_COMPILEOPTIONS
      ${DUECA_COMPONENTS} ${ADDMODULE_DUECA_COMPONENTS})

  else()

    # DUECA component set is standard, retrieve
    # get_property(DUECA_LIBRARIES GLOBAL PROPERTY DUECA_LIBRARIES_PROPERTY)
    # get_property(DUECA_COMPILEOPTIONS GLOBAL PROPERTY
    #  DUECA_COMPILEOPTIONS_PROPERTY)
  endif()

  # find comm-objects.h invocations, link dependency
  foreach(S ${ADDMODULE_SOURCES})
    execute_process(COMMAND
      grep "^[ \t]*#include[ \t][ \t]*[<\"](\\.\\./)*comm-objects\\.h[>\"][ \t]*$" ${S}
      RESULT_VARIABLE RES
      OUTPUT_QUIET ERROR_QUIET)
    if (RES EQUAL 0)
      set_property(SOURCE ${S} APPEND PROPERTY OBJECT_DEPENDS
        ${MODULETARGET}_LINK)
    else()
#      message(STATUS "Source ${S} no depend on ${MODULETARGET}_LINK")
    endif()
  endforeach()

  # set the current target
  if (ADDMODULE_SOURCES)
    add_library(${MODULETARGET} STATIC ${ADDMODULE_SOURCES})
  else()
    add_library(${MODULETARGET} STATIC ${CMAKE_BINARY_DIR}/empty.cxx)
    message(NOTICE "${MODULETARGET} has no source files")
  endif()
  set_target_properties(${MODULETARGET} PROPERTIES
    OUTPUT_NAME "module"
    LINKER_LANGUAGE CXX)
  
  # check if we are in a GIT controlled repo, and if so, add a DUECA_GITHASH
  # define to the compile options
  execute_process(COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    RESULT_VARIABLE DOES_GIT
    OUTPUT_VARIABLE GITHASH
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  # message(STATUS "In ${CMAKE_CURRENT_SOURCE_DIR} have ${GITHASH}")
  if (DOES_GIT EQUAL 0)
    execute_process(COMMAND git status -suno
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      OUTPUT_VARIABLE GITDIFF)
    if (GITDIFF)
      set(DUECA_GITHASH "-DDUECA_GITHASH=${GITHASH}+mods")
    else()
      set(DUECA_GITHASH "-DDUECA_GITHASH=${GITHASH}")
    endif()
  endif()

  target_link_libraries(${MODULETARGET} PUBLIC
    ${DUECA_LIBRARIES}
    ${PROJECT_LIBRARIES}
    ${ADDMODULE_LIBRARIES})
  target_compile_options(${MODULETARGET} PUBLIC
    ${DUECA_COMPILEOPTIONS}
    ${PROJECT_COMPILE_FLAGS}
    ${ADDMODULE_COMPILEOPTIONS_PUBLIC}
    PRIVATE ${ADDMODULE_COMPILEOPTIONS} ${DUECA_GITHASH})

  # direct dependencies on modules
  set(OTHERMODULE_INCLUDES)
  if (ADDMODULE_USEMODULES)
    dueca_read_modules(MODULES TMPMODLIST)
  endif()

  foreach(M ${ADDMODULE_USEMODULES})

    # check that this follows the PROJECT/Module pattern
    if (M MATCHES "^\([^/]+\)/\([^/]+\)$")

      if (M IN_LIST TMPMODLIST OR CMAKE_MATCH_2 STREQUAL "comm-objects")
        message(STATUS "${MODULENAME} depends on ${M}")

        # add the dependency on the module, so code is generated in the right order
        add_dependencies(${MODULETARGET} "${CMAKE_MATCH_1}_${CMAKE_MATCH_2}")

        # extend the include path to the source and generated source of this module
        list(APPEND OTHERMODULE_INCLUDES
          "${CMAKE_SOURCE_DIR}/../${CMAKE_MATCH_1}/${CMAKE_MATCH_2}"
          "${CMAKE_BINARY_DIR}/${CMAKE_MATCH_1}/${CMAKE_MATCH_2}")

      else()
        message(STATUS "${MODULENAME} depends on source only ${M}")
        list(APPEND OTHERMODULE_INCLUDES
           "${CMAKE_SOURCE_DIR}/../${CMAKE_MATCH_1}/${CMAKE_MATCH_2}")
      endif()
    else()
      message(FATAL_ERROR
        "For USEMODULES, cannot extract Project/Module structure from ${M}")
    endif()
  endforeach()

  # message(STATUS "ADDMODULE_INCLUDEDIRS ${ADDMODULE_INCLUDEDIRS}")
  target_include_directories(${MODULETARGET} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${PROJECT_INCLUDE_DIRS}
    ${ADDMODULE_INCLUDEDIRS_PUBLIC}
    ${OTHERMODULE_INCLUDES}
    PRIVATE ${ADDMODULE_INCLUDEDIRS}
    )

  # all module targets in parent scope
  set(CURRENT_MODULE ${MODULETARGET} PARENT_SCOPE)

endfunction()
