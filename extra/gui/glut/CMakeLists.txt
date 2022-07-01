# -------------------------------------------------------------------
#       item            : extra/gui/glut/CMakeLists.txt
#       made by         : Rene' van Paassen
#       date            : 1610xx
#       category        : cmake definition
#       copyright       : (c) 2016 TUDelft-AE-C&S

find_package(GLUT REQUIRED)

set(SOURCES GlutOpenGLHelper.cxx GlutOpenGLHelper.hxx)

set(INCDIRS ${CMAKE_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}
  "${CMAKE_BINARY_DIR}/dueca"
  "${CMAKE_SOURCE_DIR}/dueca"
  "${CMAKE_SOURCE_DIR}/extra")

dueca_add_library(dueca-extra-glut
  SOURCES ${SOURCES}
  LINKLIBS dueca-extra dueca-glut
  INCLUDEDIRS ${INCDIRS}
  COMPILEOPTIONS ${GLUT_CFLAGS})
configure_file(dueca-extra-glut.pc.in dueca-extra-glut.pc @ONLY)
install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/dueca-extra-glut.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

foreach(S ${SCRIPTLANG})
  dueca_add_library(dueca-extra-glut-${SCRIPT_${S}_SUFFIX}
    SOURCES InitDuecaExtraGlut.cxx ${STARTLIB_STATIC}
    LINKLIBS dueca-extra-glut ${SCRIPT_${S}_LIBS}
    COMPILEOPTIONS "-DSCRIPT_${S}"
    INCLUDEDIRS ${INCDIRS} ${SCRIPT_${S}_INCLUDES})
  set(scriptsuffix "-${SCRIPT_${S}_SUFFIX}")
  configure_file(dueca-extra-glut-ln.pc.in
    dueca-extra-glut-${SCRIPT_${S}_SUFFIX}.pc @ONLY)
  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/dueca-extra-glut-${SCRIPT_${S}_SUFFIX}.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
endforeach()

