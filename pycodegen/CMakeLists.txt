# test some supporting python packages
execute_process(COMMAND ${Python_EXECUTABLE} -c "import lxml"
  RESULT_VARIABLE PYTHON_HAVE_LXML)

if (NOT (PYTHON_HAVE_LXML EQUAL 0))
  message(FATAL_ERROR "Need python3-lxml installed")
endif()

configure_file(generation.py.in generation.py @ONLY)

set(PYSCRIPTS
  dueca-codegen.py)

install(PROGRAMS dueca-codegen.py DESTINATION ${CMAKE_INSTALL_BINDIR}
  RENAME dueca-codegen)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/generation.py __init__.py
  DESTINATION ${CMAKE_INSTALL_DATADIR}/dueca/pycodegen)

install(FILES

  DCOplugins/__init__.py
  DCOplugins/hdf5.py DCOplugins/hdf5nest.py
  DCOplugins/msgpack.py

  DESTINATION ${CMAKE_INSTALL_DATADIR}/dueca/DCOplugins)

install(FILES

  EnumPlugins/__init__.py
  EnumPlugins/hdf5.py
  EnumPlugins/msgpack.py

  DESTINATION ${CMAKE_INSTALL_DATADIR}/dueca/EnumPlugins)
