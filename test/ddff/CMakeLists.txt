add_test(DDFF ddff.x)
add_test(DDFF_MSGPACK ddff-msgpack.x)
add_test(DDFF_INVENTORY ddff-inventory.x)
add_test(DDFF_SEGMENTS ddff-segments.x)

find_package(DuecaCodegen)

DUECACODEGEN_TARGET(OUTPUT DCO1 INDUECA DCOSOURCE Objectx.dco)

add_executable(ddff.x ddff.cxx)
add_executable(ddff-msgpack.x ddff-msgpack.cxx ${DCO1_OUTPUTS})
add_executable(ddff-inventory.x ddff-inventory.cxx ${DCO1_OUTPUTS})
add_executable(ddff-segments.x ddff-segments.cxx ${DCO1_OUTPUTS})

include_directories(
  ${CMAKE_SOURCE_DIR}/ddff
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}/ddff
  ${CMAKE_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  )

target_link_libraries(ddff.x dueca-ddff${STATICSUFFIX})
target_link_libraries(ddff-msgpack.x dueca-ddff${STATICSUFFIX})
target_compile_options(ddff-msgpack.x PRIVATE -DDUECA_CONFIG_MSGPACK)
target_link_libraries(ddff-inventory.x dueca-ddff${STATICSUFFIX})
target_compile_options(ddff-inventory.x PRIVATE -DDUECA_CONFIG_MSGPACK)
target_link_libraries(ddff-segments.x dueca-ddff${STATICSUFFIX})
target_compile_options(ddff-segments.x PRIVATE -DDUECA_CONFIG_MSGPACK)
