# -------------------------------------------------------------------
#       item            : extra/gui/gtk4/CMakeLists.txt
#       made by         : Rene' van Paassen
#       date            : 1610xx
#       category        : cmake definition
#       copyright       : (c) 2016 TUDelft-AE-C&S
#       copyright       : (c) 2024 Rene van Paassen

pkg_check_modules(GTK4 REQUIRED gtk4)

set(INSTALLHEADERS

  DuecaGLWidget.hxx DuecaGtkInteraction.hxx DuecaGLGtk4Window.hxx)

set(SOURCES

  DuecaGtkInteraction.cxx DuecaGtkInteraction.hxx
    DuecaGLWidget.hxx DuecaGLWidget.cxx
    DuecaGLGtk4Window.hxx DuecaGLGtk4Window.cxx
)

configure_file(dueca-extra-gtk4.pc.in dueca-extra-gtk4.pc @ONLY)

set(INCDIRS
  ${CMAKE_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}
)

dueca_add_library(dueca-extra-gtk4
  SOURCES ${SOURCES}
  LINKLIBS dueca-gtk4 dueca-extra
  COMPILEOPTIONS ${GTK4_CFLAGS}
  LINKDIRS ${GTK4_LIBRARY_DIRS})

install(FILES ${INSTALLHEADERS}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dueca/extra/gui/gtk4)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/dueca-extra-gtk4.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

