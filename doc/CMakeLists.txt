
add_subdirectory(images)
add_subdirectory(example)

# error message documentation
execute_process(COMMAND ${Python_EXECUTABLE} -c "import pyparsing"
  RESULT_VARIABLE PYTHON_RESULT_PYPARSING)
if (PYTHON_HAVE_PYPARSING)
  message(STATUS "Missing Python module \"pyparsing\"")
endif()

execute_process(COMMAND ${Python_EXECUTABLE} -c "import argparse"
  RESULT_VARIABLE PYTHON_RESULT_ARGPARSE)
if (PYTHON_HAVE_ARGPARSE)
  message(STATUS "Missing Python module \"argparse\"")
endif()

execute_process(COMMAND ${Python_EXECUTABLE} -c "import xlwt"
  RESULT_VARIABLE PYTHON_RESULT_XLWT)
if (PYTHON_HAVE_XLWT)
  message(STATUS "Missing Python module \"xlwt\" ${PYTHON_HAVE_XLWT} ${Python_COMPILER}")
endif()

if (PYTHON_RESULT_PYPARSING EQUAL "0" AND
    PYTHON_RESULT_ARGPARSE EQUAL "0" AND
    PYTHON_RESULT_XLWT EQUAL "0")

  message(STATUS "Creating log message list")

  # only create messagelist if tools are here
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/dueca-messagelist.xlsx
    ${CMAKE_CURRENT_BINARY_DIR}/dueca-messagelist.doc
    COMMAND ${Python_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/../scripts/filterMessages.py
    --base=${CMAKE_SOURCE_DIR}
    --output=dueca-messagelist.xlsx
    --outputdoc=dueca-messagelist.doc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS ../scripts/filterMessages.py)

  add_custom_target(messagelist ALL
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/dueca-messagelist.xlsx)


  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/dueca-messagelist.xlsx
    DESTINATION ${CMAKE_INSTALL_DATADIR}/dueca)

else()

  message(STATUS "Faking log message list")
  message(STATUS "PYTHON_HAVE_PYPARSING ${PYTHON_HAVE_PYPARSING}")
  message(STATUS "PYTHON_HAVE_ARGPARSE ${PYTHON_HAVE_ARGPARSE}")
  message(STATUS "PYTHON_HAVE_XLWT ${PYTHON_HAVE_XLWT}")
  message(STATUS "Python_EXECUTABLE ${Python_EXECUTABLE}")

  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/dueca-messagelist.doc
    COMMAND echo "/** @page loglist Cannot create list of DUECA log messages */" >dueca-messagelist.doc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
endif()

add_custom_target(messagedoc ALL
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/dueca-messagelist.doc)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/api/html/sorttable.js
  COMMAND install -m 644 ${CMAKE_SOURCE_DIR}/3rdparty/sorttable.js
          ${CMAKE_CURRENT_BINARY_DIR}/api/html/sorttable.js
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  DEPENDS doc)

add_custom_target(sorttablejs ALL
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/api/html/sorttable.js)

# convert svg files to png

# images processed here to simplify dependencies
set(PNGDATA

  metafunctor.png dataflowblock.png dataflowdueca.png entity.png
  spaceplane.png threads.png bugfound-badge.png)

# find doxygen
find_package(Doxygen REQUIRED)

# search for all source header files, exclude Extra.hxx ones which are part
# of generated source classes
execute_process(COMMAND find ${CMAKE_SOURCE_DIR}
  -name gtk2 -prune -o
  \( \( -name "*.hxx" -o -name "*.h" \) -a ! -name "*Extra.hxx" \)
  -exec grep -q DUECA_API {} \; -print
  OUTPUT_VARIABLE apifiles1)

# search for all built header files
execute_process(COMMAND find ${CMAKE_SOURCE_DIR} -name "*.dco"
  -exec grep -q DUECA_API {} \; -print
  OUTPUT_VARIABLE apifiles2
  OUTPUT_STRIP_TRAILING_WHITESPACE)

