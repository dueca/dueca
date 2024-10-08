set(PLATFORMS
  platform-srs.xml
  platform-hmilab-car.xml
  platform-hmilab-ac.xml
  platform-hmilab-manual.xml
)

set(DATA

  CMakeLists.txt.com CMakeLists.txt.mod CMakeLists.txt.app modules.txt
  config.cmake machine machinemapping.xml
  policylist.xml modules.xml README-pseudomodule.md build.gitignore
  run-data-README.md ${PLATFORMS} RunProject project.gitignore
  project-README.md platform.xsd testscenario.xsd machinemapping.xsd
  modules.xsd policies.xsd policylist.xsd project.vscode.launch.json
  project.vscode.tasks.json project.vscode.settings.json
  project.clang-format )

install(FILES ${DATA} DESTINATION ${CMAKE_INSTALL_DATADIR}/dueca/data/default)

find_package(LibXml2 REQUIRED)

foreach(f ${PLATFORMS})
  add_test(NAME TEST_${f}
    COMMAND ${LIBXML2_XMLLINT_EXECUTABLE} --schema
    ${CMAKE_CURRENT_SOURCE_DIR}/platform.xsd
    ${CMAKE_CURRENT_SOURCE_DIR}/${f})
endforeach()

set(XMLTESTS machinemapping modules policylist testscenario)

foreach(x ${XMLTESTS})
  add_test(NAME TEST_${x}
    COMMAND ${LIBXML2_XMLLINT_EXECUTABLE} --schema
    ${CMAKE_CURRENT_SOURCE_DIR}/${x}.xsd
    ${CMAKE_CURRENT_SOURCE_DIR}/${x}.xml)
endforeach()
