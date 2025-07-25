# This test folder tries to check out, compile and run DUECA project
# DuecaTestCommunication under an Xvfb X server, driven by a Python
# script

# This project generates and transports data. The solo version also
# logs data with the hdf5 logger, logging is checked a bit.

# a second system that is tested is SimpleSimulation. This uses
# a grapics window, and multi-player connection is tested.

# virtual framebuffer x server
find_program(XVFB NAMES Xvfb)
find_program(XNEST NAMES Xnest)
find_program(NESTSERVER NAMES ${RUNTEST_SERVER})
find_program(WINDOWMANAGER NAMES openbox)
find_program(WMCTRL NAMES wmctrl)
find_package(LibXml2 REQUIRED)
find_program(H5DUMP NAMES h5dump)
find_program(XMLSTARLET NAMES xmlstarlet)

# variables for running
set(DISPLAYSIZE 1280x1024)
set(DISPLAY ":88")
set(RUNTEST_TIMELIMIT 360 CACHE STRING
  "Timeout limit for runtests")
math(EXPR PYTHONTIME ${RUNTEST_TIMELIMIT})
math(EXPR TIMELIMIT "${RUNTEST_TIMELIMIT} + 10")

# test some supporting python packages
execute_process(COMMAND ${Python_EXECUTABLE} -c "import PIL"
  RESULT_VARIABLE PYTHON_HAVE_PIL)
execute_process(COMMAND ${Python_EXECUTABLE} -c "import wmctrl"
  RESULT_VARIABLE PYTHON_HAVE_WMCTRL)
execute_process(COMMAND ${Python_EXECUTABLE} -c "import msgpack"
   RESULT_VARIABLE PYTHON_HAVE_MSGPACK)

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

# These files (with .xml suffix) define the test scenarios
set(CONTROLFILES
  tmssolo                  # DuecaTestCommunication, one node, various data
  tmsudptwo                # DuecaTestCommunication, two nodes, udp
  tmswsfour                # DuecaTestCommunication, four nodes, websocket
  replayscenario           # DuecaTestCommunication, 1 node, record & replay
  tmsinter                 # DuecaTestCommunication, six processes, inter
  simplesimulation         # SimpleSimulation, 1x, 2 nodes
  simplesimulationmulti    # SimpleSimulation, 3x, inter communications
  triggering               # DuecaTestCommunication, trigger variants
  dtcwebsock               # DuecaTestCommunication, websocket test
  replayscenario_gtk4      # Re-run this, but with gtk4
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

  else()
    message(WARNING "Midding h5dump from hdf5-tools, cannot check log output")
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
  if (NOT PYTHON_HAVE_MSGPACK EQUAL 0)
    message(WARNING "Cannot do runtests, need python3-msgpack installed")
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
    NOT NESTSERVER MATCHES ".*-NOTFOUND" AND
    NOT WINDOWMANAGER MATCHES ".*-NOTFOUND")

  if (${NESTSERVER} EQUAL "xvncserver")
    set(NESTEXTRAARGS "-geometry ${DISPLAYSIZE} -noxstartup -SecurityTypes None")
    set(NESTSERVER_CLEAN "${NESTSERVER} -kill ${DISPLAY}")
  else()
    set(NESTSERVER_CLEAN "kill \${NESTSERVERPID}")
    set(NESTEXTRAARGS "-screen ${DISPLAYSIZE} +extension GLX")
  endif()

  configure_file(createrun.in createrun @ONLY)
  execute_process(COMMAND chmod +x ${CMAKE_CURRENT_BINARY_DIR}/createrun)
  configure_file(viewrun.in viewrun @ONLY)
  execute_process(COMMAND chmod +x ${CMAKE_CURRENT_BINARY_DIR}/viewrun)
  message(STATUS "Created the \"${CMAKE_CURRENT_BINARY_DIR}/createrun\" tool")
  message(STATUS "Created the \"${CMAKE_CURRENT_BINARY_DIR}/viewrun\" tool")

elseif(NESTSERVER MATCHES ".*-NOTFOUND")
  message(WARNING "No ${RUNTEST_SERVER}, cannot create a scenario creation script")
endif()

if(NOT XMLSTARLET MATCHES ".*-NOTFOUND")
  foreach(SCN ${CONTROLFILES})
    add_custom_command(
      OUTPUT ${CMAKE_BINARY_DIR}/${SCN}-clean.xml
      COMMAND ${XMLSTARLET} edit -N x="https://dueca.tudelft.nl/testscenario"
      --delete 'x:scenario/x:actions/x:snap'
      --delete 'x:scenario/x:actions/x:click'
      --delete 'x:scenario/x:actions/x:check'
      --delete 'x:scenario/x:actions/x:key'
      ${SCN}.xml > ${CMAKE_BINARY_DIR}/${SCN}-clean.xml
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      DEPENDS ${SCN}.xml)
    list(APPEND CLNFILES ${CMAKE_BINARY_DIR}/${SCN}-clean.xml)
  endforeach()
endif()

add_custom_target(clnfiles ALL DEPENDS ${CLNFILES})

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
