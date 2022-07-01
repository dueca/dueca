# -------------------------------------------------------------------   */
#       item            : dusime/gui/gtk3/CMakeLists.txt
#       made by         : Rene' van Paassen
#       date            : 1610xx
#       category        : cmake definition
#       copyright       : (c) 2016 TUDelft-AE-C&S

pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

set(SOURCES GtkTrimView.cxx GtkTrimView.hxx
  DusimeControllerGtk.cxx DusimeControllerGtk.hxx
  ReplayMasterGtk3.cxx ReplayMasterGtk3.hxx
  SnapshotInventoryGtk3.cxx SnapshotInventoryGtk3.hxx)

set(GLADEFILES dusime.glade3 trimcalc_window.glade3
  replay_master_gtk3.ui initials_inventory_gtk3.ui)

configure_file(dueca-dusime-gtk3.pc.in dueca-dusime-gtk3.pc @ONLY)

set(INCDIRS
  ${CMAKE_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}
  "${CMAKE_BINARY_DIR}/dueca"
  "${CMAKE_SOURCE_DIR}/dueca"
  "${CMAKE_BINARY_DIR}/dusime"
  "${CMAKE_SOURCE_DIR}/dusime"
  "${CMAKE_SOURCE_DIR}/dueca/gui/gtk3"
  )

dueca_add_library(dueca-dusime-gtk3
  SOURCES ${SOURCES}
  LINKLIBS dueca-dusime ${GTK3_LIBRARIES} dueca-gtk3
  LINKDIRS ${GTK3_LIBRARY_DIRS}
  INCLUDEDIRS ${INCDIRS}
  COMPILEOPTIONS ${GTK3_CFLAGS})

foreach(S ${SCRIPTLANG})
  dueca_add_library(dueca-dusime-gtk3-${SCRIPT_${S}_SUFFIX}
    SOURCES InitDusimeGtk3.cxx ${STARTLIB_STATIC}
    LINKLIBS dueca-dusime-gtk3 dueca-start-${SCRIPT_${S}_SUFFIX}
    INCLUDEDIRS ${INCDIRS} ${SCRIPT_${S}_INCLUDES}
    COMPILEOPTIONS ${GTK3_CFLAGS} -DSCRIPT_${S})
  set(scriptsuffix "-${SCRIPT_${S}_SUFFIX}")
  configure_file(dueca-dusime-gtk3-ln.pc.in
    dueca-dusime-gtk3-${SCRIPT_${S}_SUFFIX}.pc @ONLY)
  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/dueca-dusime-gtk3-${SCRIPT_${S}_SUFFIX}.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
endforeach()

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/dueca-dusime-gtk3.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

install(FILES ${GLADEFILES}
  DESTINATION ${CMAKE_INSTALL_DATADIR}/dueca)
