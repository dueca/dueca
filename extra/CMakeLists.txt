# -------------------------------------------------------------------
#       item            : extra/CMakeLists.txt
#       made by         : Rene' van Paassen
#       date            : 1610xx
#       category        : cmake definition
#       copyright       : (c) 2016 TUDelft-AE-C&S


configure_file(UniqueFile.hxx.in UniqueFile.hxx)

set(SOURCES
  DuecaGLCanvas.cxx DuecaGLWindow.cxx InterpIndex.hxx randNormal.cxx
  LinearSystem.cxx LimitedLinearSystem.cxx Integrator.cxx
  SingletonPointer.hxx Polynomial.hxx InputCalibrator.hxx
  InputRatioCalibrator.hxx OutputCalibrator.hxx Steps.hxx Steps.cxx
  PolynomialN.cxx StepsN.cxx Inverse.cxx Circular.cxx
  CircularWithPoly.cxx SimpleFunction.cxx StringUtils.cxx
  RigidBody.cxx integrate_euler.cxx integrate_rungekutta.cxx
  OpenGLHelper.cxx UniqueFile.cxx FindFiles.cxx GLSweeper.cxx
  AxisTransforms.hxx AxisTransforms.cxx RvPQuat.hxx)

set(HEADERS DuecaGLCanvas.hxx DuecaGLWindow.hxx InterpIndex.hxx
  InterpTable1.hxx Interpolator1.hxx InterpTable2.hxx
  Interpolator2.hxx InterpTable3.hxx Interpolator3.hxx
  InterpTable4.hxx Interpolator4.hxx randNormal.hxx LinearSystem.hxx
  LimitedLinearSystem.hxx Integrator.hxx SingletonPointer.hxx
  Polynomial.hxx InputCalibrator.hxx InputRatioCalibrator.hxx
  OutputCalibrator.hxx PolynomialN.hxx StepsN.hxx Inverse.hxx
  Circular.hxx CircularWithPoly.hxx Steps.hxx SimpleFunction.hxx
  StringUtils.hxx RigidBody.hxx integrate_euler.hxx
  integrate_rungekutta.hxx ${CMAKE_CURRENT_BINARY_DIR}/UniqueFile.hxx
  FindFiles.hxx OpenGLHelper.hxx ConglomerateFactory.hxx IdentityFunction.hxx
  AxisTransforms.hxx RvPQuat.hxx)

set(INCDIRS
  ${CMAKE_BINARY_DIR} ${CMAKE_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}/dueca ${CMAKE_SOURCE_DIR}/dueca
  ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

dueca_add_library(dueca-extra
  SOURCES ${SOURCES}
  LINKLIBS dueca
  COMPILEOPTIONS ${EIGEN_CFLAGS}
  INCLUDEDIRS ${INCDIRS})

foreach(S ${SCRIPTLANG})
  dueca_add_library(dueca-extra-${SCRIPT_${S}_SUFFIX}
    SOURCES InitExtra.cxx ${STARTLIB_STATIC}
    LINKLIBS dueca-start-${SCRIPT_${S}_SUFFIX} dueca-extra
    COMPILEOPTIONS ${EIGEN_CFLAGS} -DSCRIPT_${S}
    INCLUDEDIRS ${INCDIRS} ${SCRIPT_${S}_INCLUDES})
  set(scriptsuffix "-${SCRIPT_${S}_SUFFIX}")
  configure_file(dueca-extra-ln.pc.in
    dueca-extra-${SCRIPT_${S}_SUFFIX}.pc @ONLY)
  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/dueca-extra-${SCRIPT_${S}_SUFFIX}.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
endforeach()

install(FILES ${HEADERS}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dueca/extra)

configure_file(dueca-extra.pc.in dueca-extra.pc @ONLY)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/dueca-extra.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

add_subdirectory(gui)
