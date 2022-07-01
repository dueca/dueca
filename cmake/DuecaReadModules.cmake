#.rst
# dueca_read_modules
# ----------------
#
# Read the list of DUECA modules in this project
#
# Takes the following input
#
# * ``MACHINECLASS`` (single value, optional)
#
#   Machine class to read the modules.xml from, otherwise the
#   currently-configured machine class is used
#
# * ``MODULES`` (single value, needed)
#
#   Variable name for the resulting list of modules

include(CMakeParseArguments)

if (CMAKE_VERSION VERSION_LESS "3.12")
  find_program(Python_EXECUTABLE NAMES python3 python python2)
else()
  find_package(Python COMPONENTS Interpreter)
endif()

function(DUECA_READ_MODULES)

  set(options )
  set(oneValueArgs MACHINECLASS MODULES)
  set(multiValueArgs )
  cmake_parse_arguments(RM "${options}" "${oneValueArgs}"
    "${multiValueArgs}" ${ARGN})

  # use a python process to run this
  if (RM_MACHINECLASS)
    execute_process(COMMAND
      ${Python_EXECUTABLE}
      ${DUECA_DATA_DIR}/dueca-list-modules.py
      --projectdir "${CMAKE_SOURCE_DIR}"
      --only-active
      --machineclass ${RM_MACHINECLASS}
      --compact
      RESULT_VARIABLE RESULT
      OUTPUT_VARIABLE _MODULES
      OUTPUT_STRIP_TRAILING_WHITESPACE
      )
  else()
    execute_process(COMMAND
      ${Python_EXECUTABLE}
      ${DUECA_DATA_DIR}/dueca-list-modules.py
      --projectdir "${CMAKE_SOURCE_DIR}"
      --only-active
      --compact
      RESULT_VARIABLE RESULT
      OUTPUT_VARIABLE _MODULES
      OUTPUT_STRIP_TRAILING_WHITESPACE
      )
  endif()

  if (RESULT)
    message(FATAL_ERROR
      "Cannot read modules file ${CMAKE_SOURCE_DIR}/.config/class/${RM_MACHINECLASS}/modules.xml ${RESULT}")
  endif()

  # return the modules as a CMake list
  set(${RM_MODULES} ${_MODULES} PARENT_SCOPE)
endfunction()
