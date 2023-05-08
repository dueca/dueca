# -------------------------------------------------------------------   */
#       item            : dueca/CMakeLists.txt
#       made by         : Rene' van Paassen
#       date            : 1610xx
#       category        : cmake definition
#       copyright       : (c) 2016 TUDelft-AE-C&S

cmake_minimum_required(VERSION 3.0)

if (POLICY CMP0075)
  message(STATUS "New policy CMP0075")
  cmake_policy(SET CMP0075 NEW)
endif()

# CXX and C
enable_language(C)
enable_language(CXX)

# gnu install dirs
include(GNUInstallDirs)

# version information, please update accordingly
set(VERMAJOR 3)
set(VERMINOR 2)
set(REVISION 9)
set(GENCODEGEN 110)
set(SOVERSION "${VERMAJOR}.${VERMINOR}")
set(VERSION_DATE "19 Apr 2023")
string(REPLACE " " "" "${VERSION_DATE}" CLIENT_DATE)
set(versioned "${VERMAJOR}-${VERMINOR}-${REVISION}")

string(REGEX REPLACE "^.* ([0-9]+)$" "\\1"
  VERSION_YEAR "${VERSION_DATE}")
project(dueca VERSION "${VERMAJOR}.${VERMINOR}.${REVISION}")

# include custom find code
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

# default version SVN location
string(REPLACE "." "-" SVNLOCATION "tags/D${dueca_VERSION}")

# base options, logic located further down
option(BUILD_RTWPARSER     "Create the RTW parser for SIMULINK models" ON)
option(BUILD_EXTRA         "Create auxiliary components" ON)
option(BUILD_INTER         "DUECA process interconnection components" ON)
option(BUILD_HDF5          "HDF5 logging utilities" OFF)
option(BUILD_DUSIME        "Build DUSIME components" ON)
option(BUILD_DMODULES      "Build DUSIME control modules" ON)
option(BUILD_DOC           "Generate documentation" ON)
option(BUILD_IP            "Build IP based communication" ON)
option(BUILD_UDP           "Build new UDP/IP based communication" ON)
option(BUILD_WEBSOCK       "Build a websocket external server" OFF)
set(MAX_NODES    254 CACHE STRING "Maximum number of nodes")
set(MAX_MANAGERS  32 CACHE STRING "Maximum number of priorities/managers")
option(GREEDY_PERIODIC     "Optimistic triggering on activities made periodic" ON)
option(BUILD_GTK2          "Build GTK2 interface" ON)
option(BUILD_GTKMM         "Build GTKMM interface" OFF)
option(BUILD_GTK3          "Build GTK3 interface" OFF)
option(BUILD_GTKMM3        "Build GTKMM interface" OFF)
option(BUILD_GTKGLEXT      "Use gtkglext for gtk2" ON)
option(ALSO_INIT_GLUT      "Initialize glut for the utilities" OFF)
option(BUILD_GLUT          "Build GLUT interface" ON)
option(BUILD_GLUTGUI       "Build GLUI interface" OFF)
option(BUILD_FLTK          "Build FLTK interface" ON)
option(BUILD_SHM           "Shared memory communication" ON)
option(BUILD_X11GL         "Direct GL window support on X11" ON)
option(SCRIPT_PYTHON       "Scripting interface to python" ON)
option(SCRIPT_SCHEME       "Scripting interface to scheme" ON)
option(USE_POSIX_THREADS   "Use POSIX threads" ON)
option(USE_GETTIMEOFDAY    "Measure time with gettimeofday" ON)
option(SYNC_WITH_SIGWAIT   "Add sigwait as synchronization means" ON)
option(SYNC_WITH_SELECT    "Add select as synchronization means" ON)
option(SYNC_WITH_NANOSLEEP "Add nanosleep as synchronization means" ON)
set(RTC_RATE "0" CACHE STRING     "Add /dev/rtc as synchronization means")
option(BUILD_MOREWARN      "Build with really fussy warnings" ON)
option(BUILD_TESTOPT       "Build with additional test options" OFF)
option(BUILD_ASSERT        "Build with assert statements active" OFF)
option(BUILD_DEBPRINT      "Build with debug print statements" OFF)
option(BUILD_FEXCEPT       "Build with floating point exceptions" OFF)
option(ACTIV_NOCATCH       "Remove exception catching mechanisms, debug only" OFF)
option(HDF5_NOCATCH        "Remove exception catching, debug only" OFF)
option(BUILD_CALLTRACE     "Generate call trace log" OFF)
option(NEW_LOGGING         "new logging code" ON)
option(RUN_TESTS           "run test code" OFF)
set(PLATFORM_INCLUDE_PATH "" CACHE FILEPATH "Path to additional headers")
set(PLATFORM_LIBRARY_PATH "" CACHE FILEPATH "Path to additional libraries")
option(DUECA_BUILD_STATIC  "Build static libraries" ON)
option(DUECA_BUILD_SHARED  "Build shared libraries" ON)
option(DUECA_INIT_STATIC   "Use static libraries for start code" ON)
set(DUECA_DOC_PATH "share/doc" CACHE FILEPATH "Documentation directory")
option(LOG_COMMUNICATIONS  "Debug log of ip communication" OFF)
option(LOG_PACKING         "Debug log of packing actions" OFF)
option(USE_DSTRING         "Use custom fixed-length strings" ON)
set(SVNHEAD ${SVNLOCATION} CACHE STRING "svn location current version")
option(USE_ATOMIC          "Use the c++ <atomic> header for atomic operations" ON)
option(USE_STD_TYPEINDEX   "Use c++11 typeindex info, otherwise boost" ON)
option(FILLPACKER_SEND_ID  "Let fill packing/unpacking add message number and ID" OFF)
option(FORCE_PYTHON_MALLOC "Force malloc as allocator for python" OFF)
option(BUILD_DDFF          "Build Delft Direct File Format logging" ON)
option(TRY_INSTALL_PYTHON_BUILD "If needed, use pip to install python build" OFF)
if (APPLE)
  set(CMAKE_FIND_FRAMEWORK LAST)
