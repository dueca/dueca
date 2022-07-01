ADD_TEST(reflectory reflectory.x)
find_package(DuecaCodegen)

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}/dueca
  ${CMAKE_SOURCE_DIR}/dueca)

set(DCOSOURCES Object.dco ReflectoryData.dco)
DUECACODEGEN_TARGET(OUTPUT DCO INDUECA DCOSOURCE ${DCOSOURCES})

set(SOURCES
  ReflectoryTest.cxx
  Reflectory.cxx Reflectory.hxx
  ReflectoryBase.cxx ReflectoryBase.hxx
  ReflectoryExceptions.cxx ReflectoryExceptions.hxx
  ReflectoryRemote.cxx ReflectoryRemote.hxx
  ReflectoryLocal.cxx ReflectoryLocal.hxx
  ReflectoryViewBase.cxx ReflectoryViewBase.hxx
  ReflectoryView.hxx)


add_executable(reflectory.x
  ${DCO_OUTPUT_SOURCE}
  ${DCO_OUTPUT_HEADERS}
  ${SOURCES})

target_link_libraries(reflectory.x dueca${STATICSUFFIX})

