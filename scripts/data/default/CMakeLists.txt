set(DATA

  Makefile.mod Makefile.app Makefile.dco modules.lst comm-objects.lst
  links.script clean.script dueca.cnf.in dueca.mod.in dueca_cnf.py.in
  dueca_mod.py.in report DuecaModuleTemplate.cxx DuecaModuleTemplate.hxx
  DusimeModuleTemplate.cxx DusimeModuleTemplate.hxx
  HardwareModuleTemplate.cxx HardwareModuleTemplate.hxx
  SimulationModelTemplate.cxx SimulationModelTemplate.hxx
  SimulationModuleTemplate.cxx SimulationModuleTemplate.hxx
  DuecaHelperTemplate.cxx DuecaHelperTemplate.hxx
  Simulink88Template.cxx Simulink88Template.hxx
  Simulink88Test.cc Simulink88Makefile
  DCOObjectTemplate.dco DCOEnumTemplate.dco
)

install(FILES ${DATA} DESTINATION ${CMAKE_INSTALL_DATADIR}/dueca/data/default)
