add_test(LINEARSYSTEM linearsystem.x)

pkg_check_modules(EIGEN REQUIRED eigen3)

include_directories(${CMAKE_SOURCE_DIR}/extra)
#execute_process(
#  COMMAND bash ${CMAKE_BINARY_DIR}/scripts/dueca-config --docbuildlibs --extra --nomain
#  OUTPUT_VARIABLE DUECA_LDFLAGS
#  OUTPUT_STRIP_TRAILING_WHITESPACE)

add_executable(linearsystem.x linearsystem.cxx)
set_target_properties(linearsystem.x PROPERTIES
  COMPILE_FLAGS ${EIGEN_CFLAGS})
target_link_libraries(linearsystem.x dueca-extra${STATICSUFFIX})
