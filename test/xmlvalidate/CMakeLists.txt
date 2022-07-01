find_program(XMLLINT xmllint)

add_test(NAME lint_policies
  COMMAND ${XMLLINT} --schema
  ${CMAKE_SOURCE_DIR}/gitscripts/default/policies.xsd
  ${CMAKE_SOURCE_DIR}/gitscripts/default/policy_example.xml)

add_test(NAME lint_policies_test
  COMMAND ${XMLLINT} --schema
  ${CMAKE_SOURCE_DIR}/gitscripts/default/policies.xsd
  ${CMAKE_SOURCE_DIR}/test/gitscript/example-policies.xml)

add_test(NAME lint_platform_srs
  COMMAND ${XMLLINT} --schema
  ${CMAKE_SOURCE_DIR}/gitscripts/default/platform.xsd
  ${CMAKE_SOURCE_DIR}/gitscripts/default/platform-srs.xml)

add_test(NAME lint_platform_hmicar
  COMMAND ${XMLLINT} --schema
  ${CMAKE_SOURCE_DIR}/gitscripts/default/platform.xsd
  ${CMAKE_SOURCE_DIR}/gitscripts/default/platform-hmilab-car.xml)

add_test(NAME lint_platform_hmiac
  COMMAND ${XMLLINT} --schema
  ${CMAKE_SOURCE_DIR}/gitscripts/default/platform.xsd
  ${CMAKE_SOURCE_DIR}/gitscripts/default/platform-hmilab-ac.xml)
