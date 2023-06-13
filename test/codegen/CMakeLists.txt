# generate data for test unpack with msgpack
set(MSGPACK_DATA PupilRemoteGaze2)

# all data files
foreach(D ${MSGPACK_DATA})
  #add_custom_command(
  #  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${D}.msgpack
  #  COMMAND ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/${D}.py
  #  COMMEND "[msgpack][${D}] Generating msgpack data"
  #  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
  add_test(${D}.msgpack ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/${D}.py)
endforeach()

add_test(CODEGEN1 test1.x)
add_test(CODEGEN2 test2.x)
add_test(CODEGEN3 test3.x)
add_test(CODEGEN4 test4.x)
add_test(CODEGEN5 test5.x)
add_test(CODEGEN6 test6.x)
add_test(CODEGEN7 test7.x)
add_test(CODEGEN8 test8.x)
add_test(CODEGEN9 test9.x)
add_test(CODEGEN10 test10.x)
add_test(CODEGEN11 test11.x)
add_test(CODEGEN12 test12.x)

add_test(MSGPACK msgpack.x)
add_test(MSGPACK2 msgpack2.x)
add_test(MSGPACK3 msgpack3.x)
add_test(MSGPACKX msgpackx.x)

set_tests_properties(MSGPACKX PROPERTIES DEPENDS PupilRemoteGaze2.msgpack)

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}/dueca
  ${CMAKE_SOURCE_DIR}/dueca)

find_package(DuecaCodegen)

DUECACODEGEN_TARGET(OUTPUT DCO1 INDUECA DCOSOURCE Object1.dco)
DUECACODEGEN_TARGET(OUTPUT DCO2 INDUECA DCOSOURCE Object2.dco)
DUECACODEGEN_TARGET(OUTPUT DCO3 INDUECA DCOSOURCE Object3.dco)
DUECACODEGEN_TARGET(OUTPUT DCO4 INDUECA DCOSOURCE Object4.dco)
DUECACODEGEN_TARGET(OUTPUT DCO5 INDUECA DCOSOURCE Object5.dco Object3.dco)
DUECACODEGEN_TARGET(OUTPUT DCO6 INDUECA DCOSOURCE Object6.dco
  INCLUDEDIRS ${CMAKE_CURRENT_SOURCE_DIR})
DUECACODEGEN_TARGET(OUTPUT DCO7 INDUECA DCOSOURCE Object7.dco
  INCLUDEDIRS ${CMAKE_CURRENT_SOURCE_DIR})
DUECACODEGEN_TARGET(OUTPUT DCO8 INDUECA DCOSOURCE Object8.dco
  INCLUDEDIRS ${CMAKE_CURRENT_SOURCE_DIR})
DUECACODEGEN_TARGET(OUTPUT DCO9 INDUECA DCOSOURCE Object9.dco)
DUECACODEGEN_TARGET(OUTPUT DCO10 DCOSOURCE Enum10.dco)
DUECACODEGEN_TARGET(OUTPUT DCO11 DCOSOURCE Object11.dco)
DUECACODEGEN_TARGET(OUTPUT DCO12 DCOSOURCE Object12.dco)

DUECACODEGEN_TARGET(OUTPUT DCOMSG DCOSOURCE PupilRemote2DEllipse.dco
PupilRemoteConfig.dco PupilRemotePupil.dco PupilRemote3DCircle.dco
PupilRemoteGaze2.dco PupilRemoteStatus.dco PupilRemote3DSphere.dco
PupilRemoteGaze.dco)



add_executable(test1.x test1.cxx ${DCO1_OUTPUTS})
add_executable(test2.x test2.cxx ${DCO2_OUTPUTS})
add_executable(test3.x test3.cxx ${DCO3_OUTPUTS})
add_executable(test4.x test4.cxx ${DCO4_OUTPUTS})
add_executable(test5.x test5.cxx ${DCO5_OUTPUTS})
add_executable(test6.x test6.cxx ${DCO6_OUTPUTS} vecsize.hxx)

add_executable(test7.x test7.cxx ${DCO7_OUTPUTS} vecsizee.hxx)