endif()

# for special purpose builds, like mac/macports
if (CMAKE_PREFIX_PATH)
  include_directories(${CMAKE_PREFIX_PATH}/include)
  link_directories(${CMAKE_PREFIX_PATH}/lib)
  list(APPEND CMAKE_REQUIRED_INCLUDES "${CMAKE_PREFIX_PATH}/include")
endif()

if (DUECA_INIT_STATIC)
  set(STARTLIB_STATIC "FORCESTATIC")
endif()
if (DUECA_BUILD_STATIC AND (NOT DUECA_BUILD_DYNAMIC))
  set(statcontrol "  --define-variable=staticsuffix=-static")
  set(STATICSUFFIX "-static")
endif()

# c++11 option compiler flag for newer gcc
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 4.6)
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-std=c++11>)
  endif()
endif()

# RTC check
if (RTC_RATE)
  foreach(TRATE 2 4 8 16 32 64 128 256 512 1024 2048 4096)
    if (${RTC_RATE} STREQUAL ${TRATE})
      option(SYNC_WITH_RTC "RTC as sync option" ON)
    endif()
  endforeach()
  if (SYNC_WITH_RTC)
    message(STATUS "Setting RTC clock rate to ${RTC_RATE} Hz")
  else()
    message(WARNING "incorrect RTC rate selected")
  endif()
endif()

# fussy warnings
if (BUILD_MOREWARN)
  add_compile_options(
    -Wall -Wpointer-arith -Wno-cast-align -Wunreachable-code
    -Wstrict-aliasing=2)
endif()

if (BUILD_ASSERT)
  set(ASSERT_ACTIVE 1)
endif()

# executable to test path trimming size in __FILE__
# (this appears to depend on the generator)
add_executable(sourcepath_test.x sourcepath_test.cxx)

# use the above executable to calculate this trimming size
execute_process(
  COMMAND
  ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target sourcepath_test.x
  COMMAND
  ${CMAKE_CURRENT_BINARY_DIR}/sourcepath_test.x
  OUTPUT_VARIABLE SOURCE_PATH_SIZE
  OUTPUT_STRIP_TRAILING_WHITESPACE)

# results
message(STATUS "CMAKE_SOURCE_DIR defined as \"${CMAKE_SOURCE_DIR}/\"")
message(STATUS "Length of source path: ${SOURCE_PATH_SIZE}")

# guile version
find_program(GUILECONFIG NAMES guile-config guile1.8-config
  guile18-config guile-config2.2 guile1-config)

