add_subdirectory(data)

set(PYSCRIPTS new-module dueca-filtmods dueca-listdco
  dueca-gencommh dueca-project dueca-scriptcommands dueca-tagfiles
  dueca-channeldot new-dco)

set(BINSCRIPTS clean-dueca-rundir makemycvsdir moveandnumber
rrunduecaXxf rrundueca rrunduecaX rrunduecaXmwm
rrunduecaXfvwm dueca-copy-project dueca-archivelogfiles)

set(PYAUX CVS.py daux.py modules.py commobjects.py)

# foreach(S ${PYSCRIPTS})
#   # special treatment, otherwise substituting all @xx@ strings
#   execute_process(COMMAND sed -e "s=\@PYTHON2\@=${PYTHON2}="
#     ${CMAKE_CURRENT_SOURCE_DIR}/${S}.in
#     OUTPUT_FILE ${CMAKE_CURRENT_BINARY_DIR}/${S})
#   install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/${S}
#     DESTINATION ${CMAKE_INSTALL_BINDIR})
# endforeach()

configure_file(dueca-config.in dueca-config @ONLY)

install(PROGRAMS ${BINSCRIPTS} ${PYSCRIPTS}
  DESTINATION ${CMAKE_INSTALL_BINDIR})

# holdpackages script if apt and dpkg are present
if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
  find_program(LSB_RELEASE_EXEC lsb_release)
  if(NOT LSB_RELEASE_EXEC)
    message(FATAL_ERROR
      "Could not detect lsb_release executable")
  endif()
  execute_process(COMMAND "${LSB_RELEASE_EXEC}" --short --id
    OUTPUT_VARIABLE LSB_RELEASE_ID
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  if (LSB_RELEASE_ID STREQUAL "Ubuntu" OR
      LSB_RELEASE_ID STREQUAL "Debian" OR
      LSB_RELEASE_ID STREQUAL "Raspbian")
    install(PROGRAMS dueca-holdpackages
      DESTINATION ${CMAKE_INSTALL_SBINDIR})
    message(STATUS "Release ${LSB_RELEASE_ID} debian-based, installing dueca-holdpackages")
  else()
    message(STATUS "Release ${LSB_RELEASE_ID} not listed as debian-based")
  endif()
endif()

install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/dueca-config
  DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES ${PYAUX} DESTINATION ${CMAKE_INSTALL_DATADIR}/dueca)

configure_file(dueca-startlink.py dueca-startlink @ONLY)
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/dueca-startlink
  DESTINATION ${CMAKE_INSTALL_BINDIR})


if (NOT CMAKE_BUILD_TYPE STREQUAL "Debug" AND
     CMAKE_INSTALL_PREFIX STREQUAL "/usr"  )
   install(FILES profile DESTINATION ${SYSCONF_INSTALL_DIR}/dueca)
   install(FILES 90-rtdueca-cpulatency.rules
     DESTINATION ${SYSCONF_INSTALL_DIR}/udev/rules.d)
endif()

if (DUECA_INSTALL_ENVIRONMENT)
  configure_file(dueca-env.sh.in dueca-env.sh @ONLY)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/dueca-env.sh
    DESTINATION ${CMAKE_INSTALL_PREFIX})
endif()

# # also run the python script for extracting all log locations and
# # comments for these
# execute_process(COMMAND ${Python_EXECUTABLE} -c "import pyparsing"
#   RESULT_VARIABLE PYTHON_HAVE_PYPARSING)
# if (PYTHON_HAVE_PYPARSING)
#   message(NOTICE "Missing Python module \"pyparsing\"")
# endif()

# execute_process(COMMAND ${Python_EXECUTABLE} -c "import argparse"
#   RESULT_VARIABLE PYTHON_HAVE_ARGPARSE)
# if (PYTHON_HAVE_ARGPARSE)
#   message(NOTICE "Missing Python module \"argparse\"")
# endif()

# execute_process(COMMAND ${Python_EXECUTABLE} -c "import xlwt"
#   RESULT_VARIABLE PYTHON_HAVE_XLWT)
# if (PYTHON_HAVE_XLWT)
#   message(NOTICE "Missing Python module \"xlwt\" ${PYTHON_HAVE_XLWT} ${Python_COMPILER}")
# endif()

# if ((PYTHON_HAVE_PYPARSING AND PYTHON_HAVE_ARGPARSE) AND PYTHON_HAVE_XLWT)

#   # only create messagelist if tools are here
#   add_custom_command(
#     OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/dueca-messagelist.xlsx
#     COMMAND ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/filterMessages.py
#     --base=${CMAKE_SOURCE_DIR}
#     --output=dueca-messagelist.xlsx
#     WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
#     DEPENDS filterMessages.py)

#   add_custom_target(messagelist ALL
#     DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/dueca-messagelist.xlsx)

#   install(FILES ${CMAKE_CURRENT_BINARY_DIR}/dueca-messagelist.xlsx
#     DESTINATION ${CMAKE_INSTALL_DATADIR}/dueca)

# endif()
