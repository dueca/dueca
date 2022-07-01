# -------------------------------------------------------------------
#       item            : udpcom/CMakeLists.txt
#       made by         : Rene' van Paassen
#       date            : 1610xx
#       category        : cmake definition
#       copyright       : (c) 2017 TUDelft-AE-C&S

include(CheckSymbolExists)
find_package(DuecaAddLibrary)

pkg_check_modules(LIBSSL REQUIRED openssl)
set(CMAKE_REQUIRED_DEFINITIONS ${CMAKE_THREAD_LIBS_INIT})
set(CMAKE_REQUIRED_LIBRARIES ${LIBSSL_LIBRARIES})
check_include_file_cxx(simple-websocket-server/server_ws.hpp
  HAVE_SIMPLE_WEBSOCKET_SERVER_SERVER_WS_HPP)
check_include_file_cxx(simple-websocket-server/server_wss.hpp
  HAVE_SIMPLE_WEBSOCKET_SERVER_SERVER_WSS_HPP)

check_symbol_exists(SO_PRIORITY netinet/ip.h SYMBOL_SO_PRIORITY)

find_package(Boost 1.60 COMPONENTS system filesystem REQUIRED)

mylinker_arguments(OUTPUT WEBSOCK_BOOST_LIBRARIES
  LIBLIST ${Boost_LIBRARIES})

set(DCOSOURCES

  UDPPeerAcknowledge.dco UDPPeerInfo.dco UDPPeerJoined.dco
  UDPPeerConfig.dco NetTimingLog.dco NetCapacityLog.dco CycleCounter.dco)

set(HEADERS

  NetTimingLogExtra.hxx UDPPeerConfigExtra.hxx)

set(SOURCES

  NetCommunicator.cxx NetCommunicatorMaster.cxx
  NetCommunicatorPeer.cxx NetCommunicator.hxx
  NetCommunicatorMaster.hxx NetCommunicatorPeer.hxx ConfigBuffer.cxx
  ConfigBuffer.hxx NetCommunicatorExceptions.hxx
  NetCommunicatorExceptions.cxx DuecaNetMaster.cxx DuecaNetMaster.hxx
  DuecaNetPeer.cxx DuecaNetPeer.hxx
  PacketCommunicator.hxx PacketCommunicator.cxx
  WebsockCommunicator.hxx WebsockCommunicator.cxx
  UDPSocketCommunicator.hxx UDPSocketCommunicator.cxx
  CRCcheck.hxx CRCcheck.cxx NetUseOverview.cxx NetUseOverview.hxx
  )

DUECACODEGEN_TARGET(OUTPUT DCO INDUECA DCOSOURCE ${DCOSOURCES})

set(INCDIRS ${CMAKE_BINARY_DIR} ${CMAKE_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}/dueca ${CMAKE_SOURCE_DIR}/dueca
  ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

dueca_add_library(dueca-udp
  SOURCES ${SOURCES} ${DCO_OUTPUT_SOURCE} ${DCO_OUTPUT_HEADERS}
  LINKLIBS dueca ${LIBSSL_LIBRARIES} ${Boost_LIBRARIES}
  LINKDIRS ${LIBSSL_LIBRARY_DIRS} ${Boost_LIBRARY_DIRS}
  INCLUDEDIRS ${INCDIRS})

foreach(S ${SCRIPTLANG})

  dueca_add_library(dueca-udp-${SCRIPT_${S}_SUFFIX}
    SOURCES InitUDPCom.cxx ${DCO_OUTPUT_HEADERS}
    ${STARTLIB_STATIC}
    LINKLIBS dueca-udp dueca-start-${SCRIPT_${S}_SUFFIX}
    dueca-ip-${SCRIPT_${S}_SUFFIX}
    COMPILEOPTIONS -DSCRIPT_${S}
    INCLUDEDIRS ${INCDIRS} ${SCRIPT_${S}_INCLUDES})

  set(scriptsuffix "-${SCRIPT_${S}_SUFFIX}")
  configure_file(dueca-udp-ln.pc.in
    dueca-udp-${SCRIPT_${S}_SUFFIX}.pc @ONLY)
  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/dueca-udp-${SCRIPT_${S}_SUFFIX}.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

endforeach()

install(FILES ${DCO_OUTPUT_HEADERS} ${HEADERS}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dueca/udp)

configure_file(dueca-udp.pc.in dueca-udp.pc @ONLY)
configure_file(dueca-udp-config.h.in dueca-udp-config.h)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/dueca-udp.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

add_subdirectory(gui)