if (NOT GUILECONFIG)
   message(FATAL_ERROR "No suitable guile-config found")
endif()

# guile cflags and guild libs
execute_process(COMMAND ${GUILECONFIG} link
  OUTPUT_VARIABLE SCRIPT_GUILE_LIBS
  OUTPUT_STRIP_TRAILING_WHITESPACE)
string(STRIP "${SCRIPT_GUILE_LIBS}" SCRIPT_GUILE_LIBS)

execute_process(COMMAND ${GUILECONFIG} compile
  OUTPUT_VARIABLE SCRIPT_SCHEME_FLAGS
  OUTPUT_STRIP_TRAILING_WHITESPACE)
set(TMPLIST ${SCRIPT_SCHEME_FLAGS})
separate_arguments(TMPLIST)
foreach(W IN LISTS TMPLIST)
          if ("${W}" MATCHES "^-I.*")
             string(SUBSTRING "${W}" 2 -1 IPATH)
             list(APPEND CMAKE_REQUIRED_INCLUDES "${IPATH}")
             include_directories("${IPATH}")
          endif()
endforeach()

# don't re-write all pkgconfig files, set the flags in there
set(prefix ${CMAKE_INSTALL_PREFIX})
set(exec_prefix "\${prefix}")
set(includedir "\${prefix}/include")
set(libdir ${CMAKE_INSTALL_FULL_LIBDIR})

# check flag for setting linker symbol undefined
include(CheckCCompilerFlag)

# this one is preferred, since it will flag missing symbols
set(CMAKE_REQUIRED_FLAGS "-Wl,--require-defined=main")
check_c_compiler_flag("" LD_REQUIREDEFINED)
if (LD_REQUIREDEFINED)
  message(STATUS "linker supports --require-defined")
  set(ldundef "--require-defined=")
else()

  # second option, long name (GNU), for good measure (SUSE, Debian),
  # add --no-as-needed
  set(CMAKE_REQUIRED_FLAGS "-Wl,--no-as-needed,--undefined=main")
  check_c_compiler_flag("" LD_UNDEFINED)
  if (LD_UNDEFINED)
    message(STATUS "linker supports --undefined")
    set(ldundef "--no-as-needed,--undefined=")
  else()

    # third option, single -u_, for Apple
    set(CMAKE_REQUIRED_FLAGS "-Wl,-u,_main")
    check_c_compiler_flag("" LD_U)
    if (LD_U)
      message(STATUS "linker supports -u,_")
      set(ldundef "-u,_")
    else()
      message(FATAL_ERROR "no flag for selecting undefined symbol with linker")
    endif()
  endif()
endif()

# check c++-11 compat, and prefer
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-std=c++11" ELEVEN)
if (ELEVEN)
  message(STATUS "standard c++11")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
else()
  message(STATUS "no c++11")
endif()

# hack, should test the flags, but simply check for compiler
message(STATUS "C compiler id ${CMAKE_C_COMPILER_ID}")
if (CMAKE_C_COMPILER_ID STREQUAL "AppleClang")
  set(prelink_flags "-r")
  set(staticflags "-Wl,-static")
  set(unstaticflags "-Wl,-dynamic")
elseif(CMAKE_C_COMPILER_ID STREQUAL "GNU")
  set(staticflags "-Wl,-static")
  set(unstaticflags "-Wl,-Bdynamic")
  if(CMAKE_C_COMPILER_VERSION VERSION_GREATER "3.9")
    set(prelink_flags "-Ur --eh-frame-hdr")
  else()
    set(prelink_flags "-Ur")
  endif()
endif()

# old version define
set(VERSION ${dueca_VERSION})
set(dueca_includedir "\${prefix}/include/dueca")

# check headers and libraries
include(FindPkgConfig)

