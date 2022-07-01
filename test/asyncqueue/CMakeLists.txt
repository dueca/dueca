add_test(ASYNCQUEUE asyncqueue.x)

find_package(Threads REQUIRED)

include_directories(
  ${CMAKE_BINARY_DIR}
  ${CMAKE_BINARY_DIR}/dueca
  ${CMAKE_SOURCE_DIR}/dueca)

add_executable(asyncqueue.x asyncqueue.cxx)
target_link_libraries(asyncqueue.x ${CMAKE_THREAD_LIBS_INIT})

