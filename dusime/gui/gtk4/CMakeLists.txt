# -------------------------------------------------------------------   */
#       item            : dusime/gui/gtk4/CMakeLists.txt
#       made by         : Rene' van Paassen
#       date            : 221207
#       category        : cmake definition
#       copyright       : (c) 2022 Rene van Paassen

pkg_check_modules(GTK4 REQUIRED gtk4)

set(SOURCES GtkTrimView.cxx GtkTrimView.hxx
  DusimeControllerGtk.cxx DusimeControllerGtk.hxx
  ReplayMasterGtk4.cxx ReplayMasterGtk4.hxx
  SnapshotInventoryGtk4.cxx SnapshotInventoryGtk4.hxx)

set(GLADEFILES dusime-gtk4.ui trimcalc_window-gtk4.ui
  replay_master-gtk4.ui initials_inventory-gtk4.ui)

configure_file(dueca-dusime-gtk4.pc.in dueca-dusime-gtk4.pc @ONLY)

set(INCDIRS
  ${CMAKE_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}
  "${CMAKE_BINARY_DIR}/dueca"
  "${CMAKE_SOURCE_DIR}/dueca"
  "${CMAKE_BINARY_DIR}/dusime"
  "${CMAKE_SOURCE_DIR}/dusime"
  "${CMAKE_SOURCE_DIR}/dueca/gui/gtk4"
  )

dueca_add_library(dueca-dusime-gtk4
  SOURCES ${SOURCES}
  LINKLIBS dueca-dusime ${GTK4_LIBRARIES} dueca-gtk4
  LINKDIRS ${GTK4_LIBRARY_DIRS}
  INCLUDEDIRS ${INCDIRS}
  COMPILEOPTIONS ${GTK4_CFLAGS})

foreach(S ${SCRIPTLANG})
  dueca_add_library(dueca-dusime-gtk4-${SCRIPT_${S}_SUFFIX}
    SOURCES InitDusimeGtk4.cxx ${STARTLIB_STATIC}
    LINKLIBS dueca-dusime-gtk4 dueca-start-${SCRIPT_${S}_SUFFIX}
    INCLUDEDIRS ${INCDIRS} ${SCRIPT_${S}_INCLUDES}
    COMPILEOPTIONS ${GTK4_CFLAGS} -DSCRIPT_${S})
  set(scriptsuffix "-${SCRIPT_${S}_SUFFIX}")
  configure_file(dueca-dusime-gtk4-ln.pc.in
    dueca-dusime-gtk4-${SCRIPT_${S}_SUFFIX}.pc @ONLY)
  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/dueca-dusime-gtk4-${SCRIPT_${S}_SUFFIX}.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
endforeach()

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/dueca-dusime-gtk4.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

install(FILES ${GLADEFILES}
  DESTINATION ${CMAKE_INSTALL_DATADIR}/dueca)
