#.rst:
# dueca_add_dco
# ----------------
#
# Reads and parses the comm-objects.lst file, and then:
#
# - Adds a target to generate the comm-objects.h file.
# - Determines the module target name matching the current source folder
# - Parses comm-objects.lst to assemble all 'to-be-generated' dco files
#   in a global data structure
# - Create a list of include folders matching the used dco files
# - Create a list of DCO dependencies for this module
#
# Outputs:
#
# ``DCO_INCLUDE_DIRS``
#     Include directories referring to binary and source dirs
# ``DCO_DEPENDS``
#     List of DCO dependencies

function(DUECA_ADD_DCO)

  # treat comm-objects.lst as a configure file, to get cmake rerun if
  # changed
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/comm-objects.lst
    ${CMAKE_CURRENT_BINARY_DIR}/comm-objects.lst COPYONLY)

  # generate a rule for the comm-objects.h
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/comm-objects.h
    ${CMAKE_CURRENT_SOURCE_DIR}/comm-objects.h
    COMMAND ${Python_EXECUTABLE}
    ${DUECA_DATA_DIR}/process-comm-objects.py
    ${CMAKE_CURRENT_BINARY_DIR}/comm-objects.lst ${CMAKE_BINARY_DIR}
    && ln -sf ${CMAKE_CURRENT_BINARY_DIR}/comm-objects.h
    ${CMAKE_CURRENT_SOURCE_DIR}/comm-objects.h
    MAIN_DEPENDENCY ${CMAKE_CURRENT_BINARY_DIR}/comm-objects.lst
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

  # when from COMMOBJECTS folder, figure out which project not to add
  # as a dependency
  if(CMAKE_CURRENT_SOURCE_DIR MATCHES ".*/\([^/]+\)/comm-objects$")
    set(OWNPRJ "${CMAKE_MATCH_1}")
    set(DFLTPRJ "${CMAKE_MATCH_1}")
    set(BASE_TARGET "${CMAKE_MATCH_1}_comm-objects")
  else()
    set(OWNPRJ )
    if (CMAKE_CURRENT_SOURCE_DIR MATCHES ".*/\([^/]+\)/\([^/]+\)$")
      set(DFLTPRJ "${CMAKE_MATCH_1}")
      set(BASE_TARGET "${CMAKE_MATCH_1}_${CMAKE_MATCH_2}")
    endif()
  endif()

  # symbolic link for comm-objects.h in source dir, to include proper
  # one per dir
  #add_custom_command(
  #  COMMAND ${CMAKE_COMMAND} -E create_symlink
  #  ${CMAKE_CURRENT_BINARY_DIR}/comm-objects.h
  #  ${CMAKE_CURRENT_SOURCE_DIR}/comm-objects.h
  #  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/comm-objects.h
  #  MAIN_DEPENDENCY ${CMAKE_CURRENT_BINARY_DIR}/comm-objects.h)

  # get current list of DCO projects
  get_property(DCOPRJ_LIST GLOBAL PROPERTY DCOPRJ_LIST_PROPERTY)

  # read the comm-objects.lst file
  file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/comm-objects.lst" LINES)

  # run through all, and organize
  foreach(D IN LISTS LINES)

    if (D MATCHES "^[ \t]*$")
      # message(STATUS "empty line")
    elseif(D MATCHES "^[ \t]*#(.*)$")
      # message(STATUS "comment ${CMAKE_MATCH_1}")
    elseif(D MATCHES
        "^[ \t]*(([^ \t/]+)/)?comm-objects/([^ \t.]+)\\.dco[ \t]*#?.*$")
      message(STATUS
        "RESULT ${CMAKE_MATCH_2};${CMAKE_MATCH_3}")
      set(PRJ ${CMAKE_MATCH_2})
      set(DCO ${CMAKE_MATCH_3})

      if (NOT PRJ)
        message(STATUS
          "Using default project name \"${DFLTPRJ}\" for dco ${DCO}")
        set(PRJ "${DFLTPRJ}")
      endif()

      # check that the DCO file exists
      if (NOT EXISTS
          "${CMAKE_SOURCE_DIR}/../${PRJ}/comm-objects/${DCO}.dco")
        message(FATAL_ERROR
          "Missing dco file ${PRJ}/comm-objects/${DCO}.dco, need refresh?")
      endif()

      # find the project in the global DCODIR list
      list(FIND DCOPRJ_LIST ${PRJ} PROJECT_FOUND)
      if (PROJECT_FOUND EQUAL -1)
        list(APPEND DCOPRJ_LIST ${PRJ})
        set_property(GLOBAL PROPERTY DCOPRJ_LIST_PROPERTY ${DCOPRJ_LIST})
      endif()

      # update the local list for this module
      list(FIND DCOPRJ_LOCAL ${PRJ} PROJECT_FOUND)
      if (PROJECT_FOUND EQUAL -1 AND NOT (OWNPRJ EQUAL PRJ))
        # remember we have a dco dependency on this project
        list(APPEND DCOPRJ_LOCAL ${PRJ})

        # add include directories for the module
        list(APPEND DCO_INCLUDE_DIRS
          "${CMAKE_BINARY_DIR}/${PRJ}/comm-objects")
        # include directories, source too, because of "extra" files there
        list(APPEND DCO_INCLUDE_DIRS
          "${CMAKE_SOURCE_DIR}/../${PRJ}/comm-objects")
        # and make a list of DCO target names
        list(APPEND DCO_DEPENDS ${PRJ}_comm-objects)
      endif()

      # check the list of used DCO's there
      get_property(DCO_${PRJ}_SOURCES GLOBAL PROPERTY
        DCO_${PRJ}_SOURCES_PROPERTY)
      list(FIND DCO_${PRJ}_SOURCES ${DCO} DCO_FOUND)

      # add the specific dco to the list if needed
      if (DCO_FOUND EQUAL -1)
        list(APPEND DCO_${PRJ}_SOURCES ${DCO})
        set_property(GLOBAL PROPERTY DCO_${PRJ}_SOURCES_PROPERTY
          ${DCO_${PRJ}_SOURCES})
        #message(STATUS "DCO_${PRJ}_SOURCES ${DCO_${PRJ}_SOURCES}")
      endif()

      # specify that this comm-objects.h depends on the given generated
      # header
      set_property(SOURCE ${CMAKE_CURRENT_BINARY_DIR}/comm-objects.h
        APPEND PROPERTY OBJECT_DEPENDS
        ${CMAKE_BINARY_DIR}/${PRJ}/comm-objects/${DCO}.hxx)

    else()
      message(STATUS
        "RESULT ${CMAKE_MATCH_1};${CMAKE_MATCH_2}")
      message(FATAL_ERROR "Failed analysis of dco line ${D}")
    endif()
  endforeach()

  #message(STATUS
  #  "DCO to include ${DCO_INCLUDE_DIRS}")
  #message(STATUS
  #  "DCO dependencies ${DCO_DEPENDS}")
  # push these variable to the calling environment
  set(DCO_INCLUDE_DIRS ${DCO_INCLUDE_DIRS} PARENT_SCOPE)
  set(DCO_DEPENDS ${DCO_DEPENDS}  PARENT_SCOPE)

endfunction()
