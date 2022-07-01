cmake_minimum_required(VERSION 3.0)

include(FindPkgConfig)
include(FindOpenGL)

pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
pkg_check_modules(EPOXY REQUIRED epoxy)
find_package(OpenGL)

add_executable(gtk3gl.x gtk3gl.cxx)
target_compile_options(gtk3gl.x PUBLIC ${GTK3_CFLAGS} ${OPENGL_CFLAGS})
target_include_directories(gtk3gl.x PUBLIC ${GTK3_INCLUDE_DIRS}
  ${OPENGL_INCLUDE_DIRS})
set_property(TARGET gtk3gl.x PROPERTY CXX_STANDARD 11)
target_link_libraries(gtk3gl.x PUBLIC
  ${GTK3_LIBRARIES} ${OPENGL_LIBRARIES} ${GTK3_LDFLAGS})

add_executable(gtk3gls.x gtk3gls.cxx)
target_compile_options(gtk3gls.x PUBLIC
  ${GTK3_CFLAGS} ${OPENGL_CFLAGS} ${EPOXY_CFLAGS})
target_include_directories(gtk3gls.x PUBLIC ${GTK3_INCLUDE_DIRS}
  ${OPENGL_INCLUDE_DIRS} ${EPOXY_INCLUDE_DIRS})
set_property(TARGET gtk3gls.x PROPERTY CXX_STANDARD 11)
target_link_libraries(gtk3gls.x PUBLIC
  ${GTK3_LIBRARIES} ${OPENGL_LIBRARIES} ${GTK3_LDFLAGS} ${EPOXY_LIBRARIES})
