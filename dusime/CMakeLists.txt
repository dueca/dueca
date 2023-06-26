# -------------------------------------------------------------------   */
#       item            : dusime/CMakeLists.txt
#       made by         : Rene' van Paassen
#       date            : 1610xx
#       category        : cmake definition
#       copyright       : (c) 2016 TUDelft-AE-C&S

find_package(DuecaCodegen)


set(DCOSOURCES Snapshot.dco XmlSnapshot.dco IncoMode.dco
  IncoNotice.dco IncoSpec.dco IncoVariable.dco IndexValuePair.dco
  ReplayCommand.dco ReplayReport.dco IncoVarType.dco)

set(SOURCES SimulationModule.cxx SimulationModule.hxx
  HardwareModule.cxx HardwareModule.hxx DusimeModule.cxx
  DusimeModule.hxx RTWModule.cxx RTWModule.hxx 
  IncoCalculator.cxx Interval.cxx IntervalCalculation.cxx
  IncoCollaborator.cxx TrimId.cxx TrimId.hxx TrimLink.hxx TrimLink.cxx
  TrimView.cxx TrimView.hxx TrimSummary.cxx IncoVariableWork.cxx
  IncoVariableWork.hxx DusimeController.cxx DusimeController.hxx
  IncoRole.hxx IncoRole.cxx ReplayFiler.hxx ReplayFiler.cxx
  DUSIMEExceptions.hxx DUSIMEExceptions.cxx ReplayMaster.hxx
  ReplayMaster.cxx SnapshotInventory.hxx SnapshotInventory.cxx )

set(HEADERS SimulationModule.hxx RTWModule.hxx HardwareModule.hxx
  DusimeModule.hxx dusime.h IncoTable.hxx
  DusimeController.hxx SnapshotExtra.hxx
  XmlSnapshotExtra.hxx IncoNoticeExtra.hxx IncoRole.hxx
  IncoVariableExtra.hxx ReplayFiler.hxx
  DUSIMEExceptions.hxx DataRecorder.hxx)

duecacodegen_target(OUTPUT DCO INDUECA DCOSOURCE ${DCOSOURCES})

set(INCDIRS ${CMAKE_BINARY_DIR} ${CMAKE_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}/dueca ${CMAKE_SOURCE_DIR}/dueca
  ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

dueca_add_library(dueca-dusime
  SOURCES ${SOURCES} ${DCO_OUTPUT_SOURCE} ${DCO_OUTPUT_HEADERS}
  LINKLIBS dueca dueca-ddff
  INCLUDEDIRS ${INCDIRS}
  COMPILEOPTIONS ${EIGEN_CFLAGS})

foreach(S ${SCRIPTLANG})
  dueca_add_library(dueca-dusime-${SCRIPT_${S}_SUFFIX}
    SOURCES InitDusime.cxx ${DCO_OUTPUT_HEADERS} ${STARTLIB_STATIC}
    LINKLIBS dueca-start-${SCRIPT_${S}_SUFFIX} dueca-dusime
    INCLUDEDIRS ${INCDIRS} ${SCRIPT_${S}_INCLUDES}
    COMPILEOPTIONS ${EIGEN_CFLAGS} -DSCRIPT_${S})
  set(scriptsuffix "-${SCRIPT_${S}_SUFFIX}")
  configure_file(dueca-dusime-ln.pc.in
    dueca-dusime-${SCRIPT_${S}_SUFFIX}.pc @ONLY)
  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/dueca-dusime-${SCRIPT_${S}_SUFFIX}.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
endforeach()

install(FILES ${HEADERS}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dueca/dusime)

install(FILES ${DCO_OUTPUT_HEADERS}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dueca/dusime)

configure_file(dueca-dusime.pc.in dueca-dusime.pc @ONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/dueca-dusime.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

if (BUILD_DMODULES)
  add_subdirectory(gui)
endif()
