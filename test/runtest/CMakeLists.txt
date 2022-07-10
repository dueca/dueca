# This test folder tries to check out, compile and run DUECA project
# DuecaTestCommunication under an Xvfb X server, driven by a Python
# script

# virtual framebuffer x server
find_program(XVFB NAMES Xvfb)
find_program(WINDOWMANAGER NAMES openbox)
find_program(WMCTRL NAMES wmctrl)
find_package(LibXml2 REQUIRED)

# variables for running
set(DISPLAYSIZE 1024x768)
set(DISPLAY ":88")
set(TIMELIMIT 120s)

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


set(CONTROLFILES tmssolo tmsudptwo tmswsudpfour)

if (PYTHON_HAVE_PIL EQUAL 0 AND
    PYTHON_HAVE_WMCTRL EQUAL 0 AND
    PYTHON_HAVE_PYNPUT EQUAL 0 AND
    PYTHON_HAVE_ATTRS EQUAL 0 AND
    NOT WMCTRL MATCHES ".*-NOTFOUND" AND
    NOT XVFB MATCHES ".*-NOTFOUND" AND
    NOT WINDOWMANAGER MATCHES ".*-NOTFOUND")


  foreach (CONTROLFILE ${CONTROLFILES})

    configure_file(testrun.in test_run_${CONTROLFILE} @ONLY)
    #file(CHMOD ${CMAKE_CURRENT_BINARY_DIR}/test_run_${CONTROLFILE}
    #  PERMISSIONS OWNER_EXECUTE OWNER_READ OWNER_WRITE)

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

# to prepare a control file:
# run a vnc server with the following configuration
#
# .vnc/config
#   geometry=1024x768
#
# .vnc/xstartup
#   unset SESSION_MANAGER
#   unset DBUS_SESSION_BUS_ADDRESS
#
# startup a vnc session to your server, e.g., with vinagre
#
# Then have a clean (no actions) control file, and run
# DISPLAY=:2 python testrunner.py --control cleanfile.xml --learn
# Note: did not work under wayland, somehow dueca window escaped the
# display??
#
# left Ctrl button inserts color checks at the cursor in the sequence
# right Ctrl button inserts snapshots in the sequence
# mouse interaction is recorded
# recording ends when 'Esc' is pressed
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
# Items needed in a build/test environment
#
# - python3-pillow (on ubuntu pip3 install pillow --user)
# - python3-pip
# - xvfb
# - wmctrl (the program)
# - openbox
# - pynput: pip install pynput --user
# - wmctrl: pip install wmctrl --user
# - dueca temp installation:
#   o PATH=/tmp/bin:$PATH
#   o PKG_CONFIG_PATH=/tmp/lib/pkgconfig:${PKG_CONFIG_PATH}
#   o LD_LIBRARY_PATH=/tmp/lib:/tmp/lib64
#   o PYTHONPATH=/tmp/lib/python/site-packages
#
# checking on the screen of the Xvfb
#
#  DISPLAY=:0 xwd -root -silent | convert xwd:- png:/tmp/screenshot.png
#
