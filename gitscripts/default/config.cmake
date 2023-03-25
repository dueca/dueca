# ============================================================================
#       item            : CMake additional configuration for a node class
#       made by         : René van Paassen
#       date            : 180326
#       copyright       : (c) 2018 TUDelft-AE-C&S
# ============================================================================

# A "node" is a computer participating in a DUECA distributed process
# Each node requires a specific configuration, e.g., to show instruments,
# out-of-the-window view, perform IO with hardware. The software
# configuration for a node (which modules are included) is determined by
# its "machine class"
#
# The "machine class" thus indicates what part of the application runs on this
# computer, examples are control_loading, ig, efis
#
# In principle, finding libraries and headers should be done in the
# CMakeLists.txt files corresponding to the different modules
#
# However, in some cases you want to specify libraries or options specific
# to a machine class. Two examples:
#
# - which GUI back-end to include, if any
#
# - IO libraries that are specific to a machine, for linking to specific
#   hardware
#
# Per machine class configuration can be specified in this file
#
# This file is included when dueca_setup_project is called from the main
# CMakeLists.txt file

# extend DUECA_COMPONENTS with additional components
set(GUI_COMPONENT "@gui@")
if(GUI_COMPONENT)
    list(APPEND DUECA_COMPONENTS ${GUI_COMPONENT})
endif()

# define PROJECT_LIBRARIES with libraries needed on the current platform,
# use CMAKE to detect these if needed
#set(PROJECT_LIBRARIES )

# define PROJECT_INCLUDE_DIRS with include directories common to all on
# the current platform
#set(PROJECT_INCLUDE_DIRS )

# define PROJECT_COMPILE_FLAGS with the flags needed for compiling
#set(PROJECT_COMPILE_FLAGS )
