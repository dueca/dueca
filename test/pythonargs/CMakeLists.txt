
if (NOT Boost_LIBRARIES)

  if (APPLE)
    set(Python_ADDITIONAL_VERSIONS 3)
    find_package(PythonLibs)
    if (NOT PythonLibs_FOUND)
      message(FATAL_ERROR "Need python development libraries >= 3")
    endif()
  else()
    pkg_check_modules(PYTHON REQUIRED python3)
    if (NOT PYTHON_FOUND)
      message(FATAL_ERROR "Need python development libraries >= 3")
    endif()
  endif()

  if (APPLE)
    set(BPCOMP python3)
  else()
    find_library(BPLIB
      NAMES boost_python3 boost_python-py34 boost_python-py35 boost_python-py36)
    get_filename_component(BPCOMP ${BPLIB} NAME_WE)
    string(SUBSTRING ${BPCOMP} 9 -1 BPCOMP)
    message(STATUS "Searching for boost-python with component ${BPCOMP}")
  endif()

  include_directories()
  set(Boost_USE_STATIC_LIBS OFF)
  set(Boost_USE_MULTITHREADED ON)
  set(Boost_USE_STATIC_RUNTIME OFF)
  find_package(Boost 1.50.0 COMPONENTS  ${BPCOMP})

endif()


add_library(pythonargs MODULE pythonargs.cxx)

set_target_properties(pythonargs PROPERTIES
  OUTPUT_NAME dueca PREFIX "")
target_include_directories(pythonargs PUBLIC
  ${Boost_INCLUDE_DIRS} ${PYTHON_INCLUDE_DIRS} ${Python_INCLUDE_DIRS})

target_link_libraries(pythonargs ${Boost_LIBRARIES}
  ${PYTHONEMBED_LIBRARIES} ${PYTHON_LIBRARIES} ${Python_LIBRARIES})


configure_file(loadandcheck.py loadandcheck.py @ONLY)

if (PYTHON3)
  add_test(PYTHONCHECK ${PYTHON3} ${CMAKE_CURRENT_BINARY_DIR}/loadandcheck.py)
endif()
