include(LibOpts)

set(DCOSOURCES

  )

set(SOURCES
  DDFFLogger.cxx DDFFLogger.hxx
  EntryWatcher.cxx EntryWatcher.hxx
  )

set(INSTALLHEADERS

  )

DUECACODEGEN_TARGET(OUTPUT DCO INDUECA DCOSOURCE ${DCOSOURCES})

set(INCDIRS
  ${CMAKE_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}/dueca
  ${CMAKE_SOURCE_DIR}/dueca
  ${CMAKE_BINARY_DIR}/dusime
  ${CMAKE_SOURCE_DIR}/dusime
  ${CMAKE_BINARY_DIR}/hdf5utils
  ${CMAKE_SOURCE_DIR}/hdf5utils
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR})

dueca_add_library(dueca-ddfflog
  SOURCES ${SOURCES} ${DCO_OUTPUT_SOURCE} ${DCO_OUTPUT_HEADERS}
  LINKLIBS dueca dueca-dusime dueca-ddff
  INCLUDEDIRS ${INCDIRS})

foreach(S ${SCRIPTLANG})
  dueca_add_library(dueca-ddfflog-${SCRIPT_${S}_SUFFIX}
    SOURCES InitDDFFLog.cxx ${DCO_OUTPUT_HEADERS} ${STARTLIB_STATIC}
    LINKLIBS dueca-ddfflog dueca-ddff dueca-start-${SCRIPT_${S}_SUFFIX}
    INCLUDEDIRS ${INCDIRS} ${SCRIPT_${S}_INCLUDES}
    COMPILEOPTIONS "-DSCRIPT_${S}")
  set(scriptsuffix "-${SCRIPT_${S}_SUFFIX}")
  configure_file(dueca-ddfflog-ln.pc.in 
    dueca-ddfflog-${SCRIPT_${S}_SUFFIX}.pc @ONLY)
  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/dueca-ddfflog-${SCRIPT_${S}_SUFFIX}.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
endforeach()

install(FILES ${INSTALLHEADERS}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dueca/hdf5utils)

configure_file(dueca-ddfflog.pc.in dueca-ddfflog.pc @ONLY)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/dueca-ddfflog.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
