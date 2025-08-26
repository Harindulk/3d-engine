cmake_minimum_required(VERSION 3.20)
project(Aurora3D VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option(AURORA_USE_EXTERNAL_GLFW "Use external/glfw subdir instead of system package" ON)
option(AURORA_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)

if(MSVC)
  add_compile_options(/W4)
  if(AURORA_WARNINGS_AS_ERRORS)
    add_compile_options(/WX)
  endif()
else()
  add_compile_options(-Wall -Wextra -Wshadow -Wconversion -Wpedantic)
  if(AURORA_WARNINGS_AS_ERRORS)
    add_compile_options(-Werror)
  endif()
endif()

# GLFW
if(AURORA_USE_EXTERNAL_GLFW AND EXISTS ${CMAKE_SOURCE_DIR}/external/glfw/CMakeLists.txt)
  set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
  set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
  add_subdirectory(external/glfw)
  set(GLFW_LIB glfw)
else()
  find_package(glfw3 REQUIRED)
  set(GLFW_LIB glfw)
endif()

# Vulkan
find_package(Vulkan REQUIRED)

# Sources
file(GLOB_RECURSE AURORA_SOURCES CONFIGURE_DEPENDS src/*.cpp src/*.h)
add_executable(aurora3d ${AURORA_SOURCES})
source_group(TREE ${CMAKE_SOURCE_DIR}/src PREFIX "Source" FILES ${AURORA_SOURCES})

target_include_directories(aurora3d PRIVATE ${CMAKE_SOURCE_DIR}/src)

if(CMAKE_BUILD_TYPE MATCHES Debug)
  target_compile_definitions(aurora3d PRIVATE AURORA_ENABLE_VALIDATION)
endif()

target_link_libraries(aurora3d PRIVATE ${GLFW_LIB} Vulkan::Vulkan)

if(APPLE)
  target_link_libraries(aurora3d PRIVATE "-framework Cocoa" "-framework IOKit" "-framework CoreFoundation")
endif()

set_target_properties(aurora3d PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
