# -------------------------------------------------------------------   */
#       item            : dueca/gui/gtkmm3/CMakeLists.txt
#       made by         : Rene' van Paassen
#       date            : 1610xx
#       category        : cmake definition
#       copyright       : (c) 2016 TUDelft-AE-C&S

pkg_check_modules(GTKMM30 REQUIRED gtkmm-3.0)

set(SOURCES GtkMMGladeWindow.cxx)

configure_file(dueca-gtkmm3.pc.in dueca-gtkmm3.pc @ONLY)

set(INCDIRS
  ${CMAKE_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}
  "${CMAKE_BINARY_DIR}/dueca"
  "${CMAKE_SOURCE_DIR}/dueca"
  "${CMAKE_SOURCE_DIR}/dueca/gui/gtk3")

add_compile_options(${GTKMM30_CFLAGS})

dueca_add_library(dueca-gtkmm3
  SOURCES ${SOURCES}
  LINKLIBS dueca-gtk3 ${GTKMM30_LIBRARIES} ${X11_LIBRARIES}
  LINKDIRS ${GTKMM30_LIBRARY_DIRS}
  INCLUDEDIRS ${INCDIRS})

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/dueca-gtkmm3.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
