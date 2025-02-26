add_test(DDFF ddff.x)
add_test(DDFF_MSGPACK ddff-msgpack.x)
add_test(DDFF_INVENTORY ddff-inventory.x)
add_test(DDFF_SEGMENTS ddff-segments.x)
find_package(Python3 COMPONENTS Interpreter)

if (Python3_Interpreter_FOUND)
  add_test(NAME DDFF_PYTHON
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/ddff-pyddff.py)

  # for practicing with conversion
  set(DDFF_CONVERT
    ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/ddff/pyddff/ddff_convert.py --verbose)
  set(DATAFILE ${CMAKE_CURRENT_SOURCE_DIR}/recordings-PHLAB-new.ddff)

  add_test(NAME DDFF_CONVERT_INFO1
    COMMAND ${DDFF_CONVERT} info ${DATAFILE})
  add_test(NAME DDFF_CONVERT_INFO2
    COMMAND ${DDFF_CONVERT} info ${DATAFILE} --period one)
  add_test(NAME DDFF_CONVERT_INFO3
    COMMAND ${DDFF_CONVERT} info ${DATAFILE} --inventory)
  add_test(NAME DDFF_CONVERT_INFO4
    COMMAND ${DDFF_CONVERT} info ${DATAFILE} "--streamid=WriteUnified:first blip")

  add_test(NAME DDFF_CONVERT_HDF1
    COMMAND ${DDFF_CONVERT} hdf5 ${DATAFILE})
  add_test(NAME DDFF_CONVERT_HDF2
    COMMAND ${DDFF_CONVERT} hdf5 ${DATAFILE}
    --outfile=convert2.hdf5 --period=one)
  add_test(NAME DDFF_CONVERT_HDF3
    COMMAND ${DDFF_CONVERT} hdf5 ${DATAFILE}
    --outfile=convert3.hdf5 "--streamid=WriteUnified:first blip")
  add_test(NAME DDFF_CONVERT_HDF1z
    COMMAND ${DDFF_CONVERT} hdf5 ${DATAFILE}
    --outfile=convert4.hdf5 --compress=gzip)
  add_test(NAME DDFF_CONVERT_HDF2z
    COMMAND ${DDFF_CONVERT} hdf5 ${DATAFILE}
    --outfile=convert5.hdf5 --period=one --compress=lzf)
  add_test(NAME DDFF_CONVERT_HDF3z
    COMMAND ${DDFF_CONVERT} hdf5 ${DATAFILE} --compress=gzip
    --outfile=convert6.hdf5 "--streamid=WriteUnified:first blip")

endif()

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
