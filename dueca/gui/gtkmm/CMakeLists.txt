# -------------------------------------------------------------------   */
#       item            : dueca/gui/gtkmm/CMakeLists.txt
#       made by         : Rene' van Paassen
#       date            : 1610xx
#       category        : cmake definition
#       copyright       : (c) 2016 TUDelft-AE-C&S

pkg_check_modules(GTKMM24 REQUIRED gtkmm-2.4)
pkg_check_modules(GLADE2 REQUIRED libglade-2.0)

set(SOURCES GtkMMGladeWindow.cxx)

configure_file(dueca-gtkmm.pc.in dueca-gtkmm.pc @ONLY)

set(INCDIRS
  ${CMAKE_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}
  "${CMAKE_BINARY_DIR}/dueca"
  "${CMAKE_SOURCE_DIR}/dueca"
  "${CMAKE_SOURCE_DIR}/dueca/gui/gtk2")

dueca_add_library(dueca-gtkmm
  SOURCES ${SOURCES}
  LINKLIBS dueca dueca-gtk2 ${GTKMM24_LIBRARIES}
  LINKDIRS ${GTKMM24_LIBRARY_DIRS}
  COMPILEOPTIONS ${GTKMM24_CFLAGS} ${GLADE2_CFLAGS}
  INCLUDEDIRS ${INCDIRS})

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/dueca-gtkmm.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