# message("apifiles2 ${apifiles2}")
string(REPLACE "${CMAKE_SOURCE_DIR}" "${CMAKE_BINARY_DIR}"
  apifiles2 "${apifiles2}")
string(REPLACE ".dco" ".hxx" apifiles2 "${apifiles2}")

# prepare for oxy files
string(REPLACE "\n" " \\\n        " apifiles1 "${apifiles1}")
string(REPLACE "\n" " \\\n        " apifiles2 "${apifiles2}")
string(CONCAT apifiles "${apifiles1}" "${apifiles2}")

# local sources
file(GLOB DOCSOURCES
  "*.doc" "*.md" "*.html"
  "${CMAKE_CURRENT_BINARY_DIR}/*.doc"
  "${CMAKE_CURRENT_BINARY_DIR}/*.html")

# these files generally carry the version information & need updating
configure_file(mainpage-inspec.doc.in mainpage-inspec.doc @ONLY)
configure_file(mainpage-api.md.in mainpage-api.md @ONLY)
configure_file(htfoot-inspec.html.in htfoot-inspec.html @ONLY)
configure_file(htfoot-api.html.in htfoot-api.html @ONLY)
configure_file(header-inspec.tex.in header-inspec.tex @ONLY)
configure_file(header-api.tex.in header-api.tex @ONLY)
configure_file(api.oxy.in api.oxy @ONLY)

# doxygen specification
set(DOXYGEN_INPUT ${CMAKE_CURRENT_BINARY_DIR}/api.oxy)
set(DOXYGEN_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/api)

# files with scheme code for script instructions
# for linking the ``scheme file'' with the object or module name
# that generates that file also under Python scripts, the
# dueca objects need their capitalized name (e.g., Environment)
# modules need their module string (e.g., inco-calculator)
set(SCMFILEN

  environment.scm:Environment
  reflective-packer.scm:ReflectivePacker
  reflective-unpacker.scm:ReflectiveUnpacker
  reflective-fill-packer.scm:ReflectiveFillPacker
  reflective-fill-unpacker.scm:ReflectiveFillUnpacker
  unpacker.scm:Unpacker
  packer.scm:Packer
  fill-unpacker.scm:FillUnpacker
  fill-packer.scm:FillPacker
  transport-delay-estimator.scm:TransportDelayEstimator
  dusime.scm:dusime
  inco-calculator.scm:inco-calculator
  packer-set.scm:PackerSet
  packer-manager.scm:PackerManager
  ticker.scm:Ticker
  time-spec.scm:TimeSpec
  priority-spec.scm:PrioritySpec
  object-manager.scm:ObjectManager
  channel-manager.scm:ChannelManager
  timing-view.scm:timing-view
  activity-view.scm:activity-view
  dueca-view.scm:dueca-view
  shared-memory-accessor.scm:ShmAccessor
  ip-multicast-accessor.scm:IPMulticastAccessor
  ip-broadcast-accessor.scm:IPBroadcastAccessor
  ip-two-way.scm:IPTwoWay
  log-view.scm:log-view
  gl-sweeper.scm:gl-sweeper
  channel-replicator-master.scm:channel-replicator-master
  channel-replicator-peer.scm:channel-replicator-peer
 )


# instructions for a dueca executable with all the script
# instructions built in
add_executable(dueca_run.x empty.cxx)

# set of libraries for which we want generated documentation
list(GET SCRIPTLANG 0 FIRSTLANG)

message(STATUS "Generating doc with script language ${FIRSTLANG}")

set(SF ${SCRIPT_${FIRSTLANG}_SUFFIX})
set(DLIBSL

  dueca-extra-${SF} dueca-start-${SF} dueca-dusime-${SF}
  dueca-ip-${SF} dueca-shm-${SF} dueca-glut-${SF}
  dueca-extra-glut-${SF} dueca-inter-${SF} dueca-gtk3-${SF}
  dueca-dusime-gtk3-${SF})

