find_package(HDF5 REQUIRED COMPONENTS CXX)
include(LibOpts)

mylinker_arguments(OUTPUT HDF5_LINKLIBS LIBLIST ${HDF5_LIBRARIES})

set(DCOSOURCES

  HDFReplayConfig.dco
  )

set(SOURCES

  HDF5Templates.cxx HDF5Templates.hxx HDF5DCOMetaFunctor.hxx
  HDF5DCOWriteFunctor.hxx HDF5DCOMetaFunctor.cxx
  HDF5DCOWriteFunctor.cxx HDF5Logger.cxx HDF5Logger.hxx
  HdfLogNamespace.hxx EntryWatcher.cxx EntryWatcher.hxx
  HDF5DCOReadFunctor.hxx HDF5DCOReadFunctor.cxx HDF5Exceptions.hxx
  HDF5Exceptions.cxx HDF5Replayer.hxx HDF5Replayer.cxx)

set(INSTALLHEADERS

  HDF5Templates.hxx HDF5DCOMetaFunctor.hxx HDF5DCOWriteFunctor.hxx
  HDF5DCOReadFunctor.hxx HDF5Exceptions.hxx HdfLogNamespace.hxx
  ${CMAKE_CURRENT_BINARY_DIR}/HDFReplayConfig.hxx
  HDFLogConfig.hxx HDFLogStatus.hxx)

DUECACODEGEN_TARGET(OUTPUT DCO INDUECA DCOSOURCE ${DCOSOURCES})

set(INCDIRS
  ${CMAKE_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}/dueca
  ${CMAKE_SOURCE_DIR}/dueca
  ${CMAKE_BINARY_DIR}/dusime
  ${CMAKE_SOURCE_DIR}/dusime
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${HDF5_INCLUDE_DIRS})

dueca_add_library(dueca-hdf
  SOURCES ${SOURCES} ${DCO_OUTPUT_SOURCE} ${DCO_OUTPUT_HEADERS}
  LINKLIBS dueca dueca-dusime ${HDF5_CXX_LIBRARIES}
  INCLUDEDIRS ${INCDIRS})

foreach(S ${SCRIPTLANG})
  dueca_add_library(dueca-hdf-${SCRIPT_${S}_SUFFIX}
    SOURCES InitHdf5.cxx ${DCO_OUTPUT_HEADERS} ${STARTLIB_STATIC}
    LINKLIBS dueca-hdf dueca-start-${SCRIPT_${S}_SUFFIX}
    INCLUDEDIRS ${INCDIRS} ${SCRIPT_${S}_INCLUDES}
    COMPILEOPTIONS "-DSCRIPT_${S}")
  set(scriptsuffix "-${SCRIPT_${S}_SUFFIX}")
  configure_file(dueca-hdf-ln.pc.in
    dueca-hdf-${SCRIPT_${S}_SUFFIX}.pc @ONLY)
  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/dueca-hdf-${SCRIPT_${S}_SUFFIX}.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
endforeach()

install(FILES ${INSTALLHEADERS}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dueca/hdf5utils)

configure_file(dueca-hdf.pc.in dueca-hdf.pc @ONLY)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/dueca-hdf.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
