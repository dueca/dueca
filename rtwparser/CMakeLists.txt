find_package(BISON)
find_package(FLEX)

BISON_TARGET(THEPARSER rtwsyntax.yxx
  ${CMAKE_CURRENT_BINARY_DIR}/rtwsyntax.cxx)
FLEX_TARGET(THESCANNER rtwgrammar.l
  ${CMAKE_CURRENT_BINARY_DIR}/rtwgrammar.c)
ADD_FLEX_BISON_DEPENDENCY(THESCANNER THEPARSER)

set(SOURCES BaseObject.cxx
  TypeNameDef.cxx NamedStruct.cxx
  rtwparser.cxx)

include_directories("${CMAKE_BINARY_DIR}/dueca"
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(dueca-rtwparser ${SOURCES}
  ${BISON_THEPARSER_OUTPUTS} ${FLEX_THESCANNER_OUTPUTS})

install(TARGETS dueca-rtwparser
  DESTINATION ${CMAKE_INSTALL_BINDIR})
