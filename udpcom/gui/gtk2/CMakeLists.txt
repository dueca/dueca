# -------------------------------------------------------------------   */
#       item            : udpcom/gui/gtk3/CMakeLists.txt
#       made by         : Rene' van Paassen
#       date            : 210420
#       category        : cmake definition
#       copyright       : (c) 2021 TUDelft-AE-C&S

pkg_check_modules(GTK2 REQUIRED gtk+-2.0)

set(SOURCES NetUseOverviewGtk2.cxx NetUseOverviewGtk2.hxx)

set(GLADEFILES net_use.glade)

install(FILES ${GLADEFILES}
  DESTINATION ${CMAKE_INSTALL_DATADIR}/dueca)

configure_file(dueca-udp-gtk2.pc.in dueca-udp-gtk2.pc @ONLY)
install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/dueca-udp-gtk2.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)


set(INCDIRS ${CMAKE_BINARY_DIR} ${CMAKE_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}/dueca ${CMAKE_SOURCE_DIR}/dueca
  ${CMAKE_BINARY_DIR}/dueca/gui/gtk2 ${CMAKE_SOURCE_DIR}/dueca/gui/gtk2
  ${CMAKE_BINARY_DIR}/udpcom ${CMAKE_SOURCE_DIR}/udpcom)
add_compile_options(${GTK2_CFLAGS})

dueca_add_library(dueca-udp-gtk2
  SOURCES ${SOURCES}
  LINKLIBS dueca dueca-gtk2 dueca-udp
  ${GTK2_LIBRARIES} ${X11_LIBRARIES} ${GLUT_LIBS} ${GL_LIBRARIES}
  LINKDIRS ${GTK2_LIBRARY_DIRS}
  INCLUDEDIRS ${INCDIRS})

foreach(S ${SCRIPTLANG})
  dueca_add_library(dueca-udp-gtk2-${SCRIPT_${S}_SUFFIX}
    SOURCES InitUDPComGtk2.cxx ${STARTLIB_STATIC}
    LINKLIBS dueca-udp-gtk2 dueca-start-${SCRIPT_${S}_SUFFIX}
    INCLUDEDIRS ${INCDIRS} ${SCRIPT_${S}_INCLUDES}
    COMPILEOPTIONS -DSCRIPT_${S})
  set(scriptsuffix "-${SCRIPT_${S}_SUFFIX}")
  configure_file(dueca-udp-gtk2-ln.pc.in
    dueca-udp-gtk2-${SCRIPT_${S}_SUFFIX}.pc @ONLY)
  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/dueca-udp-gtk2-${SCRIPT_${S}_SUFFIX}.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
endforeach()