include(CheckIncludeFiles)
include(CheckIncludeFileCXX)
check_include_files(fcntl.h HAVE_FCNTL_H)
check_include_files(limits.h HAVE_LIMITS_H)
check_include_files(sys/time.h HAVE_SYS_TIME_H)
check_include_files(libguile18.h HAVE_LIBGUILE18_H)
check_include_files(libguile.h HAVE_LIBGUILE_H)
check_include_files(unistd.h HAVE_UNISTD_H)
check_include_files(sys/param.h HAVE_SYS_PARAM_H)
check_include_files(setjmp.h HAVE_SETJMP_H)
check_include_files(sched.h HAVE_SCHED_H)
check_include_files(pthread.h HAVE_PTHREAD_H)
check_include_files(unix.h HAVE_UNIX_H)
check_include_files(signal.h HAVE_SIGNAL_H)
check_include_files(stdlib.h HAVE_STDLIB_H)
check_include_files(scrplus.h HAVE_SCRPLUS_H)
check_include_files(fp.h HAVE_FP_H)
check_include_files(sys/shm.h HAVE_SYS_SHM_H)
check_include_file_cxx(strstream HAVE_STRSTREAM)
check_include_file_cxx(sstream HAVE_SSTREAM)
check_include_files(X11/Xlib.h HAVE_X11_XLIB_H)
check_include_file_cxx(boost/smart_ptr/intrusive_ref_counter.hpp
  HAVE_BOOST_SMART_PTR_INTRUSIVE_REF_COUNTER_HPP)
check_include_file_cxx(boost/asio/io_service.hpp
  HAVE_BOOST_ASIO_IO_SERVICE_HPP)



if (USE_ATOMIC)
  check_include_file_cxx(atomic HAVE_ATOMIC)
  if (HAVE_ATOMIC)
  else()
    message(FATAL_ERROR "header <atomic> not found or not usable")
  endif()
endif()


# atomic library needed for 32 bit arm
include(CheckLibraryExists)
try_compile(ATOMIC_TEST1 ${CMAKE_CURRENT_BINARY_DIR}/atom1
  SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/atomic_test.cxx)

if (ATOMIC_TEST1)
  message(STATUS "Platform does not need to link with -latomic")
else()
  message(STATUS "Atomic test without -latomic failed")
  check_library_exists(atomic __atomic_load_8 "" LIBATOMIC_EXISTS)
  if (LIBATOMIC_EXISTS)
    set(ATOMIC_LIBRARY atomic)
  else()
    find_library(ATOMIC_LIBRARY atomic REQUIRED)
  endif()
  try_compile(ATOMIC_TEST2 ${CMAKE_CURRENT_BINARY_DIR}/atom2
    SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/atomic_test.cxx
    LINK_LIBRARIES ${ATOMIC_LIBRARY})
  if (NOT ATOMIC_TEST2)
    message(FATAL_ERROR "Cannot link with atomic")
  endif()
  message(STATUS "This platform uses -latomic")
  set(ATOMIC_LIBRARY_LIBFLAGS -latomic)
endif()

# library functions validation
include(CheckFunctionExists)
check_function_exists(random HAVE_RANDOM)
check_function_exists(rand HAVE_RAND)
check_function_exists(setuid HAVE_SETUID)
check_function_exists(mlockall HAVE_MLOCKALL)
check_function_exists(select HAVE_SELECT)
check_function_exists(socket HAVE_SOCKET)
check_function_exists(atexit HAVE_ATEXIT)
check_function_exists(on_exit HAVE_ON_EXIT)