add_executable(test8.x test8.cxx ${DCO8_OUTPUTS} Object8Extra.hxx)
add_executable(test9.x test9.cxx ${DCO9_OUTPUTS})
add_executable(test10.x test10.cxx ${DCO10_OUTPUTS})
add_executable(test11.x test11.cxx ${DCO11_OUTPUTS} ${DCO10_OUTPUTS})
add_executable(test12.x test12.cxx ${DCO12_OUTPUTS})
add_executable(msgpack.x msgpack.cxx ${DCO1_OUTPUTS} ${DCO2_OUTPUTS}
  ${DCO3_OUTPUTS} ${DCO4_OUTPUTS} ${DCO5_OUTPUTS})
add_executable(msgpack2.x msgpack2.cxx ${DCOMSG_OUTPUTS})
add_executable(msgpack3.x msgpack3.cxx)
add_executable(msgpackx.x msgpackx.cxx ${DCOMSG_OUTPUTS})


#execute_process(
#  COMMAND bash ${CMAKE_BINARY_DIR}/scripts/dueca-config --docbuildlibs
#  OUTPUT_VARIABLE DUECA_LDFLAGS
#  OUTPUT_STRIP_TRAILING_WHITESPACE)

include_directories(${CMAKE_SOURCE_DIR}/dueca)

#target_link_libraries(test1.x ${DUECA_LDFLAGS})
#target_link_libraries(test2.x ${DUECA_LDFLAGS})
#target_link_libraries(test3.x ${DUECA_LDFLAGS})
#target_link_libraries(test4.x ${DUECA_LDFLAGS})
#target_link_libraries(test6.x ${DUECA_LDFLAGS})
#target_link_libraries(test7.x ${DUECA_LDFLAGS})
target_link_libraries(test1.x dueca${STATICSUFFIX} dueca-ddff${STATICSUFFIX})
target_link_libraries(test2.x dueca${STATICSUFFIX} dueca-ddff${STATICSUFFIX})
target_link_libraries(test3.x dueca${STATICSUFFIX} dueca-ddff${STATICSUFFIX})
target_link_libraries(test5.x dueca${STATICSUFFIX} dueca-ddff${STATICSUFFIX})
target_link_libraries(test6.x dueca${STATICSUFFIX} dueca-ddff${STATICSUFFIX})
target_link_libraries(test7.x dueca${STATICSUFFIX})
target_link_libraries(test8.x dueca-start-sc)
target_link_libraries(test4.x dueca-start-sc dueca-ddff${STATICSUFFIX})
target_link_libraries(test9.x dueca-start-sc)
target_link_libraries(test10.x dueca${STATICSUFFIX})
target_link_libraries(test11.x dueca${STATICSUFFIX} dueca-ddff${STATICSUFFIX})
target_link_libraries(msgpack.x dueca${STATICSUFFIX} dueca-ddff${STATICSUFFIX})
target_link_libraries(msgpack2.x dueca${STATICSUFFIX} dueca-ddff${STATICSUFFIX})
target_link_libraries(msgpack3.x dueca${STATICSUFFIX} dueca-ddff${STATICSUFFIX})
target_link_libraries(msgpackx.x dueca${STATICSUFFIX} dueca-ddff${STATICSUFFIX})

target_compile_options(test1.x PRIVATE -DDUECA_CONFIG_MSGPACK)
target_compile_options(test2.x PRIVATE -DDUECA_CONFIG_MSGPACK)
target_compile_options(test3.x PRIVATE -DDUECA_CONFIG_MSGPACK)
target_compile_options(test4.x PRIVATE -DDUECA_CONFIG_MSGPACK)
target_compile_options(test5.x PRIVATE -DDUECA_CONFIG_MSGPACK)
target_compile_options(test6.x PRIVATE -DDUECA_CONFIG_MSGPACK)
target_compile_options(test11.x PRIVATE -DDUECA_CONFIG_MSGPACK)
target_compile_options(msgpack.x PRIVATE -DDUECA_CONFIG_MSGPACK)
target_compile_options(msgpack2.x PRIVATE -DDUECA_CONFIG_MSGPACK)
target_compile_options(msgpack3.x PRIVATE -DDUECA_CONFIG_MSGPACK)
target_compile_options(msgpackx.x PRIVATE -DDUECA_CONFIG_MSGPACK)

#target_link_libraries(test8.x ${DUECA_LDFLAGS})










