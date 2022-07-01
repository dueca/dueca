# -------------------------------------------------------------------
#       item            : extra/gui/gtk3/CMakeLists.txt
#       made by         : Rene' van Paassen
#       date            : 1610xx
#       category        : cmake definition
#       copyright       : (c) 2016 TUDelft-AE-C&S

pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

set(INSTALLHEADERS

  DuecaGLWidget.hxx DuecaGtkInteraction.hxx DuecaGLGtk3Window.hxx)

set(SOURCES

  DuecaGtkInteraction.cxx DuecaGtkInteraction.hxx
  )

if (GTK3_VERSION VERSION_GREATER "3.15.999")
  set(SOURCES

    ${SOURCES}
    DuecaGLWidget.hxx DuecaGLWidget.cxx
    DuecaGLGtk3Window.hxx DuecaGLGtk3Window.cxx
    )
else()
  message(WARNING "Gtk ${GTK3_VERSION} version too old, no GL area")
endif()

configure_file(dueca-extra-gtk3.pc.in dueca-extra-gtk3.pc @ONLY)

set(INCDIRS
  ${CMAKE_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}
  "${CMAKE_BINARY_DIR}/dueca"
  "${CMAKE_SOURCE_DIR}/dueca"
  "${CMAKE_SOURCE_DIR}/dueca/gui/gtk3"
  )

dueca_add_library(dueca-extra-gtk3
  SOURCES ${SOURCES}
  LINKLIBS dueca-gtk3 dueca-extra
  COMPILEOPTIONS ${GTK3_CFLAGS} # ${GTKGLEXT_CFLAGS}
  LINKDIRS ${GTK3_LIBRARY_DIRS})

install(FILES ${INSTALLHEADERS}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dueca/extra/gui/gtk3)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/dueca-extra-gtk3.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

