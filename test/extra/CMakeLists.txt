add_test(INTERPOLATOR InterpTest.x)
add_test(STEPS StepsTest.x)
add_test(POLYNOMIALN PolynomialNTest.x)
add_test(CIRCULARPOLY CircularWithPolyTest.x)
add_test(CIRCULAR CircularTest.x)
add_test(CALIB_RANGING InputCalibratorRangingTest.x)

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}/dueca
  ${CMAKE_SOURCE_DIR}/dueca)


#execute_process(COMMAND
#  bash ${CMAKE_BINARY_DIR}/scripts/dueca-config --docbuildlibs --nomain
#  OUTPUT_VARIABLE DUECA_LDFLAGS
#  OUTPUT_STRIP_TRAILING_WHITESPACE)

add_executable(InterpTest.x InterpTest.cxx)
target_link_libraries(InterpTest.x dueca-extra${STATICSUFFIX})
add_executable(StepsTest.x StepsTest.cxx)
target_link_libraries(StepsTest.x dueca-extra${STATICSUFFIX} fmt::fmt)
add_executable(PolynomialNTest.x PolynomialNTest.cxx)
target_link_libraries(PolynomialNTest.x dueca-extra${STATICSUFFIX})
add_executable(CircularWithPolyTest.x CircularWithPolyTest.cxx)
target_link_libraries(CircularWithPolyTest.x dueca-extra${STATICSUFFIX})
add_executable(CircularTest.x CircularTest.cxx)
target_link_libraries(CircularTest.x dueca-extra${STATICSUFFIX})
add_executable(InputCalibratorRangingTest.x InputCalibratorRangingTest.cxx)
target_link_libraries(InputCalibratorRangingTest.x dueca-extra${STATICSUFFIX})