set(DLIBS
  dueca-extra dueca-dusime
  dueca-ip dueca-shm dueca-glut
  dueca-extra-glut dueca-inter dueca-gtk3
  dueca-dusime-gtk3 dueca-extra-X11)

# manual work, to force insertion of the code
set(dlinkflags
  "-L../dueca \
   -L../dueca/gui/gtk3 \
   -L../dueca/gui/glut \
   -L../dusime \
   -L../dusime/gui/gtk3 \
   -L../extra \
   -L../extra/gui/gtk3 \
   -L../extra/gui/glut \
   -L../inter \
   -Wl,${ldundef}InitIp \
   -Wl,${ldundef}InitShm \
   -Wl,${ldundef}InitInter \
   -Wl,${ldundef}InitDusimeGtk3 \
   -Wl,${ldundef}InitDusime \
   -Wl,${ldundef}InitExtraGlut \
   -Wl,${ldundef}InitGlut \
   -Wl,${ldundef}InitGtk3 \
   -Wl,${ldundef}InitExtra")
#   -Wl,${ldundef}InitExtraGtk3 \

if (BUILD_HDF5)
  list(APPEND SCMFILEN
    hdf5-logger.scm:hdf5-logger
    hdf5-replayer.scm:hdf5-replayer )
  list(APPEND DLIBSL dueca-hdf-${SF} )
  list(APPEND DLIBS dueca-hdf )
  set(dlinkflags "${dlinkflags} -L../hdf5utils -Wl,${ldundef}InitHDF5")
endif()

if (BUILD_UDP)
  list(APPEND SCMFILEN
    dueca-net-peer.scm:NetPeer
    dueca-net-master.scm:NetMaster)
  list(APPEND DLIBSL dueca-udp-${SF} )
  list(APPEND DLIBS dueca-udp)
  set(dlinkflags "${dlinkflags} -L../udpcom -Wl,${ldundef}InitUDPCom")
endif()

if (BUILD_WEBSOCK)
  list(APPEND SCMFILEN
    web-sockets-server.scm:web-sockets-server
    config-storage.scm:config-storage)
  list(APPEND DLIBSL dueca-websock-${SF} )
  list(APPEND DLIBS dueca-websock)
  set(dlinkflags "${dlinkflags} -L../websock -Wl,${ldundef}InitWebSock")
endif()

if (BUILD_DDFF)
  list(APPEND DLIBS dueca-ddff)
endif()


# convert to a static version too
set(DLIBSTAT ${DLIBSL})
foreach(LIB ${DLIBS})
  list(APPEND DLIBSTAT "${LIB}${STATICSUFFIX}")
endforeach()

# message(STATUS "${DLIBSTAT}")
# message(STATUS ${dlinkflags})

#if(DUECA_BUILD_SHARED)
  target_link_libraries(dueca_run.x ${DLIBSTAT})
  set_target_properties(dueca_run.x PROPERTIES LINK_FLAGS "${dlinkflags}")
#elseif(DUECA_BUILD_STATIC)

# List of targets
set(SCMFILED)

