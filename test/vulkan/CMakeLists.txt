cmake_minimum_required(VERSION 3.0)

include(FindPkgConfig)

if (APPLE)
  pkg_check_modules(GLFW REQUIRED glfw3)
  set(VULKAN_LIBRARIES /Users/repa/MoltenVK/MoltenVK/macOS/libMoltenVK.dylib)
  set(VULKAN_INCLUDEDIR /Users/repa/MoltenVK/MoltenVK/include)
else()
  pkg_check_modules(GLM REQUIRED glm)
  pkg_check_modules(GLFW REQUIRED glfw3)
  pkg_check_modules(VULKAN REQUIRED vulkan)
endif()

add_executable(vulkan01.x vulkan01.cxx)
target_compile_options(vulkan01.x PUBLIC
  ${GLM_CFLAGS} ${GLFW_CFLAGS} ${VULKAN_CFLAGS})
target_include_directories(vulkan01.x PUBLIC ${VULKAN_INCLUDEDIR})
set_property(TARGET vulkan01.x PROPERTY CXX_STANDARD 11)
target_link_libraries(vulkan01.x PUBLIC
  ${GLM_LIBRARIES} ${GLFW_LIBRARIES} ${VULKAN_LIBRARIES})

add_executable(vulkan02.x vulkan02.cxx)
target_compile_options(vulkan02.x PUBLIC
  ${GLFW_CFLAGS} ${VULKAN_CFLAGS})
target_include_directories(vulkan02.x PUBLIC ${VULKAN_INCLUDEDIR})
set_property(TARGET vulkan02.x PROPERTY CXX_STANDARD 11)
target_link_libraries(vulkan02.x PUBLIC
  ${GLM_LIBRARIES} ${GLFW_LIBRARIES} ${VULKAN_LIBRARIES})
