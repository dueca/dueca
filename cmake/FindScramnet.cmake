# Try to find SCRAMNet. Once done, this will define:
#
#   SCRAMNET_FOUND - variable which returns the result of the search
#   SCRAMNET_INCLUDE_DIRS - list of include directories
#   SCRAMNET_LIBRARIES - options for the linker
find_path(SCRAMNET_INCLUDE_DIR
  scramnet/scrplus.h)
find_library(SCRAMNET_LIBRARY_PLUS plus
  HINTS /usr/lib64/scramnet /usr/lib/scramnet)
find_library(SCRAMNET_LIBRARY_HW hw
  HINTS /usr/lib64/scramnet /usr/lib/scramnet)
find_library(SCRAMNET_LIBRARY_HWD hwd
  HINTS /usr/lib64/scramnet /usr/lib/scramnet)

set(SCRAMNET_INCLUDE_DIRS ${SCRAMNET_INCLUDE_DIR})
set(SCRAMNET_LIBRARIES ${SCRAMNET_LIBRARY_PLUS} ${SCRAMNET_LIBRARY_HW} ${SCRAMNET_LIBRARY_HWD})
if(SCRAMNET_INCLUDE_DIRS AND SCRAMNET_LIBRARIES)
  set(SCRAMNET_FOUND TRUE)
endif()

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(SCRAMNET DEFAULT_MSG
        SCRAMNET_INCLUDE_DIR
        SCRAMNET_LIBRARY_PLUS
        SCRAMNET_LIBRARY_HW
        SCRAMNET_LIBRARY_HWD
)

mark_as_advanced(
        SCRAMNET_INCLUDE_DIR
        SCRAMNET_LIBRARY_PLUS
        SCRAMNET_LIBRARY_HW
        SCRAMNET_LIBRARY_HWD
)
