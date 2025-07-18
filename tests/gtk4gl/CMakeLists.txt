cmake_minimum_required(VERSION 3.30)
project(gtk4glarea)
enable_language(C)
enable_language(CXX)

find_package(PkgConfig REQUIRED)
include(GNUInstallDirs)

pkg_check_modules(GTK4 REQUIRED gtk4)
pkg_check_modules(FREETYPE freetype2 REQUIRED)
pkg_check_modules(EPOXY epoxy REQUIRED)

set(SOURCES main.cxx display.cxx display.hxx TextRenderer.cxx TextRenderer.hxx VaoVbo.cxx Shader.cxx)

add_executable(gtk4glarea.x ${SOURCES})
target_link_libraries(gtk4glarea.x ${GTK4_LIBRARIES} ${FREETYPE_LIBRARIES} ${EPOXY_LIBRARIES})
target_include_directories(gtk4glarea.x PUBLIC ${GTK4_INCLUDE_DIRS} ${FREETYPE_INCLUDE_DIRS} ${EPOXY_INCLUDE_DIRS})