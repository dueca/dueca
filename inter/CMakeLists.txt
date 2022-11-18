find_package(DuecaCodegen)

# need websocket server
check_include_file_cxx(simple-websocket-server/server_ws.hpp
  HAVE_SIMPLE_WEBSOCKET_SERVER_SERVER_WS_HPP)

# optional stuff for websocket connection over ssl
pkg_check_modules(LIBSSL REQUIRED openssl)
find_package(Boost 1.60 COMPONENTS system filesystem REQUIRED)

if (${CMAKE_VERSION} VERSION_LESS "3.14.0")

  # compile checks fail for older cmake
  find_file(HAVE_SIMPLE_WEBSOCKET_SERVER_SERVER_WS_HPP
    simple-websocket-server/server_ws.hpp
    PATHS ${INCLUDE_DIRECTORIES})
  find_file(HAVE_SIMPLE_WEBSOCKET_SERVER_SERVER_WSS_HPP
    simple-websocket-server/server_wss.hpp
    PATHS ${INCLUDE_DIRECTORIES})

else()

  set(CMAKE_REQUIRED_DEFINITIONS ${CMAKE_THREAD_LIBS_INIT})
  set(CMAKE_REQUIRED_LIBRARIES ${LIBSSL_LIBRARIES} Boost::system
    ${CMAKE_THREAD_LIBS_INIT})
  check_include_file_cxx(simple-websocket-server/server_ws.hpp
    HAVE_SIMPLE_WEBSOCKET_SERVER_SERVER_WS_HPP)
  check_include_file_cxx(simple-websocket-server/server_wss.hpp
    HAVE_SIMPLE_WEBSOCKET_SERVER_SERVER_WSS_HPP)

endif()

if (LIBSSL_FOUND AND HAVE_SIMPLE_WEBSOCKET_SERVER_SERVER_WSS_HPP AND
    Boost_FOUND)
  set(WEBSOCK_LIBS ${LIBSSL_LIBRARIES} ${Boost_LIBRARIES})
endif()

set(DCOSOURCES

  ReplicatorConfig.dco ReplicatorPeerInfo.dco ReplicatorInfo.dco
  ReplicatorPeerAcknowledge.dco ReplicatorPeerJoined.dco)

set(SOURCES

  ChannelReplicator.cxx EntryWriter.cxx
  ChannelReplicator.hxx EntryHandler.cxx EntryWriter.hxx
  ChannelReplicatorMaster.cxx EntryHandler.hxx
  ChannelReplicatorMaster.hxx EntryReader.cxx
  ReplicatorConfigExtra.hxx ChannelReplicatorPeer.cxx EntryReader.hxx
  ReplicatorExceptions.cxx ChannelReplicatorPeer.hxx EntryWatcher.cxx
  ReplicatorExceptions.hxx EntryWatcher.hxx
  ReplicatorNamespace.hxx PeerTiming.cxx PeerTiming.hxx)

set(INSTALLHEADERS

  ${CMAKE_CURRENT_BINARY_DIR}/ReplicatorPeerAcknowledge.hxx
  ${CMAKE_CURRENT_BINARY_DIR}/ReplicatorPeerJoined.hxx
  ${CMAKE_CURRENT_BINARY_DIR}/ReplicatorInfo.hxx)

DUECACODEGEN_TARGET(OUTPUT DCO INDUECA DCOSOURCE ${DCOSOURCES})

set(INCDIRS ${CMAKE_BINARY_DIR} ${CMAKE_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}/dueca ${CMAKE_SOURCE_DIR}/dueca
  ${CMAKE_BINARY_DIR}/udpcom ${CMAKE_SOURCE_DIR}/udpcom
  ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

configure_file(dueca-inter-config.h.in dueca-inter-config.h @ONLY)

dueca_add_library(dueca-inter
  SOURCES ${SOURCES} ${DCO_OUTPUT_SOURCE} ${DCO_OUTPUT_HEADERS}
  LINKLIBS dueca-udp ${WEBSOCK_LIBS}
  INCLUDEDIRS ${INCDIRS} )

foreach(S ${SCRIPTLANG})

  dueca_add_library(dueca-inter-${SCRIPT_${S}_SUFFIX}
    SOURCES InitInter.cxx ${DCO_OUTPUT_HEADERS} ${STARTLIB_STATIC}
    LINKLIBS dueca-start-${SCRIPT_${S}_SUFFIX} dueca-inter
    INCLUDEDIRS ${INCDIRS} ${SCRIPT_${S}_INCLUDES}
    COMPILEOPTIONS "-DSCRIPT_${S}")
  set(scriptsuffix "-${SCRIPT_${S}_SUFFIX}")
  configure_file(dueca-inter-ln.pc.in
    dueca-inter-${SCRIPT_${S}_SUFFIX}.pc @ONLY)
  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/dueca-inter-${SCRIPT_${S}_SUFFIX}.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

endforeach()


install(FILES ${INSTALLHEADERS}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dueca/inter)

configure_file(dueca-inter.pc.in dueca-inter.pc @ONLY)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/dueca-inter.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
