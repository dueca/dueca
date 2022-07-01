include(CMakeParseArguments)

function(DUECA_GET_CONFIG)

  # process all arguments, accept both -- and non -- forms
  cmake_parse_arguments(CONFIG
    "" "LIBRARIES;COMPILEOPTIONS;SCRIPTLANG" "" ${ARGN})

  set(CLINE)
  foreach(C ${CONFIG_UNPARSED_ARGUMENTS})
    if (${C} MATCHES "^[-][-].*")
      set(NC "${C}")
    else()
      set(NC "--${C}")
    endif()
    string(FIND "${CLINE}" ${NC} ARGIDX)
    if (ARGIDX GREATER -1)
      #message(WARNING "duplicate dueca component ${C}")
    else()
      set(CLINE "${CLINE} ${NC}")
    endif()
  endforeach()

  # message(STATUS "Arguments ${CLINE}")

  execute_process(COMMAND dueca-config --cflags ${CLINE}
    OUTPUT_VARIABLE COMPILEOPTIONS
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE CONFIGFAIL)
  if (CONFIGFAIL)
    message(FATAL_ERROR "failed: dueca-config --cflags ${CLINE}")
  endif()
  separate_arguments(COMPILEOPTIONS)

  execute_process(COMMAND dueca-config --libs ${CLINE}
    OUTPUT_VARIABLE LIBRARIES
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE CONFIGFAIL)
  if (CONFIGFAIL)
    message(FATAL_ERROR "failed: dueca-config --libs ${CLINE}")
  endif()
  separate_arguments(LIBRARIES)

  execute_process(COMMAND dueca-config --scriptlang ${CLINE}
    OUTPUT_VARIABLE DUECA_SCRIPTLANG
    OUTPUT_STRIP_TRAILING_WHITESPACE)

  #message(STATUS "DUECA_LIBRARIES ${LIBRARIES} ${CONFIG_LIBRARIES}")
  #message(STATUS "DUECA_COMPILEOPTIONS ${COMPILEOPTIONS}")

  if (CONFIG_LIBRARIES)
    set (${CONFIG_LIBRARIES} ${LIBRARIES} PARENT_SCOPE)
  endif()
  if (CONFIG_COMPILEOPTIONS)
    set (${CONFIG_COMPILEOPTIONS} ${COMPILEOPTIONS} PARENT_SCOPE)
  endif()
  if (CONFIG_SCRIPTLANG)
    set (${CONFIG_SCRIPTLANG} ${DUECA_SCRIPTLANG} PARENT_SCOPE)
  endif()
endfunction()
