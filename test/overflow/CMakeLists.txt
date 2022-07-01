add_test(OVERFLOW overflow.x)

include_directories(
  ${CMAKE_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}/dueca
  ${CMAKE_SOURCE_DIR}/dueca
  ${CMAKE_BINARY_DIR}/inter
  ${CMAKE_SOURCE_DIR}/inter
  ${CMAKE_BINARY_DIR}/udpcom
  ${CMAKE_SOURCE_DIR}/udpcom
  )

add_executable(overflow.x overflow.cxx)

target_link_libraries(overflow.x dueca-inter${STATICSUFFIX})

