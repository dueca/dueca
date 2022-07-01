
add_executable(locale.x locale.cxx)

add_test(NAME DEF_LOCALE
  COMMAND locale.x)

add_test(NAME ENUS_LOCALE
  COMMAND bash -c "LC_NUMERIC=\"en_US.UTF-8\" ${CMAKE_CURRENT_BINARY_DIR}/locale.x"
  )

add_test(NAME NLNL_LOCALE
  COMMAND bash -c "LC_NUMERIC=\"nl_NL.UTF-8\" ${CMAKE_CURRENT_BINARY_DIR}/locale.x")

set_tests_properties(NLNL_LOCALE
  PROPERTIES WILL_FAIL TRUE)


