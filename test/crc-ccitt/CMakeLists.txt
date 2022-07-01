add_test(CRCTEST crctest.x)

add_executable(crctest.x crctest.cxx)

include_directories(
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}/udpcom
  ${CMAKE_BINARY_DIR}/udpcom
  ${CMAKE_SOURCE_DIR}/dueca
  ${CMAKE_BINARY_DIR}/dueca
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  )

target_link_libraries(crctest.x dueca-udp${STATICSUFFIX})
