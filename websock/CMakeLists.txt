# -------------------------------------------------------------------
#       item            : websock/CMakeLists.txt
#       made by         : Rene' van Paassen
#       date            : 190103
#       category        : cmake definition
#       copyright       : (c) 2019 TUDelft-AE-C&S

find_package(DuecaAddLibrary)
find_package(DuecaCodegen)

# optionally for websocket over ssl
pkg_check_modules(LIBSSL openssl REQUIRED)
find_package(Boost 1.60 COMPONENTS system filesystem REQUIRED)
if (${CMAKE_VERSION} VERSION_LESS "3.14.0")

  # compile checks fail for this cmake
  find_file(HAVE_SIMPLE_WEBSOCKET_SERVER_SERVER_WS_HPP
    simple-websocket-server/server_ws.hpp
    PATHS ${INCLUDE_DIRECTORIES})
  find_file(HAVE_SIMPLE_WEBSOCKET_SERVER_SERVER_WSS_HPP
    simple-websocket-server/server_wss.hpp
    PATHS ${INCLUDE_DIRECTORIES})
  find_file(HAVE_SIMPLE_WEB_SERVER_SERVER_HTTP_HPP
    simple-web-server/server_http.hpp
    PATHS ${INCLUDE_DIRECTORIES})
  find_file(HAVE_SIMPLE_WEB_SERVER_SERVER_HTTPS_HPP
    simple-web-server/server_https.hpp
    PATHS ${INCLUDE_DIRECTORIES})

else()

  set(CMAKE_REQUIRED_DEFINITIONS ${CMAKE_THREAD_LIBS_INIT})
  set(CMAKE_REQUIRED_LIBRARIES ${LIBSSL_LINK_LIBRARIES} Boost::system
    ${CMAKE_THREAD_LIBS_INIT})

  check_include_file_cxx(simple-websocket-server/server_ws.hpp
    HAVE_SIMPLE_WEBSOCKET_SERVER_SERVER_WS_HPP)
  check_include_file_cxx(simple-websocket-server/server_wss.hpp
    HAVE_SIMPLE_WEBSOCKET_SERVER_SERVER_WSS_HPP)
  check_include_file_cxx(simple-web-server/server_http.hpp
    HAVE_SIMPLE_WEB_SERVER_SERVER_HTTP_HPP)
  check_include_file_cxx(simple-web-server/server_https.hpp
    HAVE_SIMPLE_WEB_SERVER_SERVER_HTTPS_HPP)
endif()

if (NOT HAVE_SIMPLE_WEBSOCKET_SERVER_SERVER_WS_HPP)
  message(FATAL_ERROR "Could not find simple-websocket-server/server_ws.hpp")
endif()
if (NOT HAVE_SIMPLE_WEBSOCKET_SERVER_SERVER_WSS_HPP)
  message(FATAL_ERROR "Could not find simple-websocket-server/server_wss.hpp")
endif()
if (NOT HAVE_SIMPLE_WEB_SERVER_SERVER_HTTP_HPP)
  message(FATAL_ERROR "Could not find simple-web-server/server_http.hpp")
endif()
if (NOT HAVE_SIMPLE_WEB_SERVER_SERVER_HTTPS_HPP)
  message(FATAL_ERROR "Could not find simple-web-server/server_https.hpp")
endif()

mylinker_arguments(OUTPUT WEBSOCK_BOOST_LIBRARIES
   LIBLIST ${Boost_LIBRARIES})

set(DCOSOURCES
  ConfigFileData.dco  ConfigFileRequest.dco  NameSizeDate.dco)

set(SOURCES

  CommonChannelServer.cxx WebSocketsServer.hxx
  CommonChannelServer.hxx WebSocketsServer.cxx
  ConfigStorage.cxx ConfigStorage.hxx)

set(INCDIRS ${CMAKE_BINARY_DIR} ${CMAKE_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}/dueca ${CMAKE_SOURCE_DIR}/dueca
  ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

DUECACODEGEN_TARGET(OUTPUT DCO INDUECA DCOSOURCE ${DCOSOURCES})

dueca_add_library(dueca-websock
  SOURCES ${SOURCES} ${DCO_OUTPUT_SOURCE} ${DCO_OUTPUT_HEADERS}
  LINKLIBS dueca ${LIBSSL_LIBRARIES} ${Boost_LIBRARIES}
  LINKDIRS ${LIBSSL_LIBRARY_DIRS} ${Boost_LIBRARY_DIRS}
  INCLUDEDIRS ${INCDIRS})

foreach(S ${SCRIPTLANG})

  dueca_add_library(dueca-websock-${SCRIPT_${S}_SUFFIX}
    SOURCES InitWebSock.cxx ${DCO_OUTPUT_HEADERS}
    ${STARTLIB_STATIC}
    LINKLIBS dueca-websock dueca-start-${SCRIPT_${S}_SUFFIX}
    COMPILEOPTIONS -DSCRIPT_${S}
    INCLUDEDIRS ${INCDIRS} ${SCRIPT_${S}_INCLUDES})

  set(scriptsuffix "-${SCRIPT_${S}_SUFFIX}")
  configure_file(dueca-websock-ln.pc.in
    dueca-websock-${SCRIPT_${S}_SUFFIX}.pc @ONLY)
  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/dueca-websock-${SCRIPT_${S}_SUFFIX}.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

endforeach()

install(FILES ${DCO_OUTPUT_HEADERS} ${HEADERS}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dueca/websock)

configure_file(dueca-websock.pc.in dueca-websock.pc @ONLY)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/dueca-websock.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

