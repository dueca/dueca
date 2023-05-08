set(PIXMAPS active.xpm fell-back.xpm incomplete.xpm abort.xpm
  confirm-abort.xpm inactive.xpm inprogress.xpm)

install(FILES ${PIXMAPS}
  DESTINATION ${CMAKE_INSTALL_DATADIR}/dueca/pixmaps)

set(SVGIMG

  event-logo.svg multi-logo.svg picking-logo.svg stream-logo.svg
  label-logo.svg number-logo.svg sequential-logo.svg)

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

add_custom_target(pngfiles ALL DEPENDS ${PNGFILES})
install(FILES ${PNGFILES}
  DESTINATION ${CMAKE_INSTALL_DATADIR}/dueca/pixmaps)

