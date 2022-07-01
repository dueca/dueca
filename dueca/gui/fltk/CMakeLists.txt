# -------------------------------------------------------------------   */
#       item            : dueca/gui/fltk/CMakeLists.txt
#       made by         : Rene' van Paassen
#       date            : 1610xx
#       category        : cmake definition
#       copyright       : (c) 2016 TUDelft-AE-C&S

# library search helper
include(LibOpts)

set(SOURCES FltkHandler.cxx)

set(INCDIRS ${CMAKE_BINARY_DIR} ${CMAKE_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}/dueca ${CMAKE_SOURCE_DIR}/dueca)

dueca_add_library(dueca-fltk
  SOURCES ${SOURCES}
  LINKLIBS dueca ${FLTK_LIBRARIES}
  INCLUDEDIRS ${INCDIRS}
  COMPILEOPTIONS ${FLTK_CFLAGS})
configure_file(dueca-fltk.pc.in dueca-fltk.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/dueca-fltk.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

foreach(S ${SCRIPTLANG})

  dueca_add_library(dueca-fltk-${SCRIPT_${S}_SUFFIX}
    SOURCES InitFltk.cxx ${STARTLIB_STATIC}
    LINKLIBS dueca-start-${SCRIPT_${S}_SUFFIX} dueca-fltk
    INCLUDEDIRS ${INCDIRS} ${SCRIPT_${S}_INCLUDES}
    COMPILEOPTIONS -DSCRIPT_${S})

  set(scriptsuffix "-${SCRIPT_${S}_SUFFIX}")
  configure_file(dueca-fltk-ln.pc.in
    dueca-fltk-${SCRIPT_${S}_SUFFIX}.pc @ONLY)
  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/dueca-fltk-${SCRIPT_${S}_SUFFIX}.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

endforeach()
