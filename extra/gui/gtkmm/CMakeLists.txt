# -------------------------------------------------------------------
#       item            : extra/gui/gtkmm/CMakeLists.txt
#       made by         : Rene' van Paassen
#       date            : 1610xx
#       category        : cmake definition
#       copyright       : (c) 2016 TUDelft-AE-C&S

pkg_check_modules(GTKMM24 gtkmm-2.4 REQUIRED)
pkg_check_modules(GTKGLEXT gtkglext-1.0 REQUIRED)

set(SOURCES DuecaGLWidget.hxx DuecaGLWidget.cxx)

set(INSTALLHEADERS DuecaGLWidget.hxx)

configure_file(dueca-extra-gtkmm.pc.in dueca-extra-gtkmm.pc @ONLY)

set(INCDIRS ${CMAKE_BINARY_DIR} ${CMAKE_SOURCE_DIR}
  "${CMAKE_BINARY_DIR}/dueca"
  "${CMAKE_SOURCE_DIR}/dueca"
  "${CMAKE_SOURCE_DIR}/extra"
  "${CMAKE_SOURCE_DIR}/dueca/gui/gtkmm"
  "${CMAKE_SOURCE_DIR}/extra/gui/gtk2")


dueca_add_library(dueca-extra-gtkmm
  SOURCES ${SOURCES}
  LINKLIBS dueca-extra-gtk2 dueca-gtkmm ${GTKMM24_LIBRARIES}
  COMPILEOPTIONS ${GTKMM24_CFLAGS} ${GTKGLEXT_CFLAGS}
  LINKDIRS ${GTKMM24_LIBRARY_DIRS} ${GTKGLEXT_LIBRARY_DIRS}
  INCLUDEDIRS ${INCDIRS})

install(FILES ${INSTALLHEADERS}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dueca/extra/gui/gtkmm)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/dueca-extra-gtkmm.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
