function (MYLINKER_ARGUMENTS)

  # mylinker_arguments(OUTPUT xx_libs LIBLIST libraries [ libraries ])
  #
  # process a series of library references (library files, -l or -L options)
  # into linker arguments. Results is returned in the OUTPUT variable as a
  # string with linker arguments.

  # extensions processed
  set(EXTENSIONS a so dylib)

  # output argument
  set(oneValueArgs OUTPUT)

  # multiple lists accepted as input arguments
  set(multiValueArgs LIBLIST)

  # parse the arguments
  cmake_parse_arguments(MYLINKER_ARGUMENTS
    "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if (MYLINKER_ARGUMENTS_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "MYLINKER_ARGUMENTS, cannot process ${MYLINKER_ARGUMENTS_UNPARSED_ARGUMENTS}")
  endif()

  # haven't found a good set of default library paths; fudging it here
  if(UNIX)
    set(MY_SYSTEM_LIBPATHS /usr/lib /usr/lib64 /lib /lib64 /usr/lib/x86_64-linux-gnu)
  elseif(APPLE)
    set(MY_SYSTEM_LIBPATHS /usr/lib /usr/lib64 /lib /lib64)
  endif()

  #message(STATUS "LIBLIST ${MYLINKER_ARGUMENTS_LIBLIST}")

  set(resultLibs "")
  set(resultDirs "")
  #message(STATUS "DEFAULT PATH ${CMAKE_SYSTEM_FRAMEWORK_PATH}")

  foreach(l ${MYLINKER_ARGUMENTS_LIBLIST})

    if (l MATCHES "^[-]l.*$")

      # already a library flag
      # message(STATUS "Already lib ${l}")
      list(APPEND resultLibs ${l})

    elseif (l MATCHES "^[-]L.*$")

      # library location flag
      # message(STATUS "Already dir ${l}")
      list(APPEND resultDirs ${l})

    elseif (l MATCHES "^[-].*$")

      # a different kind of flag
      # message(STATUS "Flag found ${l}")
      list(APPEND resultLibs ${l})

    elseif(l MATCHES "^Boost\\:\\:.*$")

      # sept 2020, got a problem with Ubuntu 20.04 claiming
      # Boost::python, Boost::system etc were found as library files
      # and returned in Boost_LIBRARIES with:
      #   find_package(Boost 1.60 COMPONENTS ...)
      message(WARNING "Fixing strange Ubuntu lib name '${l}'")
      string(REGEX REPLACE "^Boost\\:\\:(.*)$" "boost_\\1" LIB "${l}")
      if (LIB STREQUAL "boost_python")
        find_library(BPLIB NAMES boost_python310 boost_python39 boost_python38)
	string(REGEX REPLACE "^.*libboost_python([0-9]*)\\.so$"
	  "boost_python\\1" PLIB "${BPLIB}")
        list(APPEND resultLibs "-l${PLIB}")
      else()
        list(APPEND resultLibs "-l${LIB}")
      endif()

    else()

      # This appears to be a library file. Turn it into folder and
      # -l option
      get_filename_component(DIR ${l} DIRECTORY)
      get_filename_component(FN ${l} NAME)
      set(F)

      # test for all known extensions
      foreach(ex ${EXTENSIONS})
        # message(STATUS "F ${F} ex ${ex} fn ${FN} ^(.*)\\.${ex}$")
        string(REGEX MATCH ".*\\.${ex}" FND ${FN})
        if (FND)
          string(REGEX REPLACE "^(.*)\\.${ex}$" "\\1" F ${FN})
          # message(STATUS "FND ${FND} ex ${ex} F ${F} RE ^(.*)\\.${ex}$")
          set(E ${ex})
        endif()
      endforeach()
      # message(STATUS "${l} -> ${DIR} / ${F} . ${E}")

      if(DIR)

        # is dir default or known
        list(FIND resultDirs ${DIR} DIRFOUND)
        list(FIND MY_SYSTEM_LIBPATHS ${DIR} STDDIRFOUND)

        # add a -L option
        if (DIRFOUND EQUAL -1 AND STDDIRFOUND EQUAL -1)
          list(APPEND resultDirs "-L${DIR}")
        endif()

        # find the -l option
        if (F)
          string(REGEX REPLACE "^lib\(.*\)" "\\1" LIB ${F})
          # message(STATUS "libpart ${LIB}")
          list(FIND resultLibs ${LIB} LIBFOUND)
          if (LIBFOUND EQUAL -1)
            list(APPEND resultLibs "-l${LIB}")
          endif()
          # message(STATUS "${LIBFOUND} not yet in lib")
        endif()
      else()
        # message(STATUS "Ignoring library argument ${l}")
      endif()
    endif()

    # message(STATUS "dirs ${resultDirs} libs ${resultLibs}")
  endforeach()

  # convert this to a single library options line
  foreach(l ${resultDirs})
    if (linkline)
      set(linkline "${linkline} ${l}")
    else()
      set(linkline "${l}")
    endif()
  endforeach()
  foreach(l ${resultLibs})
    if (linkline)
      set(linkline "${linkline} ${l}")
    else()
      set(linkline "${l}")
    endif()
  endforeach()

  message(STATUS "setting ${MYLINKER_ARGUMENTS_OUTPUT} to ${linkline}")
  separate_arguments(resultLibs)
  set(${MYLINKER_ARGUMENTS_OUTPUT} ${linkline} PARENT_SCOPE)
endfunction(MYLINKER_ARGUMENTS)
