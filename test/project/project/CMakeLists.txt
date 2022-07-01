# ============================================================================
#       item            : CMake configuration DUECA executable
#       made by         : René van Paassen
#       date            : 180326
#       copyright       : (c) 2018 TUDelft-AE-C&S
# ============================================================================

cmake_minimum_required(VERSION 3.0)

project(project)

list(APPEND CMAKE_MODULE_PATH
  "${CMAKE_SOURCE_DIR}/../../../build-posix/cmake"
  "${CMAKE_SOURCE_DIR}/../../../cmake")

# selection of dueca components common to all machines in this project
set(DUECA_COMPONENTS python extra dusime ip)

# find DUECA, activate specific functions
find_package(DUECA)

# This reads the platform name from the ".machine" file, then figures
# out which modules are needed from the modules.<platform>, and sets
# PROJECT_MODULES
# it also sets DUECA_LIBRARIES with the components listed above
dueca_setup_machine(${DUECA_COMPONENTS})

# create the main executable
add_executable(dueca_run.x ${CMAKE_BINARY_DIR}/empty.cxx)

# specify linking in the modules and libraries
target_link_libraries(dueca_run.x PRIVATE
  ${PROJECT_MODULES} ${DUECA_LIBRARIES})
target_compile_options(dueca_run.x PRIVATE
  ${DUECA_COMPILEOPTIONS})
