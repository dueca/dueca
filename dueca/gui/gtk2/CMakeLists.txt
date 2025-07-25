# -------------------------------------------------------------------   */
#       item            : dueca/gui/gtk2/CMakeLists.txt
#       made by         : Rene' van Paassen
#       date            : 1610xx
#       category        : cmake definition
#       copyright       : (c) 2016 TUDelft-AE-C&S

include(LibOpts)

if (APPLE)
  set(INKSCAPE "/Applications/Inkscape.app/Contents/Resources/bin/inkscape")
else()
  find_program(INKSCAPE inkscape)
endif()

if (ALSO_INIT_GLUT)
  find_package(GLUT REQUIRED)
endif()

pkg_check_modules(GTK2 REQUIRED gtk+-2.0)
pkg_check_modules(GLADE2 REQUIRED libglade-2.0)

set(INSTALLHEADERS GtkGladeWindow.hxx GtkCaller.hxx GladeException.hxx)

set(SOURCES GtkDuecaView.cxx GtkCaller.cxx ActivityView.cxx
  TimingViewGtk.cxx GtkDuecaButtons.cxx GtkGladeWindow.cxx GtkHandler.cxx
  LogViewGui.cxx LogView.cxx GladeException.cxx TimingViewGtk.hxx
  ChannelDataMonitorGtk2.cxx ChannelDataMonitorGtk2.hxx
  ChannelOverviewGtk2.cxx ChannelOverviewGtk2.hxx)

set(GLADEFILES tinydueca_if.glade2 activity_view.glade2
  timingview.glade2 common_if.glade2 logview.glade2
  channel_datamonitor.glade2 channel_overview.glade2)

configure_file(dueca-gtk2.pc.in dueca-gtk2.pc @ONLY)

set(INCDIRS ${CMAKE_BINARY_DIR} ${CMAKE_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}/dueca ${CMAKE_SOURCE_DIR}/dueca
  ${GTK2_INCLUDE_DIRS} ${GLADE_INCLUDE_DIRS})

dueca_add_library(dueca-gtk2
  SOURCES ${SOURCES}
  LINKLIBS dueca ${GLADE2_LIBRARIES} ${GTK2_LIBRARIES} ${X11_LIBRARIES}
  ${GLUT_LIBRARIES}
  INCLUDEDIRS ${INCDIRS}
  LINKDIRS ${GLADE2_LIBRARY_DIRS} ${GTK2_LIBRARY_DIRS}
  COMPILEOPTIONS ${GTK2_CFLAGS} ${GLADE2_CFLAGS})

foreach(S ${SCRIPTLANG})
  dueca_add_library(dueca-gtk2-${SCRIPT_${S}_SUFFIX}
    SOURCES InitGtk2.cxx ${STARTLIB_STATIC}
    LINKLIBS dueca-gtk2 dueca-start-${SCRIPT_${S}_SUFFIX}
    INCLUDEDIRS ${INCDIRS} ${SCRIPT_${S}_INCLUDES}
    COMPILEOPTIONS ${GTK2_CFLAGS} ${GLADE2_CFLAGS} -DSCRIPT_${S})
  set(scriptsuffix "-${SCRIPT_${S}_SUFFIX}")
  configure_file(dueca-gtk2-ln.pc.in
    dueca-gtk2-${SCRIPT_${S}_SUFFIX}.pc @ONLY)
  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/dueca-gtk2-${SCRIPT_${S}_SUFFIX}.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
endforeach()

install(FILES ${INSTALLHEADERS}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dueca/dueca/gui/gtk2)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/dueca-gtk2.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

install(FILES ${GLADEFILES}
  DESTINATION ${CMAKE_INSTALL_DATADIR}/dueca)
