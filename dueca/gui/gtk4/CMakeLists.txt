# -------------------------------------------------------------------   */
#       item            : dueca/gui/gtk4/CMakeLists.txt
#       made by         : Rene' van Paassen
#       date            : 1610xx
#       category        : cmake definition
#       copyright       : (c) 2016 TUDelft-AE-C&S

pkg_check_modules(GTK4 REQUIRED gtk4)
if (ALSO_INIT_GLUT)
  find_package(GLUT REQUIRED)
endif()

set(INSTALLHEADERS GtkGladeWindow.hxx GtkCaller.hxx GladeException.hxx)

set(SOURCES GtkDuecaView.cxx GtkCaller.cxx ActivityView.cxx
            TimingViewGtk.cxx TimingViewGtk.hxx GtkDuecaButtons.cxx
            GtkGladeWindow.cxx GtkHandler.cxx LogViewGui.cxx
            GladeException.cxx LogView.cxx
            ChannelOverviewGtk4.cxx ChannelOverviewGtk4.hxx
            ChannelDataMonitorGtk4.cxx ChannelDataMonitorGtk4.hxx
            )

set(GLADEFILES tinydueca_if-gtk4.ui activity_view-gtk4.ui
  timingview-gtk4.ui common_if-gtk4.ui logview-gtk4.ui
  channel_datamonitor-gtk4.ui channel_overview-gtk4.ui )

configure_file(dueca-gtk4.pc.in dueca-gtk4.pc @ONLY)

set(INCDIRS ${CMAKE_BINARY_DIR} ${CMAKE_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}/dueca ${CMAKE_SOURCE_DIR}/dueca)

add_compile_options(${GTK4_CFLAGS})

# script-independent lib
dueca_add_library(dueca-gtk4
  SOURCES ${SOURCES}
  LINKLIBS dueca ${GTK4_LIBRARIES} ${X11_LIBRARIES} ${GLUT_LIBRARIES} ${GL_LIBRARIES}
  LINKDIRS ${GTK4_LIBRARY_DIRS} ${GLUT_LIBRARY_DIRS}
  INCLUDEDIRS ${INCDIRS} ${GTK4_INCLUDE_DIRS})

foreach(S ${SCRIPTLANG})
  dueca_add_library(dueca-gtk4-${SCRIPT_${S}_SUFFIX}
    SOURCES InitGtk4.cxx ${STARTLIB_STATIC}
    LINKLIBS dueca-gtk4 dueca-start-${SCRIPT_${S}_SUFFIX}
    INCLUDEDIRS ${INCDIRS} ${SCRIPT_${S}_INCLUDES}
    COMPILEOPTIONS -DSCRIPT_${S})
  set(scriptsuffix "-${SCRIPT_${S}_SUFFIX}")
  configure_file(dueca-gtk4-ln.pc.in
    dueca-gtk4-${SCRIPT_${S}_SUFFIX}.pc @ONLY)
  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/dueca-gtk4-${SCRIPT_${S}_SUFFIX}.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
endforeach()

install(FILES ${INSTALLHEADERS}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dueca/dueca/gui/gtk4)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/dueca-gtk4.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

install(FILES ${GLADEFILES}
  DESTINATION ${CMAKE_INSTALL_DATADIR}/dueca)
