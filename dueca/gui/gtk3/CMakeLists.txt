# -------------------------------------------------------------------   */
#       item            : dueca/gui/gtk3/CMakeLists.txt
#       made by         : Rene' van Paassen
#       date            : 1610xx
#       category        : cmake definition
#       copyright       : (c) 2016 TUDelft-AE-C&S

pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

set(INSTALLHEADERS GtkGladeWindow.hxx GtkCaller.hxx GladeException.hxx)

set(SOURCES GtkDuecaView.cxx GtkCaller.cxx ActivityView.cxx
            TimingViewGtk.cxx TimingViewGtk.hxx GtkDuecaButtons.cxx
            GtkGladeWindow.cxx GtkHandler.cxx LogViewGui.cxx
            GladeException.cxx LogView.cxx
            ChannelOverviewGtk3.cxx ChannelOverviewGtk3.hxx
            ChannelDataMonitorGtk3.cxx ChannelDataMonitorGtk3.hxx
            )

set(GLADEFILES tinydueca_if.glade3 activity_view.glade3
  timingview.glade3 common_if.glade3 logview.glade3
  channel_datamonitor.glade3 channel_overview.glade3 )

configure_file(dueca-gtk3.pc.in dueca-gtk3.pc @ONLY)

set(INCDIRS ${CMAKE_BINARY_DIR} ${CMAKE_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}/dueca ${CMAKE_SOURCE_DIR}/dueca)

add_compile_options(${GTK3_CFLAGS})

# script-independent lib
dueca_add_library(dueca-gtk3
  SOURCES ${SOURCES}
  LINKLIBS dueca ${GTK3_LIBRARIES} ${X11_LIBRARIES} ${GLUT_LIBRARIES} ${GL_LIBRARIES}
  LINKDIRS ${GTK3_LIBRARY_DIRS} ${GLUT_LIBRARY_DIRS}
  INCLUDEDIRS ${INCDIRS})

foreach(S ${SCRIPTLANG})
  dueca_add_library(dueca-gtk3-${SCRIPT_${S}_SUFFIX}
    SOURCES InitGtk3.cxx ${STARTLIB_STATIC}
    LINKLIBS dueca-gtk3 dueca-start-${SCRIPT_${S}_SUFFIX}
    INCLUDEDIRS ${INCDIRS} ${SCRIPT_${S}_INCLUDES}
    COMPILEOPTIONS -DSCRIPT_${S})
  set(scriptsuffix "-${SCRIPT_${S}_SUFFIX}")
  configure_file(dueca-gtk3-ln.pc.in
    dueca-gtk3-${SCRIPT_${S}_SUFFIX}.pc @ONLY)
  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/dueca-gtk3-${SCRIPT_${S}_SUFFIX}.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
endforeach()

install(FILES ${INSTALLHEADERS}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dueca/dueca/gui/gtk3)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/dueca-gtk3.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

install(FILES ${GLADEFILES}
  DESTINATION ${CMAKE_INSTALL_DATADIR}/dueca)
