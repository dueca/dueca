#.rst:
# duecacodegen_target
# -------------------
#
# Add targets for DUECA generated code.
#
# Takes the following input
#
# ``INDUECA`` (flag)
#     Indicate that build is within DUECA, adds -d flag to code generator
#     invocation
# ``NAMESPACE`` (one or more values)
#     Namespace(s) for the DCO object (incompatible with INDUECA), adds a
#     -n ${NAMESPACE} argument to code generator
# ``OUTPUT`` (single value)
#     Prefix for the information results
# ``DCOSOURCE`` (list)
#     List of DCO objects to generate code for
# ``FLAGS`` (list)
#     Additional flags and arguments for compile c++ calls by the code
#     generator, which may be needed to determine size of arrays.
# ``INCLUDEDIRS`` (list)
#     Extra include folders to search when doing c++ calls
#
# result variables:
#
# ``${OUTPUT}_OUTPUTS``
#     List of source and header files generated
# ``${OUTPUT}_OUTPUT_SOURCE``
#     Only the source files
# ``${OUTPUT}_OUTPUT_HEADERS``
#     Only the header files


# message(STATUS "searching in ${CMAKE_SOURCE_DIR}/pycodegen")

find_path(CODEGEN_SOURCE dueca-codegen.py
  PATHS "${CMAKE_SOURCE_DIR}/pycodegen" NO_DEFAULT_PATH)

find_program(DuecaCodegen_EXECUTABLE dueca-codegen
  PATHS /usr/bin /usr/local/bin /opt/local/bin /tmp/bin)
include(CMakeParseArguments)


# message(STATUS "executable ${DuecaCodegen_EXECUTABLE}")
if (NOT DuecaCodegen_EXECUTABLE AND NOT CODEGEN_SOURCE)
  message(FATAL_ERROR "Cannot find dueca-codegen or its development source")
endif()

macro(DUECACODEGEN_TARGET)

    cmake_parse_arguments(DCG
      "INDUECA" "OUTPUT" "NAMESPACE;DCOSOURCE;FLAGS;INCLUDEDIRS" ${ARGN})

    unset(TCFLAGS)
    if(DCG_INDUECA)
      list(APPEND TCFLAGS "-d")
    endif()
    if (DCG_NAMESPACE)
      foreach(N ${DCG_NAMESPACE})
        list(APPEND TCFLAGS "-n${N}")
      endforeach()
    endif()
    if(DCG_INCLUDEDIRS OR DCG_FLAGS)
      list(APPEND TCFLAGS "--")
    endif()
    if(DCG_FLAGS)
      foreach(F ${DCG_FLAGS})
        list(APPEND TCFLAGS "${F}")
      endforeach()
    endif()
    if (DCG_INCLUDEDIRS)
      foreach(I ${DCG_INCLUDEDIRS})
        list(APPEND TCFLAGS "-I${I}")
      endforeach()
    endif()

    # reset list of sources/headers
    unset(DCG_sources)
    unset(DCG_headers)

    # message(STATUS "code generation for ${DCG_DCOSOURCE}")
    foreach(DCO ${DCG_DCOSOURCE})
      string(REPLACE ".dco" ".cxx" SOURCE ${DCO})
      string(REPLACE ".dco" ".hxx" HEADER ${DCO})
      if (DCG_INDUECA OR CODEGEN_SOURCE)
	add_custom_command(OUTPUT
          ${CMAKE_CURRENT_BINARY_DIR}/${HEADER}
          ${CMAKE_CURRENT_BINARY_DIR}/${SOURCE}
          COMMAND ${Python_EXECUTABLE}
	  ${CMAKE_SOURCE_DIR}/pycodegen/dueca-codegen.py ${TCFLAGS} <
          ${CMAKE_CURRENT_SOURCE_DIR}/${DCO}
          DEPENDS ${CMAKE_SOURCE_DIR}/pycodegen/dueca-codegen.py ${DCO}
          COMMENT "[DuecaCodegen][${DCG_OUTPUT}] Code generation ${DCO}"
          WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
      else()
	add_custom_command(OUTPUT
          ${CMAKE_CURRENT_BINARY_DIR}/${HEADER}
          ${CMAKE_CURRENT_BINARY_DIR}/${SOURCE}
          COMMAND ${DuecaCodegen_EXECUTABLE} ${TCFLAGS} <
          ${CMAKE_CURRENT_SOURCE_DIR}/${DCO}
          DEPENDS ${DuecaCodegen_EXECUTABLE} ${DCO}
          COMMENT "[DuecaCodegen][${DCG_OUTPUT}] Code generation ${DCO}"
          WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
      endif()
      list(APPEND DCG_sources ${CMAKE_CURRENT_BINARY_DIR}/${SOURCE})
      list(APPEND DCG_headers ${CMAKE_CURRENT_BINARY_DIR}/${HEADER})
    endforeach()

    set(${DCG_OUTPUT}_OUTPUTS ${DCG_sources};${DCG_headers})
    set(${DCG_OUTPUT}_OUTPUT_SOURCE ${DCG_sources})
    set(${DCG_OUTPUT}_OUTPUT_HEADERS ${DCG_headers})

  endmacro()


include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(DuecaCodegen DEFAULT_MSG
  DuecaCodegen_EXECUTABLE)