# namespace ios options (obsolete?)
include (CheckCXXSourceCompiles)
check_cxx_source_compiles(
  "#include <fstream>
using namespace std; int main() { ios_base::openmode t;}" NAMESPACE_IOS_BASE)
check_cxx_source_compiles(
  "#include <fstream>
using namespace std; int main() { ios::openmode t;}" NAMESPACE_IOS)

# pkg-config checks
pkg_check_modules(EIGEN REQUIRED eigen3)

# gtk variants, these are re-checked with required in specific files
pkg_check_modules(GTKMM24 gtkmm-2.4)
pkg_check_modules(GTKGLEXT gtkglext-1.0)
pkg_check_modules(GTK3 gtk+-3.0)
pkg_check_modules(GTKMM30 gtkmm-3.0)

# rapidjson for channel data coding
pkg_check_modules(RAPIDJSON RapidJSON)
find_package(msgpack REQUIRED)

# gl and glui family
set(OpenGL_GL_PREFERENCE "GLVND")
find_package(OpenGL)

# date header library
find_package(date REQUIRED)

# toml11 needed for snapshot files
find_package(toml11 REQUIRED)

# search explicitly for freeglut, for the sake of macosx
check_include_files(GL/freeglut.h HAVE_GL_FREEGLUT_H)
if (DEFINED HAVE_GL_FREEGLUT_H)
  find_library(GLUT_LIB glut)
else()
  find_package(GLUT)
  check_include_files(GL/glut.h HAVE_GL_GLUT_H)
  check_include_files(glut.h HAVE_GLUT_H)
endif()
pkg_check_modules(GL gl)
find_package(X11)
set(GLUT_LIBRARIES ${GLUT_LIB} ${GLUT_LIBRARIES})

include(LibOpts)
#message(STATUS "GLUT_LIBRARIES ${GLUT_LIBRARIES}")
set(CMAKE_REQUIRED_LIBRARIES ${GLUT_LIBRARIES})
check_function_exists(glutMainLoopEvent HAVE_GLUTMAINLOOPEVENT)

if (BUILD_GLUTGUI)
  find_library(GLUI glui)
  set(CMAKE_REQUIRED_QUIET ON)
  check_include_file_cxx(GL/glui.h HAVE_GL_GLUI_H)
  check_include_file_cxx(glui.h HAVE_GLUI_HT)
  unset(CMAKE_REQUIRED_QUIET)
  if (NOT HAVE_GLUI_H AND NOT HAVE_GL_GLUI_H)
    message(FATAL_ERROR "Could not find glui.h nor GL/glui.h")
  endif()
  mylinker_arguments(OUTPUT dueca_glut_util_libs LIBLIST ${GLUT_LIBRARIES})
  mylinker_arguments(OUTPUT dueca_glut_libs LIBLIST ${GLUT_LIBRARIES})
endif()
# fltk
# set(FLTK_SKIP_FLUID TRUE)
# find_package(FLTK)
# set(dueca_fltk_libs ${FLTK_LIBRARIES})

# fltk cmake find package is flaky, and fails on deb-based systems
# https://groups.google.com/forum/#!topic/fltkgeneral/GWlW6KiYnKM
if (BUILD_FLTK)
  execute_process(COMMAND fltk-config --version
    RESULT_VARIABLE FLTK_FOUND
    OUTPUT_VARIABLE FLTK_VERSION)
  if(FLTK_VERSION)
    message(STATUS "Found fltk version ${FLTK_VERSION}")
  else()
    message(FATAL_ERROR "Did not find fltk")
  endif()
  execute_process(COMMAND fltk-config --use-gl --cxxflags
    OUTPUT_VARIABLE FLTK_CFLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  set(dueca_fltk_cflags "${FLTK_CFLAGS}")
  separate_arguments(FLTK_CFLAGS)

  set(APPLE_FLTK_LIBRARIES)
  if(APPLE)
    execute_process(COMMAND fltk-config --use-gl --libs
      OUTPUT_VARIABLE dueca_fltk_libs
      OUTPUT_STRIP_TRAILING_WHITESPACE)
    find_library(COREFOUNDATION_LIBRARY CoreFoundation )
    list(APPEND APPLE_FLTK_LIBRARIES ${COREFOUNDATION_LIBRARY})
    find_library(COREGRAPHICS_LIBRARY CoreGraphics)
    list(APPEND APPLE_FLTK_LIBRARIES ${COREGRAPHICS_LIBRARY})
    find_library(CORETEXT_LIBRARY CoreText)
    list(APPEND APPLE_FLTK_LIBRARIES ${CORETEXT_LIBRARY})
    message(STATUS "FLTK_LIBRARIES ${APPLE_FLTK_LIBRARIES}")
  else()
    execute_process(COMMAND fltk-config --use-gl --ldflags
      OUTPUT_VARIABLE dueca_fltk_libs
      OUTPUT_STRIP_TRAILING_WHITESPACE)
  endif()
endif()

set(CMAKE_THREAD_PREFER_PTHREAD ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
message(STATUS "threadflag ${CMAKE_THREAD_LIBS_INIT}")

# message(STATUS "SVNHEAD ${SVNHEAD}")
add_custom_target(ChangeLog
  COMMAND svn2cl -r 'HEAD:1837' --authors=${CMAKE_CURRENT_SOURCE_DIR}/AUTHORS
  --include-actions --ignore-message-starting=HACK --stdout
  https://svn.lr.tudelft.nl/ADIO-CS/DUECA/${SVNHEAD} --reparagraph
  --group-by-day | cat - ${CMAKE_CURRENT_SOURCE_DIR}/ChangeLog.old > ChangeLog
  SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/ChangeLog.old
  COMMENT "[svn2cl] generating ChangeLog"
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

# configured files output;
# most of these can go to the underlying CMakeLists.txt files
add_compile_options(-DHAVE_CONFIG_H)
configure_file(dueca-confcm.h.in dueca-conf.h)
configure_file(debprint.h.in debprint.h)
configure_file(dassert.h.in dassert.h)
configure_file(gencodegen.h.in gencodegen.h)
configure_file(oddoptions.h.in oddoptions.h)
configure_file(obs/dueca.spec.in obs/dueca.spec @ONLY)
configure_file(obs/dueca.dsc.in obs/dueca.dsc @ONLY)
configure_file(obs/Portfile.in obs/Portfile @ONLY)
configure_file(obs/dueca-versioned.spec.in obs/dueca-versioned.spec @ONLY)
configure_file(pycodegen/generation.py.in pycodegen/generation.py @ONLY)

include(CMakePackageConfigHelpers)
configure_package_config_file(
  cmake/DUECAConfig.cmake.in cmake/DUECAConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_DATADIR}/dueca/CMake
  PATH_VARS CMAKE_INSTALL_DATADIR CMAKE_INSTALL_INCLUDEDIR)

# configure_file(cmake/FindDUECA.cmake.in cmake/FindDUECA.cmake @ONLY)

configure_file(obs/debian-versioned/changelog.in
  obs/debian-versioned/changelog @ONLY)
configure_file(obs/debian-versioned/control.in
  obs/debian-versioned/control @ONLY)
configure_file(obs/debian-versioned/copyright
  obs/debian-versioned/copyright)
# configure_file(obs/debian-versioned/install.in
#   obs/debian-versioned/dueca-${versioned}.install @ONLY)
configure_file(obs/debian-versioned/rules.in
  obs/debian-versioned/rules @ONLY)
configure_file(obs/dueca-versioned.dsc.in
  obs/dueca-versioned.dsc)
configure_file(obs/debian-versioned/compat
  obs/debian-versioned/compat)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/dueca-conf.h
  ${CMAKE_CURRENT_BINARY_DIR}/gencodegen.h
  ${CMAKE_CURRENT_BINARY_DIR}/oddoptions.h
  ${CMAKE_CURRENT_BINARY_DIR}/debprint.h
  ${CMAKE_CURRENT_BINARY_DIR}/dassert.h
  undebprint.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dueca)

set(CMAKEFILES

  ${CMAKE_CURRENT_BINARY_DIR}/cmake/DUECAConfig.cmake
  cmake/DuecaAddDCO.cmake
  cmake/DuecaActiveDCO.cmake
  cmake/FindDuecaCodegen.cmake
  cmake/DuecaAddModule.cmake
  cmake/DuecaGetConfig.cmake
  cmake/DuecaSetupProject.cmake
  cmake/DuecaReadModules.cmake
  cmake/HMILibAddIf.cmake)

install(FILES ${CMAKEFILES}
  DESTINATION ${CMAKE_INSTALL_DATADIR}/dueca/CMake)

find_program(LSB_RELEASE lsb_release)
execute_process(COMMAND ${LSB_RELEASE} -is
  OUTPUT_VARIABLE LSB_RELEASE_ID_SHORT
  OUTPUT_STRIP_TRAILING_WHITESPACE)
#message("LSBRELEASE ${LSB_RELEASE_ID_SHORT}" )

# scripting languages
set(SCRIPTLANG)

# python and boost only for python script interface
if (SCRIPT_PYTHON)

  if (APPLE)
    set(BPCOMP python311)

  else()
    find_library(BPLIB

      NAMES boost_python311 boost_python310 boost_python39 boost_python38
      boost_python37 boost_python-py311 boost_python-py310 boost_python-py39
      boost_python-py38 boost_python-py37 boost_python-py36
      boost_python3 boost_python-py3 boost_python27 boost_python2
      boost_python)

    if (BPLIB STREQUAL "BPLIB-NOTFOUND")
      message(FATAL_ERROR "Cannot find a boost-python library")
    endif()
    get_filename_component(BPCOMP ${BPLIB} NAME_WE)
    string(SUBSTRING ${BPCOMP} 9 -1 BPCOMP)
    message(STATUS "Searching for boost-python with component ${BPCOMP}")
    if (${BPCOMP} MATCHES "^python.*3.*$")
      set(PYREQ 3)
    else()
      message(FATAL_ERROR "Python 2 is obsolete")
      # set(PYREQ 2)
    endif()
  endif()

  # FindPython exists from cmake 3.12 onwards
  if (CMAKE_VERSION VERSION_LESS "3.12")
    message(STATUS "Older cmake, using PythonLibs to find python")
    # pkg_check_modules(Python python${PYREQ} REQUIRED)
    find_package(PythonLibs REQUIRED)
    set(Python_LIBRARIES ${PYTHON_LIBRARIES})
    set(Python_INCLUDE_DIRS ${PYTHON_INCLUDE_DIRS})
    find_program(Python_EXECUTABLE NAMES python3 python2 python)
    message(STATUS "Python libraries '${Python_LIBRARIES}'")
  else()
    find_package(Python COMPONENTS Interpreter Development)
    if (NOT Python_Development_FOUND)
      message(FATAL_ERROR "Need python development libraries >= 3")
    endif()
    if (Python_VERSION_MAJOR LESS "3")
      message(FATAL_ERROR "Need python >= 3")
    endif()
  endif()

  set(Boost_USE_STATIC_LIBS OFF)
  # set(Boost_USE_MULTITHREADED ON)
  set(Boost_USE_STATIC_RUNTIME OFF)
  find_package(Boost COMPONENTS ${BPCOMP})

  # this is for a specific failure on SLE 12 with new boost!
  if (Boost_LIBRARIES STREQUAL "")
    message(WARNING
      "Cannot find Boost python library through find_package, fallback")
    set(Boost_LIBRARIES ${BPLIB})
  else()
    message(STATUS "Boost (python) libraries \"${Boost_LIBRARIES}\"")
  endif()

  foreach(d ${Boost_INCLUDE_DIRS} ${Python_INCLUDE_DIRS})
    set(PYSCRIPTFLAGS "${PYSCRIPTFLAGS} -I${d}")
  endforeach()
  mylinker_arguments(OUTPUT PYSCRIPTLIBS
    LIBLIST ${Boost_LIBRARIES} ${Python_LIBRARIES})
  message(STATUS "PYSCRIPTLIBS: ${PYSCRIPTLIBS}")

  # extend the list of languages and suffixes
  list(APPEND SCRIPTLANG "PYTHON")
  set(SCRIPT_PYTHON_SUFFIX "py")
  set(SCRIPT_PYTHON_LIBS ${PYSCRIPTLIBS})
  set(SCRIPT_PYTHON_INCLUDES ${Boost_INCLUDE_DIRS}
    ${PYTHON_INCLUDE_DIRS} ${PYTHONEMBED_INCLUDE_DIRS} ${Python_INCLUDE_DIRS})
  set(SCRIPT_PYTHON_FLAGS ${PYSCRIPTFLAGS})
  message(STATUS "SCRIPT_PYTHON_LIBS: ${SCRIPT_PYTHON_LIBS}")

endif()

# python packages to test
# python packages needed
execute_process(COMMAND ${Python_EXECUTABLE} -c "import pyparsing"
  RESULT_VARIABLE PYTHON_HAVE_PYPARSING)
if (PYTHON_HAVE_PYPARSING)
  message(FATAL_ERROR "Missing Python module \"pyparsing\"")
endif()

execute_process(COMMAND ${Python_EXECUTABLE} -c "import pip"
  RESULT_VARIABLE PYTHON_HAVE_PIP)
if (PYTHON_HAVE_PIP)
  message(FATAL_ERROR "Missing Python module \"pip\"")
endif()

execute_process(COMMAND ${Python_EXECUTABLE} -c "import wheel"
  RESULT_VARIABLE PYTHON_HAVE_WHEEL)
if (PYTHON_HAVE_WHEEL)
  message(FATAL_ERROR "Missing Python module \"wheel\"")
endif()

# scheme now secondary language
if (SCRIPT_SCHEME)

  # extend the list of languages and suffixes
  list(APPEND SCRIPTLANG "SCHEME")
  set(SCRIPT_SCHEME_SUFFIX "sc")
  set(SCRIPT_SCHEME_LIBS ${SCRIPT_GUILE_LIBS})
  set(SCRIPT_SCHEME_INCLUDES ${SCRIPT_SCHEME_FLAGS})

endif()

# look for SVG converter
set(SVG_PNGFILE_ARG )
find_program(RSVGCONVERT rsvg-convert)
message(STATUS "SVG conversion: ${RSVGCONVERT}")
if (NOT RSVGCONVERT STREQUAL "RSVGCONVERT-NOTFOUND")
  EXECUTE_PROCESS(COMMAND ${RSVGCONVERT}
    --output=_dummy.png
    ${CMAKE_SOURCE_DIR}/doc/images/threads.svg
    RESULT_VARIABLE RSVG_RESULT OUTPUT_QUIET ERROR_QUIET)
  if (RSVG_RESULT EQUAL 0)
    message(STATUS "Using ${RSVG_CONVERT} for conversion to png")
    set(SVG_CONVERSION_COMMAND ${RSVGCONVERT})
    set(SVG_PNGFILE_ARG "--output")
    set(SVG_ICON_SIZE_ARG --width=18 --height=18 --keep-aspect-ratio)
  else()
    message(WARNING "Test failed: ${RSVGCONVERT} --output=/dev/null ${CMAKE_SOURCE_DIR}/doc/images/threads.svg")
  endif()
else()
  message(STATUS "Could not find rsvg-convert, trying inkscape")
endif()

if (NOT SVG_PNGFILE_ARG)
  # look for inkscape, for pixmaps and doc
  find_program(INKSCAPE inkscape
    PATHS "/Applications/Inkscape.app/Contents/Resources/bin/inkscape"
    "/opt/local/bin")

  if (NOT INSCAPE STREQUAL "INKSCAPE-NOTFOUND")

    set(SVG_CONVERSION_COMMAND ${INKSCAPE})
    set(SVG_ICON_SIZE_ARG
      --export-area-page --export-width=18 --export-height=18)

    # figure out the required argument for png export
    EXECUTE_PROCESS(COMMAND ${INKSCAPE} --export-filename="test.png"
      RESULT_VARIABLE INKSCAPE_RESULT OUTPUT_QUIET ERROR_QUIET)
    if (INKSCAPE_RESULT EQUAL 0)
      set(SVG_PNGFILE_ARG "--export-filename")
    else()
      EXECUTE_PROCESS(COMMAND ${INKSCAPE} --export-png="test.png"
	RESULT_VARIABLE INKSCAPE_RESULT OUTPUT_QUIET ERROR_QUIET)
    endif()
    if (NOT SVG_PNGFILE_ARG AND INKSCAPE_RESULT EQUAL 0)
      set(SVG_PNGFILE_ARG "--export-png")
    else()
      EXECUTE_PROCESS(COMMAND ${INKSCAPE} --export-file="test.png"
	RESULT_VARIABLE INKSCAPE_RESULT OUTPUT_QUIET ERROR_QUIET)
    endif()
    if (NOT SVG_PNGFILE_ARG AND INKSCAPE_RESULT EQUAL 0)
      set(SVG_PNGFILE_ARG "--export-file")
    endif()
    if (NOT SVG_PNGFILE_ARG)
      message(FATAL "Cannot determine png export option for inkscape")
    endif()
  endif()
  message(STATUS "inkscape export PNG command ${SVG_PNGFILE_ARG}")
endif()

# subdirs choice
add_subdirectory(dueca)
add_subdirectory(scripts)
add_subdirectory(pycodegen)
add_subdirectory(pixmaps)
if (BUILD_UDP)
  add_subdirectory(udpcom)
endif()
if (BUILD_WEBSOCK)
  add_subdirectory(websock)
endif()

if (RUN_TESTS)
  enable_testing()
  add_subdirectory(test)
endif()

if (BUILD_RTWPARSER)
  add_subdirectory(rtwparser)
endif()
if (BUILD_EXTRA)
  add_subdirectory(extra)
endif()
if (BUILD_INTER)
  if (NOT BUILD_UDP)
    message(FATAL_ERROR "Inter-dueca communication requires BUILD_UDP")
  endif()
  add_subdirectory(inter)
endif()
if (BUILD_DDFF)
  add_subdirectory(ddff)
endif()
if (BUILD_DUSIME)
  add_subdirectory(dusime)
endif()
if (BUILD_HDF5)
  add_subdirectory(hdf5utils)
endif()
if (BUILD_DOC)
  add_subdirectory(doc)
endif()
add_subdirectory(gitscripts)