# create targets for each scm file
foreach(_SCM ${SCMFILEN})

  # split the target instruction
  #message("SPLITTING ${_SCM}")
  string(REGEX REPLACE "^([^:]+):[^:]+$" "\\1" SCM ${_SCM})
  #message("SCM ${SCM}")
  if (${FIRSTLANG} STREQUAL "SCHEME")
    string(REPLACE .scm "" MOD ${SCM})
  else()
    string(REGEX REPLACE "^[^:]+:([^:]+)$" "\\1" MOD ${_SCM})
  endif()
  list(APPEND SCMFILED ${SCM})

  # add the target
  #  add_custom_target(${SCM}
  add_custom_command(
    OUTPUT ${SCM}
    COMMAND DUECA_SCRIPTINSTRUCTIONS=${MOD}
    LD_LIBRARY_PATH=${CMAKE_BINARY_DIR}/dueca:${CMAKE_BINARY_DIR}/extra:${CMAKE_BINARY_DIR}/inter:${CMAKE_BINARY_DIR}/dusime:${CMAKE_BINARY_DIR}/dueca/gui/gtk3:${CMAKE_BINARY_DIR}/dueca/gui/glut:${CMAKE_BINARY_DIR}/extra/gui/glut:${CMAKE_BINARY_DIR}/extra/gui/X11:${CMAKE_BINARY_DIR}/extra/gui/gtk3:${CMAKE_BINARY_DIR}/dusime/gui/gtk3:${CMAKE_BINARY_DIR}/hdf5utils:${CMAKE_BINARY_DIR}/udpcom:${CMAKE_BINARY_DIR}/websock:${CMAKE_BINARY_DIR}/ddff
    DYLD_LIBRARY_PATH=${CMAKE_BINARY_DIR}/dueca:${CMAKE_BINARY_DIR}/extra:${CMAKE_BINARY_DIR}/inter:${CMAKE_BINARY_DIR}/dusime:${CMAKE_BINARY_DIR}/dueca/gui/gtk3:${CMAKE_BINARY_DIR}/dueca/gui/glut:${CMAKE_BINARY_DIR}/extra/gui/glut:${CMAKE_BINARY_DIR}/extra/gui/X11:${CMAKE_BINARY_DIR}/extra/gui/gtk3:${CMAKE_BINARY_DIR}/dusime/gui/gtk3:${CMAKE_BINARY_DIR}/hdf5utils:${CMAKE_BINARY_DIR}/udpcom:${CMAKE_BINARY_DIR}/websock:${CMAKE_BINARY_DIR}/ddff
    ${CMAKE_CURRENT_BINARY_DIR}/dueca_run.x > ${CMAKE_CURRENT_BINARY_DIR}/${SCM}
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/dueca_run.x
    )
endforeach()

add_custom_target(schemefiles
  DEPENDS ${SCMFILED}
)

add_custom_command(
  OUTPUT api/html/index.html
  COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_INPUT}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  DEPENDS ${DOXYGEN_INPUT} ${PNGDATA} schemefiles messagedoc dco_doc
  ${DOCSOURCES}
)

if(NOT ${CMAKE_BUILD_TYPE} STREQUAL "Debug")
  message(STATUS "Build type ${CMAKE_BUILD_TYPE}, adding doc to default target")
  add_custom_target(
    doc ALL
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/api/html/index.html
  )
else()
  message(STATUS
    "Build type ${CMAKE_BUILD_TYPE}, adding doc as explicit target")
  add_custom_target(
    doc
    DEPENDS api/html/index.html
  )
endif()

# each png file becomes a custom target, add as dep to the doc target
# I suspect inkscape on MacOSX may not be started in parallel; creating a dep chain
set(DEPCHAIN "")
set(PREVIOUS)
foreach(D ${PNGDATA})
  message(STATUS "Adding target ${D}, using ${SVG_CONVERSION_COMMAND}")
  string(REPLACE .png .svg SVG ${D})
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/images/${D}"
    COMMAND
    ${SVG_CONVERSION_COMMAND} ${SVG_PNGFILE_ARG}=${D}
    "${CMAKE_CURRENT_SOURCE_DIR}/images/${SVG}"
    MAIN_DEPENDENCY "${CMAKE_CURRENT_SOURCE_DIR}/images/${SVG}"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/images)
  add_custom_target(${D} ALL
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/images/${D}")
  add_dependencies(doc ${D})
endforeach()

# the doc command, with dependency on the doxy input files, generated
# script instruction files and the png images

install(
  DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/api
  DESTINATION ${DUECA_DOC_PATH}/dueca-doc-${dueca_VERSION}
  OPTIONAL)
