# -------------------------------------------------------------------
#       item            : extra/gui/gtkmm/CMakeLists.txt
#       made by         : Rene' van Paassen
#       date            : 1610xx
#       category        : cmake definition
#       copyright       : (c) 2016 TUDelft-AE-C&S
#       copyright       : (c) 2024 Rene van Paassen

pkg_check_modules(GTKMM40 gtkmm-4.0 REQUIRED)

set(SOURCES DuecaGLWidget.hxx DuecaGLWidget.cxx)

set(INSTALLHEADERS DuecaGLWidget.hxx)

configure_file(dueca-extra-gtkmm4.pc.in dueca-extra-gtkmm4.pc @ONLY)

set(INCDIRS ${CMAKE_BINARY_DIR} ${CMAKE_SOURCE_DIR}
  "${CMAKE_BINARY_DIR}/dueca"
  "${CMAKE_SOURCE_DIR}/dueca"
  "${CMAKE_SOURCE_DIR}/extra"
  "${CMAKE_SOURCE_DIR}/dueca/gui/gtkmm4"
  "${CMAKE_SOURCE_DIR}/extra/gui/gtk4")


dueca_add_library(dueca-extra-gtkmm4
  SOURCES ${SOURCES}
  LINKLIBS dueca-extra-gtk4 dueca-gtkmm4 ${GTKMM40_LIBRARIES}
  COMPILEOPTIONS ${GTKMM40_CFLAGS}
  LINKDIRS ${GTKMM40_LIBRARY_DIRS}
  INCLUDEDIRS ${INCDIRS})

install(FILES ${INSTALLHEADERS}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dueca/extra/gui/gtkmm4)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/dueca-extra-gtkmm4.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
