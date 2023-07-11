# This test folder tries to check out, compile and run DUECA project
# DuecaTestCommunication under an Xvfb X server, driven by a Python
# script

# This project generates and transports data. The solo version also
# logs data with the hdf5 logger, logging is checked a bit.

# virtual framebuffer x server
find_program(XVFB NAMES Xvfb)
find_program(XVNCSERVER NAMES vncserver)
find_program(WINDOWMANAGER NAMES openbox)
find_program(WMCTRL NAMES wmctrl)
find_package(LibXml2 REQUIRED)
find_program(H5DUMP NAMES h5dump)

# variables for running
set(DISPLAYSIZE 1024x768)
set(DISPLAY ":88")
set(TIMELIMIT 180s)

# test some supporting python packages
execute_process(COMMAND ${Python_EXECUTABLE} -c "import PIL"
  RESULT_VARIABLE PYTHON_HAVE_PIL)
execute_process(COMMAND ${Python_EXECUTABLE} -c "import wmctrl"
  RESULT_VARIABLE PYTHON_HAVE_WMCTRL)

# pynput expects access to an X server
configure_file(testpynput.in testpynput @ONLY)
# following fails on older CMake!
#file(CHMOD ${CMAKE_CURRENT_BINARY_DIR}/testpynput
#    PERMISSIONS OWNER_EXECUTE OWNER_READ OWNER_WRITE)
execute_process(COMMAND chmod +x ${CMAKE_CURRENT_BINARY_DIR}/testpynput)
execute_process(COMMAND ${CMAKE_CURRENT_BINARY_DIR}/testpynput
  RESULT_VARIABLE PYTHON_HAVE_PYNPUT)
execute_process(COMMAND ${Python_EXECUTABLE} -c "import attr"
  RESULT_VARIABLE PYTHON_HAVE_ATTRS)


set(TESTRUN_BASE "/tmp/tmp.runner" CACHE PATH
  "Location for running test projects")
set(TESTRUN_PROJ
  ${TESTRUN_BASE}/DuecaTestCommunication/DuecaTestCommunication
  CACHE PATH
  "Location with base test project")

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/h5dump-script
  "${H5DUMP} ${TESTRUN_PROJ}/run/solo/solo/datalog.hdf5 >datalog.dmp")
execute_process(COMMAND chmod +x ${CMAKE_CURRENT_BINARY_DIR}/h5dump-script)

# these names should occur in the hdf5 log file produced in the solo/solo
# project
set(LOGGROUPS entry child __parent__ coded drive fixvector limvector
lists map mappedfixvector nestedmap second varvector)

set(CONTROLFILES
  tmssolo tmsudptwo tmswsudpfour replayscenario
)

