# -------------------------------------------------------------------   */
#       item            : dueca/gui/glut-gui/CMakeLists.txt
#       made by         : Rene' van Paassen
#       date            : 1610xx
#       category        : cmake definition
#       copyright       : (c) 2016 TUDelft-AE-C&S

# library search helper
#find_package(DuecaAddLibrary)
#include(LibOpts)

find_package(OpenGL REQUIRED)
find_library(GLUI_LIBRARY glui)
check_include_file_cxx(GL/glui.h HAVE_GL_GLUI_H)
check_include_file_cxx(glui.h HAVE_GLUI_H)
if ( ( (NOT DEFINED HAVE_GL_GLUI_H) AND
       (NOT DEFINED HAVE_GLUI_H) ) OR (NOT GLUI_LIBRARY))
  message(FATAL_ERROR "Cannot find glui development")
endif()

#mylinker_arguments(OUTPUT dueca_glut_gui_libs LIBLIST ${GLUT_LIBRARIES})

set(SOURCES GluiHandler.cxx GluiProtocol.cxx )

set(INCDIRS ${CMAKE_BINARY_DIR} ${CMAKE_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}/dueca ${CMAKE_SOURCE_DIR}/dueca)

dueca_add_library(dueca-glut-gui
  SOURCES ${SOURCES}
  LINKLIBS dueca ${GLUI_LIBRARY} ${GLUT_LIBRARIES} ${OPENGL_LIBRARIES}
  INCLUDEDIRS ${INCDIRS}
  COMPILEOPTIONS ${GLUT_CFLAGS})
configure_file(dueca-glut-gui.pc.in dueca-glut-gui.pc @ONLY)
install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/dueca-glut-gui.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

foreach(S ${SCRIPTLANG})

  dueca_add_library(dueca-glut-gui-${SCRIPT_${S}_SUFFIX}
    SOURCES InitGlutGui.cxx ${STARTLIB_STATIC}
    LINKLIBS dueca-glut-gui dueca-start-${SCRIPT_${S}_SUFFIX}
    INCLUDEDIRS ${INCDIRS} ${SCRIPT_${S}_INCLUDES}
    COMPILEOPTIONS -DSCRIPT_${S})
  set(scriptsuffix "-${SCRIPT_${S}_SUFFIX}")
  configure_file(dueca-glut-gui-ln.pc.in
    dueca-glut-gui-${SCRIPT_${S}_SUFFIX}.pc @ONLY)
  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/dueca-glut-gui-${SCRIPT_${S}_SUFFIX}.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

endforeach()


