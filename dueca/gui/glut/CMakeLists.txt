# -------------------------------------------------------------------   */
#       item            : dueca/gui/glut/CMakeLists.txt
#       made by         : Rene' van Paassen
#       date            : 1610xx
#       category        : cmake definition
#       copyright       : (c) 2016 TUDelft-AE-C&S

# library search helper
include(LibOpts)

find_package(GLUT REQUIRED)
find_package(OpenGL REQUIRED)

check_function_exists(glutMainLoopEvent HAVE_GLUTMAINLOOPEVENT)
if (NOT HAVE_GLUTMAINLOOPEVENT)
  message(FATAL_ERROR "No glutmainloop event; need freeglut headers")
endif()

set(SOURCES GlutHandler.cxx GlutProtocol.cxx)

configure_file(dueca-glut.pc.in dueca-glut.pc @ONLY)

set(INCDIRS ${CMAKE_BINARY_DIR} ${CMAKE_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}/dueca ${CMAKE_SOURCE_DIR}/dueca)

dueca_add_library(dueca-glut
  SOURCES ${SOURCES}
  LINKLIBS dueca ${GLUT_LIBRARIES} ${OPENGL_LIBRARIES} ${X11_LIBRARIES}
  INCLUDEDIRS ${INCDIRS})
configure_file(dueca-glut.pc.in dueca-glut.pc @ONLY)
install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/dueca-glut.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

foreach(S ${SCRIPTLANG})

  dueca_add_library(dueca-glut-${SCRIPT_${S}_SUFFIX}
    SOURCES InitGlut.cxx ${STARTLIB_STATIC}
    LINKLIBS dueca-glut dueca-start-${SCRIPT_${S}_SUFFIX}
      ${GLUT_LIBRARIES} ${OPENGL_LIBRARIES} ${X11_LIBRARIES}
    INCLUDEDIRS ${INCDIRS} ${SCRIPT_${S}_INCLUDES}
    COMPILEOPTIONS -DSCRIPT_${S})
  set(scriptsuffix "-${SCRIPT_${S}_SUFFIX}")
  configure_file(dueca-glut-ln.pc.in
    dueca-glut-${SCRIPT_${S}_SUFFIX}.pc @ONLY)
  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/dueca-glut-${SCRIPT_${S}_SUFFIX}.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

endforeach()

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/dueca-glut.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