# run these tests with enough python modules
if (PYTHON_HAVE_PIL EQUAL 0 AND
    PYTHON_HAVE_WMCTRL EQUAL 0 AND
    PYTHON_HAVE_PYNPUT EQUAL 0 AND
    PYTHON_HAVE_ATTRS EQUAL 0 AND
    NOT WMCTRL MATCHES ".*-NOTFOUND" AND
    NOT XVFB MATCHES ".*-NOTFOUND" AND
    NOT WINDOWMANAGER MATCHES ".*-NOTFOUND")

  # remove the hdf log file if it is there
  file(REMOVE ${TESTRUN_PROJ}/run/solo/solo/datalog.hdf5)

  # for the different configurations
  foreach (CONTROLFILE ${CONTROLFILES})

    # configure the script that runs the test
    configure_file(testrun.in test_run_${CONTROLFILE} @ONLY)

    # validate the test input file first
    add_test(NAME TEST_VAL_${CONTROLFILE}
      COMMAND ${LIBXML2_XMLLINT_EXECUTABLE} --schema
      ${CMAKE_SOURCE_DIR}/gitscripts/default/testscenario.xsd
      ${CMAKE_CURRENT_SOURCE_DIR}/${CONTROLFILE}.xml)

    # run the test
    add_test(NAME TEST_RUN_${CONTROLFILE}
      COMMAND ${CMAKE_CURRENT_BINARY_DIR}/test_run_${CONTROLFILE}
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

  endforeach()

  # if H5DUMP is found, convert and check the log file
  if (NOT H5DUMP MATCHES ".*-NOTFOUND")

    add_test(NAME TEST_CONVERT_datalog
      COMMAND ./h5dump-script
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )

    # check that the given log groups are there. Might convert to a better
    # check in the future.
    foreach(LG ${LOGGROUPS})
      add_test(NAME TEST_HDF5_LOG_${LG}
        COMMAND grep "GROUP .${LG}. {" ${CMAKE_CURRENT_BINARY_DIR}/datalog.dmp
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      )
      set_tests_properties(TEST_HDF5_LOG_${LG}
        PROPERTIES DEPENDS TEST_CONVERT_datalog)
    endforeach()

  endif()

else()
  if (NOT PYTHON_HAVE_PIL EQUAL 0)
    message(WARNING "Cannot do runtests, need python3-pillow installed")
  endif()
  if (NOT PYTHON_HAVE_WMCTRL EQUAL 0)
    message(WARNING "Cannot do runtests, need pip install wmctrl --user")
  endif()
  if (NOT PYTHON_HAVE_PYNPUT EQUAL 0)
    message(WARNING "Cannot do runtests, need pip install pynput --user")
  endif()
  if (NOT PYTHON_HAVE_ATTRS EQUAL 0)
    message(WARNING "Cannot do runtests, need pip install attrs --user")
  endif()
  if (XVFB MATCHES ".*-NOTFOUND")
    message(WARNING "Cannot do runtests, need xvfb-run/Xvfb installed")
  endif()
  if (WINDOWMANAGER MATCHES ".*-NOTFOUND")
    message(WARNING "Cannot do runtests, need openbox installed")
  endif()
  if (WMCTRL MATCHES ".*-NOTFOUND")
    message(WARNING "Cannot do runtests, need wmctrl program installed")
  endif()
endif()

if (PYTHON_HAVE_PIL EQUAL 0 AND
    PYTHON_HAVE_WMCTRL EQUAL 0 AND
    PYTHON_HAVE_PYNPUT EQUAL 0 AND
    PYTHON_HAVE_ATTRS EQUAL 0 AND
    NOT XVNCSERVER MATCHES ".*-NOTFOUND" AND
    NOT WINDOWMANAGER MATCHES ".*-NOTFOUND")

  configure_file(createrun.in createrun @ONLY)
  execute_process(COMMAND chmod +x ${CMAKE_CURRENT_BINARY_DIR}/createrun)
  configure_file(viewrun.in viewrun @ONLY)
  execute_process(COMMAND chmod +x ${CMAKE_CURRENT_BINARY_DIR}/viewrun)
  message(STATUS "Created the \"${CMAKE_CURRENT_BINARY_DIR}/createrun\" tool")
  message(STATUS "Created the \"${CMAKE_CURRENT_BINARY_DIR}/viewrun\" tool")

elseif(XVNCSERVER MATCHES ".*-NOTFOUND")
  message(WARNING "No Xnest, cannot create a scenario creation script")
endif()

# User interaction with DUECA windows is read from xml files, and replayed
# through a python script.
#
# to prepare a new xml file, use the createrun script. This requires that
# cmake finds an installed vnc server
#
# Then have a clean (no actions) control file, see cleanscenario.xml and
# run ./test/runtest/createrun cleanscenario.xml
#
# Connect to the window with a vnc client, e.g., remmina, on
# vnc://127.0.0.1:5988
#
# - F1 button inserts color checks at the cursor in the sequence
# - F2 button inserts snapshots in the sequence
# - recording ends when 'Esc' is pressed
# - mouse interaction is recorded and passed to the application
# - other keys are passed to the application
# The recorded control file can be played back on the same window manager
# modify as needed, particularly wait a bit more in "advance", and extend
# the timeout
#
# the window manager affects the window with additional decoration
# size, and possibly (yes you, gnome3 with wayland on F34) a strange offset;
# normally the (0,0) point of the window is the top left corner of the
# content, just below the window-manager added menu bar
#
# adjust if needed with --offset-x, --offset-y, --extra-y
# --extra-y should match the size of the menu bar; clicks there will be
# assigned to the window, and when translated to be relative to the
# window have a negative y value
#
# You can check the resulting file with the viewrun script, same vnc connection
#
# Items needed in a build/test environment
#
# - python3-pillow (on ubuntu pip3 install pillow --user)
# - python3-pip
# - Xvfb
# - vncserver (for recording)
# - wmctrl (the program)
# - openbox (chosen window manager)
# - pynput: pip install pynput --user
# - wmctrl: pip install wmctrl --user
# - dueca temp installation:
#   o PATH=/tmp/bin:$PATH
#   o PKG_CONFIG_PATH=/tmp/lib/pkgconfig:${PKG_CONFIG_PATH}
#   o LD_LIBRARY_PATH=/tmp/lib:/tmp/lib64
#   o PYTHONPATH=/tmp/lib/python/site-packages
