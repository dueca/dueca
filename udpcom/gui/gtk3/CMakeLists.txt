# -------------------------------------------------------------------   */
#       item            : udpcom/gui/gtk3/CMakeLists.txt
#       made by         : Rene' van Paassen
#       date            : 210420
#       category        : cmake definition
#       copyright       : (c) 2021 TUDelft-AE-C&S

pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

set(SOURCES NetUseOverviewGtk3.cxx NetUseOverviewGtk3.hxx)

set(GLADEFILES net_use.ui)

install(FILES ${GLADEFILES}
  DESTINATION ${CMAKE_INSTALL_DATADIR}/dueca)

configure_file(dueca-udp-gtk3.pc.in dueca-udp-gtk3.pc @ONLY)
install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/dueca-udp-gtk3.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)


set(INCDIRS ${CMAKE_BINARY_DIR} ${CMAKE_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}/dueca ${CMAKE_SOURCE_DIR}/dueca
  ${CMAKE_BINARY_DIR}/dueca/gui/gtk3 ${CMAKE_SOURCE_DIR}/dueca/gui/gtk3
  ${CMAKE_BINARY_DIR}/udpcom ${CMAKE_SOURCE_DIR}/udpcom)
add_compile_options(${GTK3_CFLAGS})

dueca_add_library(dueca-udp-gtk3
  SOURCES ${SOURCES}
  LINKLIBS dueca dueca-gtk3 dueca-udp
  ${GTK3_LIBRARIES} ${X11_LIBRARIES} ${GLUT_LIBS} ${GL_LIBRARIES}
  LINKDIRS ${GTK3_LIBRARY_DIRS}
  INCLUDEDIRS ${INCDIRS})

foreach(S ${SCRIPTLANG})
  dueca_add_library(dueca-udp-gtk3-${SCRIPT_${S}_SUFFIX}
    SOURCES InitUDPComGtk3.cxx ${STARTLIB_STATIC}
    LINKLIBS dueca-udp-gtk3 dueca-start-${SCRIPT_${S}_SUFFIX}
    INCLUDEDIRS ${INCDIRS} ${SCRIPT_${S}_INCLUDES}
    COMPILEOPTIONS -DSCRIPT_${S})
  set(scriptsuffix "-${SCRIPT_${S}_SUFFIX}")
  configure_file(dueca-udp-gtk3-ln.pc.in
    dueca-udp-gtk3-${SCRIPT_${S}_SUFFIX}.pc @ONLY)
  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/dueca-udp-gtk3-${SCRIPT_${S}_SUFFIX}.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
endforeach()
