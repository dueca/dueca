add_test(ASYNCQUEUE asynclist.x)

find_package(Threads REQUIRED)

include_directories(
  ${CMAKE_BINARY_DIR}
  ${CMAKE_BINARY_DIR}/dueca
  ${CMAKE_SOURCE_DIR}/dueca)

add_executable(asynclist.x asynclist.cxx)
target_link_libraries(asynclist.x ${CMAKE_THREAD_LIBS_INIT})

