# find PyPA build
execute_process(COMMAND ${Python_EXECUTABLE} -c "import build"
  RESULT_VARIABLE PYTHON_HAVE_PYPABUILD)

if (NOT PYTHON_HAVE_PYPABUILD EQUAL 0)
  message(WARNING
    "Prefer python module PyPA build; run python -m pip install build")
endif()

# figure the python major.minor version string
execute_process(COMMAND ${Python_EXECUTABLE} -c
  "import sys; print('.'.join(map(str,sys.version_info[:2])),end='')"
  OUTPUT_VARIABLE PYTHON_MAJMIN)
execute_process(COMMAND ${Python_EXECUTABLE} -c
  "import sys; print(sys.version_info[0],end='')"
  OUTPUT_VARIABLE PYTHON_MAJOR)
execute_process(COMMAND bash "-c"
  "${Python_EXECUTABLE} -m pip help install | grep '[-]-system'"
  RESULT_VARIABLE PYTHON_PIP_WITHSYSTEM)
if (PYTHON_PIP_WITHSYSTEM EQUAL 0)
  message(STATUS "pip accepts --system")
  set(PIP_SYSTEM "--system")
else()
  message(STATUS "pip does not accept --system")
endif()

# list all python modules and files that create the package
set(DEPS
  ${CMAKE_CURRENT_SOURCE_DIR}/src/actions/changedco.py
  ${CMAKE_CURRENT_SOURCE_DIR}/src/actions/changemodule.py
  ${CMAKE_CURRENT_SOURCE_DIR}/src/actions/__init__.py
  ${CMAKE_CURRENT_SOURCE_DIR}/src/actions/inserttext.py
  ${CMAKE_CURRENT_SOURCE_DIR}/src/actions/policyaction.py
  ${CMAKE_CURRENT_SOURCE_DIR}/src/commobjects.py
  ${CMAKE_CURRENT_SOURCE_DIR}/src/conditions/borrowsmodule.py
  ${CMAKE_CURRENT_SOURCE_DIR}/src/conditions/findpattern.py
  ${CMAKE_CURRENT_SOURCE_DIR}/src/conditions/hasdco.py
  ${CMAKE_CURRENT_SOURCE_DIR}/src/conditions/__init__.py
  ${CMAKE_CURRENT_SOURCE_DIR}/src/conditions/policycondition.py
  ${CMAKE_CURRENT_SOURCE_DIR}/src/conditions/conditionand.py
  ${CMAKE_CURRENT_SOURCE_DIR}/src/conditions/conditionor.py
  ${CMAKE_CURRENT_SOURCE_DIR}/src/conditions/conditionnot.py
  ${CMAKE_CURRENT_SOURCE_DIR}/src/githandler.py
  ${CMAKE_CURRENT_SOURCE_DIR}/src/__init__.py
  ${CMAKE_CURRENT_SOURCE_DIR}/src/machinemapping.py
  ${CMAKE_CURRENT_SOURCE_DIR}/src/matchreference.py
  ${CMAKE_CURRENT_SOURCE_DIR}/src/modules.py
  ${CMAKE_CURRENT_SOURCE_DIR}/src/policylist.py
  ${CMAKE_CURRENT_SOURCE_DIR}/src/policy.py
  ${CMAKE_CURRENT_SOURCE_DIR}/src/verboseprint.py
  ${CMAKE_CURRENT_SOURCE_DIR}/src/xmlutil.py
  ${CMAKE_CURRENT_SOURCE_DIR}/src/codegen/MemberSummary.py
  ${CMAKE_CURRENT_SOURCE_DIR}/src/codegen/__init__.py
  ${CMAKE_CURRENT_SOURCE_DIR}/README.md
  ${CMAKE_CURRENT_SOURCE_DIR}/setup.cfg
  ${CMAKE_CURRENT_SOURCE_DIR}/pyproject.toml
  )

