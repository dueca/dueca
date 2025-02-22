# -------------------------------------------------------------------   */
#       item            : udpcom/gui/gtk4/CMakeLists.txt
#       made by         : Rene' van Paassen
#       date            : 210420
#       category        : cmake definition
#       copyright       : (c) 2021 TUDelft-AE-C&S
#       copyright       : (c) 2024 Rene van Paassen

pkg_check_modules(GTK4 REQUIRED gtk4)

set(SOURCES NetUseOverviewGtk4.cxx NetUseOverviewGtk4.hxx)

set(GLADEFILES net_use-gtk4.ui)

install(FILES ${GLADEFILES}
  DESTINATION ${CMAKE_INSTALL_DATADIR}/dueca)

configure_file(dueca-udp-gtk4.pc.in dueca-udp-gtk4.pc @ONLY)
install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/dueca-udp-gtk4.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)


set(INCDIRS ${CMAKE_BINARY_DIR} ${CMAKE_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}/dueca ${CMAKE_SOURCE_DIR}/dueca
  ${CMAKE_BINARY_DIR}/dueca/gui/gtk4 ${CMAKE_SOURCE_DIR}/dueca/gui/gtk4
  ${CMAKE_BINARY_DIR}/udpcom ${CMAKE_SOURCE_DIR}/udpcom)
add_compile_options(${GTK4_CFLAGS})

dueca_add_library(dueca-udp-gtk4
  SOURCES ${SOURCES}
  LINKLIBS dueca dueca-gtk4 dueca-udp
  ${GTK4_LIBRARIES} ${X11_LIBRARIES} ${GLUT_LIBS} ${GL_LIBRARIES}
  LINKDIRS ${GTK4_LIBRARY_DIRS}
  INCLUDEDIRS ${INCDIRS})

foreach(S ${SCRIPTLANG})
  dueca_add_library(dueca-udp-gtk4-${SCRIPT_${S}_SUFFIX}
    SOURCES InitUDPComGtk4.cxx ${STARTLIB_STATIC}
    LINKLIBS dueca-udp-gtk4 dueca-start-${SCRIPT_${S}_SUFFIX}
    INCLUDEDIRS ${INCDIRS} ${SCRIPT_${S}_INCLUDES}
    COMPILEOPTIONS -DSCRIPT_${S})
  set(scriptsuffix "-${SCRIPT_${S}_SUFFIX}")
  configure_file(dueca-udp-gtk4-ln.pc.in
    dueca-udp-gtk4-${SCRIPT_${S}_SUFFIX}.pc @ONLY)
  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/dueca-udp-gtk4-${SCRIPT_${S}_SUFFIX}.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
endforeach()
