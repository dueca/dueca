set(PIXMAPS active.xpm fell-back.xpm incomplete.xpm abort.xpm
  confirm-abort.xpm inactive.xpm inprogress.xpm)

install(FILES ${PIXMAPS}
  DESTINATION ${CMAKE_INSTALL_DATADIR}/dueca/pixmaps)

set(SVGDIRECT
  active.svg fell-back.svg incomplete.svg abort.svg
  confirm-abort.svg inactive.svg inprogress.svg
)

set(SVGIMG

  event-logo.svg multi-logo.svg picking-logo.svg stream-logo.svg
  label-logo.svg number-logo.svg sequential-logo.svg
)


# scale the svg buttons
if (NOT RSVGCONVERT STREQUAL "RSVGCONVERT-NOTFOUND")
  foreach(SVG ${SVGDIRECT})
    add_custom_command(
      OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${SVG}
      COMMAND 
      ${RSVGCONVERT} -f svg -z 0.7 ${CMAKE_CURRENT_SOURCE_DIR}/${SVG} --output ${SVG}
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      DEPENDS ${SVG})
    list(APPEND SVGFILES ${CMAKE_CURRENT_BINARY_DIR}/${SVG})
  endforeach()
else()
  # alas, no scaling
  set(SVGFILES ${SVGDIRECT})
endif()

# convert svg files to png
foreach(SVG ${SVGIMG})
  string(REPLACE .svg .png PNG ${SVG})
  message(STATUS "Adding target ${PNG}, using ${SVG_CONVERSION_COMMAND}")
  add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${PNG}
    COMMAND
    ${SVG_CONVERSION_COMMAND}
    ${SVG_PNGFILE_ARG}="${CMAKE_CURRENT_BINARY_DIR}/${PNG}"
    ${SVG_ICON_SIZE_ARG}
    "${CMAKE_CURRENT_SOURCE_DIR}/${SVG}"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS ${SVG})
  list(APPEND PNGFILES ${CMAKE_CURRENT_BINARY_DIR}/${PNG})
endforeach()

foreach(SVG ${SVGDIRECT})
  string(REPLACE .svg .png PNG ${SVG})
  message(STATUS "Adding target ${PNG}, using ${SVG_CONVERSION_COMMAND}")
  add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${PNG}
    COMMAND
    ${SVG_CONVERSION_COMMAND}
    ${SVG_PNGFILE_ARG}="${CMAKE_CURRENT_BINARY_DIR}/${PNG}"
    ${SVG_ICON_SIZE2_ARG}
    "${CMAKE_CURRENT_SOURCE_DIR}/${SVG}"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS ${SVG})
  list(APPEND PNGFILES ${CMAKE_CURRENT_BINARY_DIR}/${PNG})
endforeach()

add_custom_target(pngfiles ALL DEPENDS ${PNGFILES})
add_custom_target(svgfiles ALL DEPENDS ${SVGFILES})

install(FILES ${PNGFILES} ${SVGFILES}
  DESTINATION ${CMAKE_INSTALL_DATADIR}/dueca/pixmaps)

