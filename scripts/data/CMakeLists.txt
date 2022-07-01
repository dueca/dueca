add_subdirectory(default)

configure_file(MakefileTail.mod.in MakefileTail.mod @ONLY)

set(DATA ${CMAKE_CURRENT_BINARY_DIR}/MakefileTail.mod MakefileTail.app
  dummy.c GenericStart module.oxy comm-objects.oxy MakefileTailApp.doc
  MakefileTailMod.doc MakefileTailCom.doc )

install(FILES ${DATA} DESTINATION ${CMAKE_INSTALL_DATADIR}/dueca/data)

install(FILES dco.xsd DESTINATION
  ${CMAKE_INSTALL_DATADIR}/dueca/data/default)

