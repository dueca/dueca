# -------------------------------------------------------------------
#       item            : extra/gui/gtkmm/CMakeLists.txt
#       made by         : Rene' van Paassen
#       date            : 1610xx
#       category        : cmake definition
#       copyright       : (c) 2016 TUDelft-AE-C&S

pkg_check_modules(GTKMM30 gtkmm-3.0 REQUIRED)

set(SOURCES DuecaGLWidget.hxx DuecaGLWidget.cxx)

set(INSTALLHEADERS DuecaGLWidget.hxx)

configure_file(dueca-extra-gtkmm3.pc.in dueca-extra-gtkmm3.pc @ONLY)

set(INCDIRS ${CMAKE_BINARY_DIR} ${CMAKE_SOURCE_DIR}
  "${CMAKE_BINARY_DIR}/dueca"
  "${CMAKE_SOURCE_DIR}/dueca"
  "${CMAKE_SOURCE_DIR}/extra"
  "${CMAKE_SOURCE_DIR}/dueca/gui/gtkmm3"
  "${CMAKE_SOURCE_DIR}/extra/gui/gtk3")


dueca_add_library(dueca-extra-gtkmm3
  SOURCES ${SOURCES}
  LINKLIBS dueca-extra-gtk3 dueca-gtkmm3 ${GTKMM30_LIBRARIES}
  COMPILEOPTIONS ${GTKMM30_CFLAGS}
  LINKDIRS ${GTKMM30_LIBRARY_DIRS}
  INCLUDEDIRS ${INCDIRS})

install(FILES ${INSTALLHEADERS}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dueca/extra/gui/gtkmm3)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/dueca-extra-gtkmm3.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
