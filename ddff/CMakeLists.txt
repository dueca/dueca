# -------------------------------------------------------------------
#       item            : ddff/CMakeLists.txt
#       made by         : Rene' van Paassen
#       date            : 212022
#       category        : cmake definition
#       copyright       : (c) 2021 TUDelft-AE-C&S

add_subdirectory(pyddff)

find_package(DuecaAddLibrary)

set(DCOSOURCES

  IndexPoint.dco IndexMark.dco)

set(HEADERS DDFFExceptions.hxx FileHandler.hxx FileStreamRead.hxx
  FileStreamWrite.hxx ControlBlock.hxx AQMTMessageBufferAlloc.hxx
  FileWithInventory.hxx DDFFDCOWriteFunctor.hxx DDFFDCOReadFunctor.hxx
  DDFFDCOMetaFunctor.hxx ddff_ns.h DDFFMessageBuffer.hxx
  FileWithSegments.hxx DDFFDataRecorder.hxx)

set(SOURCES

  DDFFMessageBuffer.hxx DDFFMessageBuffer.cxx
  FileHandler.cxx FileHandler.hxx
  FileStreamRead.cxx FileStreamRead.hxx
  FileStreamWrite.cxx FileStreamWrite.hxx
  DDFFExceptions.cxx DDFFExceptions.hxx
  ControlBlock.cxx ControlBlock.hxx
  AQMTMessageBufferAlloc.hxx
  FileWithInventory.hxx FileWithInventory.cxx
  DDFFDCOWriteFunctor.hxx DDFFDCOWriteFunctor.cxx
  DDFFDCOReadFunctor.hxx DDFFDCOReadFunctor.cxx
  DDFFDCOMetaFunctor.hxx DDFFDCOMetaFunctor.cxx
  FileWithSegments.hxx FileWithSegments.cxx
  DDFFDataRecorder.hxx DDFFDataRecorder.cxx
  )

DUECACODEGEN_TARGET(OUTPUT DCO INDUECA NAMESPACE "ddff" DCOSOURCE ${DCOSOURCES})

set(INCDIRS ${CMAKE_BINARY_DIR} ${CMAKE_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}/dueca ${CMAKE_SOURCE_DIR}/dueca
  ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

dueca_add_library(dueca-ddff
  SOURCES ${SOURCES} ${DCO_OUTPUT_SOURCE} ${DCO_OUTPUT_HEADERS}
  LINKLIBS dueca dueca-udp
  INCLUDEDIRS ${INCDIRS})

install(FILES ${DCO_OUTPUT_HEADERS} ${HEADERS}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dueca/ddff)

configure_file(dueca-ddff.pc.in dueca-ddff.pc @ONLY)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/dueca-ddff.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