# add the target
set(DUVERSION 0.9)
add_custom_target(
  duecautils ALL
  DEPENDS
  dist/duecautils-${DUVERSION}.tar.gz
  dist/duecautils-${DUVERSION}-py${PYTHON_MAJOR}-none-any.whl
  )

# find an installation folder for the Python packages; we know
# pyparsing is installed, since we need it for dco objects
if (NOT PYSITEDIR)
  find_path(PYSITEDIR
    NAMES pyparsing.py
    NO_DEFAULT_PATH
    PATH_SUFFIXES dist-packages site-packages
    PATHS
    ${CMAKE_INSTALL_PREFIX}/lib/python${PYTHON_MAJOR}
    ${CMAKE_INSTALL_PREFIX}/lib/python${PYTHON_MAJMIN}
    /usr/lib/python${PYTHON_MAJOR}
    /usr/lib/python${PYTHON_MAJMIN}
    /usr/local/lib/python${PYTHON_MAJOR}
    /usr/local/lib/python${PYTHON_MAJMIN})
endif()

set(PYMOD_INSTALL_TYPE bdist_wheel)

# install the python package note that "DESTDIR" and
# "CMAKE_INSTALL_PREFIX" need to be kept intact, to allow substitution
# at the install stage
# this code gets executed by the build system (ninja, Makefile or the like)
install(CODE "execute_process(COMMAND bash -c \"PYTHONPATH=\$ENV{DESTDIR}${PYSITEDIR} ${Python_EXECUTABLE} -m pip install ${PIP_SYSTEM} --no-index --find-links=${CMAKE_CURRENT_BINARY_DIR}/dist --target \$ENV{DESTDIR}${PYSITEDIR} --upgrade duecautils\")" )


if (PYTHON_HAVE_PYPABUILD EQUAL 0)
  # simple build with PyPA build, following PEP-517, PEP-518, using
  # setup.cfg, pyproject.toml
  add_custom_command(
    OUTPUT
    dist/duecautils-${DUVERSION}.tar.gz
    dist/duecautils-${DUVERSION}-py${PYTHON_MAJOR}-none-any.whl

    BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/timestamp

    COMMAND ${CMAKE_COMMAND} -E create_symlink
    ${CMAKE_CURRENT_SOURCE_DIR}/setup.cfg
    ${CMAKE_CURRENT_BINARY_DIR}/setup.cfg

    COMMAND ${CMAKE_COMMAND} -E create_symlink
    ${CMAKE_CURRENT_SOURCE_DIR}/README.md
    ${CMAKE_CURRENT_BINARY_DIR}/README.md

    COMMAND ${CMAKE_COMMAND} -E create_symlink
    ${CMAKE_CURRENT_SOURCE_DIR}/pyproject.toml
    ${CMAKE_CURRENT_BINARY_DIR}/pyproject.toml

    COMMAND ${CMAKE_COMMAND} -E create_symlink
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}/src

    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/dist
    COMMAND ${Python_EXECUTABLE} -m build
    COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/timestamp
    DEPENDS ${DEPS}
    )
else()
  configure_file(setup.py.in setup.py @ONLY)

  add_custom_command(
    OUTPUT
    dist/duecautils-${DUVERSION}.tar.gz
    dist/duecautils-${DUVERSION}-py${PYTHON_MAJOR}-none-any.whl
    BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/timestamp

    COMMAND ${CMAKE_COMMAND} -E create_symlink
    ${CMAKE_CURRENT_SOURCE_DIR}/README.md
    ${CMAKE_CURRENT_BINARY_DIR}/README.md

    COMMAND ${CMAKE_COMMAND} -E create_symlink
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}/src

    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/dist
    COMMAND ${Python_EXECUTABLE} setup.py bdist_wheel
    COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/timestamp

    DEPENDS ${DEPS} ${CMAKE_CURRENT_BINARY_DIR}/setup.py
    )

endif()

# create folder for install
install(DIRECTORY DESTINATION ${PYSITEDIR})
