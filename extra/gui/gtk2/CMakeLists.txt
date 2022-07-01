# -------------------------------------------------------------------
#       item            : extra/gui/gtk2/CMakeLists.txt
#       made by         : Rene' van Paassen
#       date            : 1610xx
#       category        : cmake definition
#       copyright       : (c) 2016 TUDelft-AE-C&S

pkg_check_modules(GTK2 REQUIRED gtk+-2.0)
pkg_check_modules(GTKGLEXT REQUIRED gtkglext-1.0)

set(SOURCES GtkGLWidgetHelper.cxx GtkGLWidgetHelper.hxx
            GtkOpenGLHelper.cxx GtkOpenGLHelper.hxx)

configure_file(dueca-extra-gtk2.pc.in dueca-extra-gtk2.pc @ONLY)

include_directories(
  ${CMAKE_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}
  "${CMAKE_BINARY_DIR}/dueca"
  "${CMAKE_SOURCE_DIR}/dueca"
  "${CMAKE_SOURCE_DIR}/extra"
  "${CMAKE_SOURCE_DIR}/dueca/gui/gtk2"
  )

dueca_add_library(dueca-extra-gtk2
  SOURCES ${SOURCES}
  LINKLIBS dueca-gtk2 dueca-extra ${GTKGLEXT_LIBRARIES}
  LINKDIRS ${GTKGLEXT_LIBRARY_DIRS}
  COMPILEOPTIONS ${GTK2_CFLAGS} ${GTKGLEXT_CFLAGS})

dueca_add_library(dueca-extra-gtk2-in
  SOURCES InitDuecaExtraGtk2.cxx ${STARTLIB_STATIC}
  LINKLIBS dueca-extra-gtk2 ${GTKGLEXT_LIBRARIES}
  COMPILEOPTIONS ${GTK2_CFLAGS} ${GTKGLEXT_CFLAGS})

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/dueca-extra-gtk2.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

foreach(S ${SCRIPTLANG})
  set(scriptsuffix "-${SCRIPT_${S}_SUFFIX}")
  configure_file(dueca-extra-gtk2-ln.pc.in
    dueca-extra-gtk2-${SCRIPT_${S}_SUFFIX}.pc @ONLY)
  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/dueca-extra-gtk2-${SCRIPT_${S}_SUFFIX}.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
endforeach()

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/dueca-extra-gtk2.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
