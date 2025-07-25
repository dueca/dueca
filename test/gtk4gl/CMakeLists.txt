cmake_minimum_required(VERSION 3.12)

include(FindPkgConfig)
include(FindOpenGL)

pkg_check_modules(GTK4 REQUIRED gtk4)
pkg_check_modules(EPOXY REQUIRED epoxy)


add_executable(gtk4gls.x gtk4gls.cxx)
target_compile_options(gtk4gls.x PUBLIC
  ${GTK4_CFLAGS} ${OPENGL_CFLAGS} ${EPOXY_CFLAGS})
target_include_directories(gtk4gls.x PUBLIC ${GTK4_INCLUDE_DIRS}
  ${OPENGL_INCLUDE_DIRS} ${EPOXY_INCLUDE_DIRS})
set_property(TARGET gtk4gls.x PROPERTY CXX_STANDARD 17)
target_link_libraries(gtk4gls.x PUBLIC
  ${GTK4_LIBRARIES} ${OPENGL_LIBRARIES} ${GTK4_LDFLAGS} ${EPOXY_LIBRARIES})

  add_executable(gtkai.x gtkai.cxx)
target_compile_options(gtkai.x PUBLIC
  ${GTK4_CFLAGS} ${OPENGL_CFLAGS} ${EPOXY_CFLAGS})
target_include_directories(gtkai.x PUBLIC ${GTK4_INCLUDE_DIRS}
  ${OPENGL_INCLUDE_DIRS} ${EPOXY_INCLUDE_DIRS})
set_property(TARGET gtkai.x PROPERTY CXX_STANDARD 17)
target_link_libraries(gtkai.x PUBLIC
  ${GTK4_LIBRARIES} ${OPENGL_LIBRARIES} ${GTK4_LDFLAGS} ${EPOXY_LIBRARIES})
