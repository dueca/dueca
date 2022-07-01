# -------------------------------------------------------------------   */
#       item            : dusime/gui/gtk2/CMakeLists.txt
#       made by         : Rene' van Paassen
#       date            : 1610xx
#       category        : cmake definition
#       copyright       : (c) 2016 TUDelft-AE-C&S

pkg_check_modules(GTK2 REQUIRED gtk+-2.0)
pkg_check_modules(GLADE2 REQUIRED libglade-2.0)

set(SOURCES GtkTrimView.cxx GtkTrimView.hxx
  DusimeControllerGtk.cxx DusimeControllerGtk.hxx)

set(GLADEFILES dusime.glade2 trimcalc_window.glade2)

configure_file(dueca-dusime-gtk2.pc.in dueca-dusime-gtk2.pc @ONLY)

set(INCDIRS
  ${CMAKE_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}
  "${CMAKE_BINARY_DIR}/dueca"
  "${CMAKE_SOURCE_DIR}/dueca"
  "${CMAKE_BINARY_DIR}/dusime"
  "${CMAKE_SOURCE_DIR}/dusime"
  "${CMAKE_SOURCE_DIR}/dueca/gui/gtk2"
  )

dueca_add_library(dueca-dusime-gtk2
  SOURCES ${SOURCES}
  LINKLIBS dueca-dusime ${GTK2_LIBRARIES} ${GLADE2_LIBRARIES} dueca-gtk2
  LINKDIRS ${GTK2_LIBRARY_DIRS} ${GLADE2_LIBRARY_DIRS}
  INCLUDEDIRS ${INCDIRS}
  COMPILEOPTIONS ${GTK2_CFLAGS} ${GLADE2_CFLAGS})

foreach(S ${SCRIPTLANG})
  dueca_add_library(dueca-dusime-gtk2-${SCRIPT_${S}_SUFFIX}
    SOURCES InitDusimeGtk2.cxx ${STARTLIB_STATIC}
    LINKLIBS dueca-dusime-gtk2 dueca-start-${SCRIPT_${S}_SUFFIX}
    INCLUDEDIRS ${INCDIRS} ${SCRIPT_${S}_INCLUDES}
    COMPILEOPTIONS ${GTK2_CFLAGS} ${GLADE2_CFLAGS} -DSCRIPT_${S})
  set(scriptsuffix "-${SCRIPT_${S}_SUFFIX}")
  configure_file(dueca-dusime-gtk2-ln.pc.in
    dueca-dusime-gtk2-${SCRIPT_${S}_SUFFIX}.pc @ONLY)
  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/dueca-dusime-gtk2-${SCRIPT_${S}_SUFFIX}.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
endforeach()

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/dueca-dusime-gtk2.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

install(FILES ${GLADEFILES}
  DESTINATION ${CMAKE_INSTALL_DATADIR}/dueca)
