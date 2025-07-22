# -------------------------------------------------------------------
#       item            : extra/gui/glfw/MakeLists.txt
#       made by         : Rene' van Paassen
#       date            : 250721
#       category        : cmake definition
#       copyright       : (c) 2025 Rene van Paassen


set(SOURCES DuecaGLFWWindow.cxx DuecaGLFWWindow.hxx)

pkg_check_modules(GLFW glfw3 REQUIRED)

configure_file(dueca-extra-glfw.pc.in dueca-extra-glfw.pc @ONLY)

dueca_add_library(dueca-extra-glfw
  SOURCES ${SOURCES}
  LINKLIBS dueca-extra
  COMPILEOPTIONS ${GLFW_CFLAGS} # ${GTKGLEXT_CFLAGS}
  LINKDIRS ${GLFW_LIBRARY_DIRS})

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/dueca-extra-glfw.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

install(FILES DuecaGLFWWindow.hxx
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dueca/extra/gui/glfw)

