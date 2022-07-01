
configure_file(dueca-extra-x11.pc.in dueca-extra-x11.pc @ONLY)

set(SOURCES BareDuecaGLWindow.cxx BareDuecaGLWindow.hxx)

dueca_add_library(dueca-extra-X11
  SOURCES ${SOURCES}
  LINKLIBS dueca-extra ${GL_LIBRARIES} ${X11_LIBRARIES}
  ${X11_Xext_LIB} ${X11_Xi_LIB}
  LINKDIRS ${GL_LIBRARY_DIRS}
  INCLUDEDIRS ${CMAKE_BINARY_DIR} ${CMAKE_SOURCE_DIR}
  "${CMAKE_BINARY_DIR}/dueca" "${CMAKE_SOURCE_DIR}/dueca"
  "${CMAKE_SOURCE_DIR}/extra")

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/dueca-extra-x11.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

install(FILES BareDuecaGLWindow.hxx
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dueca/extra/gui/X11)
