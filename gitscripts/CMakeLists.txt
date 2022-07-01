# find git

execute_process(COMMAND ${Python_EXECUTABLE} -c "import git"
  RESULT_VARIABLE PYTHON_HAVE_GITPYTHON)
execute_process(COMMAND ${Python_EXECUTABLE} -c "import lxml"
  RESULT_VARIABLE PYTHON_HAVE_LXML)

if (NOT PYTHON_HAVE_GITPYTHON EQUAL 0)
  message(FATAL_ERROR "need python module GitPython")
endif()
if (NOT PYTHON_HAVE_LXML EQUAL 0)
  message(FATAL_ERROR "need python module lxml")
endif()

add_subdirectory(default)
add_subdirectory(duecautils)




set(PYSCRIPTS
  dueca-list-modules.py
  process-comm-objects.py)

install(PROGRAMS dueca-cvs-to-git.py DESTINATION ${CMAKE_INSTALL_BINDIR}
  RENAME dueca-cvs-to-git)

install(FILES ${PYSCRIPTS}
  DESTINATION ${CMAKE_INSTALL_DATADIR}/dueca)

install(PROGRAMS dueca-gproject.py DESTINATION ${CMAKE_INSTALL_BINDIR}
  RENAME dueca-gproject)
