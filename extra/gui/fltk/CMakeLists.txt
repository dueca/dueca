# -------------------------------------------------------------------
#       item            : extra/gui/fltk/CMakeLists.txt
#       made by         : Rene' van Paassen
#       date            : 1610xx
#       category        : cmake definition
#       copyright       : (c) 2016 TUDelft-AE-C&S


set(SOURCES FltkOpenGLHelper.cxx FltkOpenGLHelper.hxx
  InitDuecaExtraFltk.cxx)

configure_file(dueca-extra-fltk.pc.in dueca-extra-fltk.pc @ONLY)

dueca_add_library(dueca-extra-fltk
  SOURCES ${SOURCES} ${STARTLIB_STATIC}
  LINKLIBS dueca-extra ${dueca_fltk_libs}
  ${APPLE_COREGRAPHICS_LIBRARY} ${APPLE_COREFOUNDATION_LIBRARY}
  ${OPENGL_LIBRARIES} ${OPENGL_gl_LIBRARY}
  LINKDIRS ${FLTK_LIBRARY_DIRS}
  COMPILEOPTIONS ${FLTK_CFLAGS}
  INCLUDEDIRS
  ${CMAKE_BINARY_DIR} ${CMAKE_SOURCE_DIR}
  "${CMAKE_BINARY_DIR}/dueca" "${CMAKE_SOURCE_DIR}/dueca"
  "${CMAKE_SOURCE_DIR}/extra")

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/dueca-extra-fltk.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
