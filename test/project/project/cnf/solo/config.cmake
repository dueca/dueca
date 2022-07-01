# ============================================================================
#       item            : CMake additional configuration for a node class
#       made by         : René van Paassen
#       date            : 180326
#       copyright       : (c) 2018 TUDelft-AE-C&S
# ============================================================================

# A "node" is a computer participating in a DUECA distributed process
# The "node class" indicates what part of the application runs on this
# computer, e.g. control_loading, ig, efis
#
# Per node class, specify what libraries to link, additional DUECA
# components, compile flags etc.

# This file is "sourced" when dueca_adapt_machine is called.

# extend DUECA_COMPONENTS with additional components
list(APPEND DUECA_COMPONENTS gtk3)

# define PROJECT_LIBRARIES with libraries needed on the current platform,
# use CMAKE to detect these if needed
#set(PROJECT_LIBRARIES )

# define PROJECT_INCLUDE_DIRS with include directories
#set(PROJECT_INCLUDE_DIRS )

# define PROJECT_COMPILE_FLAGS with the flags needed for compiling
#set(PROJECT_COMPILE_FLAGS )
