# -------------------------------------------------------------------
#       item            : extra/gui/glut-gui/CMakeLists.txt
#       made by         : Rene' van Paassen
#       date            : 1610xx
#       category        : cmake definition
#       copyright       : (c) 2016 TUDelft-AE-C&S

find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)
find_library(GLUI_LIBRARY glui)
check_include_file_cxx(GL/glui.h HAVE_GL_GLUI_H)
if ((NOT DEFINED HAVE_GL_GLUI_H) OR (NOT GLUI_LIBRARY))
  message(FATAL_ERROR "Cannot find glui development")
endif()

set(SOURCES GlutGuiOpenGLHelper.cxx GlutGuiOpenGLHelper.hxx)

set(INCDIRS ${CMAKE_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}
  "${CMAKE_BINARY_DIR}/dueca"
  "${CMAKE_SOURCE_DIR}/dueca"
  "${CMAKE_SOURCE_DIR}/extra")

dueca_add_library(dueca-extra-glut-gui
  SOURCES ${SOURCES}
  LINKLIBS dueca-extra dueca-glut-gui ${GLUI_LIBRARY} ${OPENGL_LIBRARIES}
  INCLUDEDIRS ${INCDIRS}
  COMPILEOPTIONS ${GLUT_CFLAGS})

configure_file(dueca-extra-glut-gui.pc.in dueca-extra-glut-gui.pc @ONLY)
install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/dueca-extra-glut-gui.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

foreach(S ${SCRIPTLANG})
  dueca_add_library(dueca-extra-glut-gui-${SCRIPT_${S}_SUFFIX}
    SOURCES InitDuecaExtraGlutGui.cxx ${STARTLIB_STATIC}
    LINKLIBS dueca-extra-glut-gui dueca-glut-gui ${SCRIPT_${S}_LIBS}
    COMPILEOPTIONS "-DSCRIPT_${S}"
    INCLUDEDIRS ${INCDIRS} ${SCRIPT_${S}_INCLUDES})
  set(scriptsuffix "-${SCRIPT_${S}_SUFFIX}")
  configure_file(dueca-extra-glut-gui-ln.pc.in
    dueca-extra-glut-gui-${SCRIPT_${S}_SUFFIX}.pc @ONLY)
  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/dueca-extra-glut-gui-${SCRIPT_${S}_SUFFIX}.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
endforeach()

