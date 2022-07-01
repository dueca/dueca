function(APPEND_STATIC)
  cmake_parse_arguments(APPEND_STATIC
    "" "OUTPUT" "" ${ARGN})
  set(resultlist)
  # message(STATUS "args ${APPEND_STATIC_UNPARSED_ARGUMENTS} ${ARGN}")
  foreach(l ${APPEND_STATIC_UNPARSED_ARGUMENTS})
    if (${l} MATCHES "^[-].*$")
      #message(STATUS "match 1:${l}")
      list(APPEND resultlist ${l})
    elseif (${l} MATCHES "^.*[.]so$")
      #message(STATUS "match 2:${l}")
      list(APPEND resultlist ${l})
    elseif (${l} MATCHES "^dueca.*$")
      #message(STATUS "match d:${l}")
      if (TARGET ${l}-static)
        list(APPEND resultlist ${l}-static)
      else()
        if (${l} MATCHES "dueca.*-sc|dueca.*-py")
          # no message
          # message(STATUS "language link file ${l}")
        else()
          # message(WARNING "Expected to have ${l}-static, but did not find it")
        endif()
        list(APPEND resultlist ${l})
      endif()
    else()
      #message(STATUS "nomatch:${l}")
      list(APPEND resultlist ${l})
    endif()
  endforeach()
  # message(STATUS "set(${APPEND_STATIC_OUTPUT} ${resultlist} PARENT_SCOPE)")
  set(${APPEND_STATIC_OUTPUT} ${resultlist} PARENT_SCOPE)
endfunction(APPEND_STATIC)


macro(DUECA_ADD_LIBRARY)

  set(DAL_COMPILE_OPTIONS)

  #message(STATUS "Parsing ${ARGN}")
  # parse the arguments
  cmake_parse_arguments(DAL
    "FORCESTATIC" ""
    "COMPILEOPTIONS;SOURCES;LINKLIBS;INCLUDEDIRS;LINKDIRS;LINKOPTIONS" ${ARGN})
  list(LENGTH DAL_UNPARSED_ARGUMENTS NUNPARSED)
  if (NUNPARSED EQUAL 1)
    set(LIBNAME ${DAL_UNPARSED_ARGUMENTS})
  else()
    message(ERROR "Cannot find single lib name in ${DAL_UNPARSED_ARGUMENTS}")
  endif()

  #  message(STATUS "dueca_add_library SOURCES ${DAL_SOURCES}
  #COMPILEOPTIONS :${DAL_COMPILEOPTIONS}:
  #LINKLIBS :${DAL_LINKLIBS}:
  #LIBNAME :${LIBNAME}")

  if(DAL_LINKDIRS)
    link_directories(${DAL_LINKDIRS})
  endif()

  if (DAL_FORCESTATIC)

    message(STATUS "Forced static linkage for ${LIBNAME}")
    add_library(${LIBNAME} STATIC ${DAL_SOURCES})
    set_target_properties(${LIBNAME} PROPERTIES
      OUTPUT_NAME ${LIBNAME} VERSION ${dueca_VERSION})
      append_static(${DAL_LINKLIBS} OUTPUT STATLIBS)
    target_link_libraries(${LIBNAME} PUBLIC ${STATLIBS})
    if (DAL_LINKOPTIONS)
      target_link_options(${LIBNAME} PUBLIC ${DAL_LINKOPTIONS})
    endif()
    if (DAL_COMPILEOPTIONS)
      target_compile_options(${LIBNAME} PUBLIC ${DAL_COMPILEOPTIONS})
    endif()
    if (DAL_INCLUDEDIRS)
      target_include_directories(${LIBNAME} PUBLIC ${DAL_INCLUDEDIRS})
    endif()
    install(TARGETS ${LIBNAME}
      ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

  else()
    if(DUECA_BUILD_STATIC)

      add_library(${LIBNAME}-static STATIC ${DAL_SOURCES})
      set_target_properties(${LIBNAME}-static PROPERTIES
        OUTPUT_NAME ${LIBNAME}-static VERSION ${dueca_VERSION})
      append_static(${DAL_LINKLIBS} OUTPUT STATLIBS)
      #message(STATUS "statlibs ${STATLIBS}")
      target_link_libraries(${LIBNAME}-static PUBLIC ${STATLIBS})
      if (DAL_LINKOPTIONS)
        target_link_options(${LIBNAME}-static PUBLIC ${DAL_LINKOPTIONS})
      endif()
      if (DAL_COMPILEOPTIONS)
        target_compile_options(${LIBNAME}-static PUBLIC ${DAL_COMPILEOPTIONS})
      endif()
      if (DAL_INCLUDEDIRS)
        target_include_directories(${LIBNAME}-static PUBLIC ${DAL_INCLUDEDIRS})
      endif()

      install(TARGETS ${LIBNAME}-static
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
    endif()
    if(DUECA_BUILD_SHARED)

      add_library(${LIBNAME} SHARED ${DAL_SOURCES})
      set_target_properties(${LIBNAME} PROPERTIES
        VERSION ${dueca_VERSION} SOVERSION ${SOVERSION})
      target_link_libraries(${LIBNAME} PUBLIC ${DAL_LINKLIBS})
      if (DAL_LINKOPTIONS)
        target_link_options(${LIBNAME} PUBLIC ${DAL_LINKOPTIONS})
      endif()
      if (DAL_COMPILEOPTIONS)
        target_compile_options(${LIBNAME} PUBLIC ${DAL_COMPILEOPTIONS})
      endif()
      if (DAL_INCLUDEDIRS)
        target_include_directories(${LIBNAME} PUBLIC ${DAL_INCLUDEDIRS})
      endif()

      install(TARGETS ${LIBNAME}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

    endif()
  endif()
endmacro()

