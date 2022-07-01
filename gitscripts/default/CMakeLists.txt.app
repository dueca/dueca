# -*-cmake-*-
# ============================================================================
#       item            : CMake configuration DUECA executable
#       made by         : René van Paassen
#       date            : 180326
#       copyright       : (c) 2021 TUDelft-AE-C&S
# ============================================================================

cmake_minimum_required(VERSION 3.8)

project(@project@)

# default c++11
set(CMAKE_CXX_STANDARD 11)

# script language used
set(SCRIPTLANG  @scriptlang@)

# selection of dueca components common to all machines in this project
set(DUECA_COMPONENTS ${SCRIPTLANG} extra dusime udp hdf5)

# find DUECA, activate specific functions
find_package(DUECA)

# This reads the machine class from the ".config/machine" file, then figures
# out which modules are needed from ".config/class/modules.xml", and sets
# PROJECT_MODULES
# it also sets DUECA_LIBRARIES with the components listed above
dueca_setup_project(${DUECA_COMPONENTS})

# create the main executable
add_executable(dueca_run.x
  ${CMAKE_BINARY_DIR}/empty.cxx)

# specify linking in the modules and libraries
target_link_libraries(dueca_run.x PRIVATE
  ${PROJECT_MODULES} ${PROJECT_LIBRARIES} ${DUECA_LIBRARIES})
target_compile_options(dueca_run.x PRIVATE
  ${PROJECT_COMPILEOPTIONS} ${DUECA_COMPILEOPTIONS})
