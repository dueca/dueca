# -------------------------------------------------------------------   */
#       item            : dueca/gui/gtkmm3/CMakeLists.txt
#       made by         : Rene' van Paassen
#       date            : 1610xx
#       category        : cmake definition
#       copyright       : (c) 2016 TUDelft-AE-C&S

pkg_check_modules(GTKMM40 REQUIRED gtkmm-4.0)

set(SOURCES GtkMMGladeWindow.cxx)

configure_file(dueca-gtkmm4.pc.in dueca-gtkmm4.pc @ONLY)

set(INCDIRS
  ${CMAKE_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}
  "${CMAKE_BINARY_DIR}/dueca"
  "${CMAKE_SOURCE_DIR}/dueca"
  "${CMAKE_SOURCE_DIR}/dueca/gui/gtk4")

add_compile_options(${GTKMM40_CFLAGS})

dueca_add_library(dueca-gtkmm4
  SOURCES ${SOURCES}
  LINKLIBS dueca-gtk4 ${GTKMM40_LIBRARIES} ${X11_LIBRARIES}
  LINKDIRS ${GTKMM40_LIBRARY_DIRS}
  INCLUDEDIRS ${INCDIRS})

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/dueca-gtkmm4.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
