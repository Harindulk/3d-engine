cmake_minimum_required(VERSION 3.20)
project(Aurora3D VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option(AURORA_USE_EXTERNAL_GLFW "Use external/glfw subdir instead of system package" ON)
option(AURORA_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)
option(AURORA_FORCE_VALIDATION "Force-enable Vulkan validation layers (overrides build type)" OFF)

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

# Engine library (public headers in engine/include)
file(GLOB_RECURSE AURORA_ENGINE_SRC CONFIGURE_DEPENDS engine/src/*.cpp)
add_library(aurora_engine STATIC ${AURORA_ENGINE_SRC})
target_include_directories(aurora_engine PUBLIC ${CMAKE_SOURCE_DIR}/engine/include)
target_include_directories(aurora_engine PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/engine/src)

# Temporary: engine still depends on current src/ implementation (App & Vulkan managers) until migrated
file(GLOB_RECURSE AURORA_LEGACY_SRC CONFIGURE_DEPENDS src/*.cpp src/*.h)
target_sources(aurora_engine PRIVATE ${AURORA_LEGACY_SRC})

source_group(TREE ${CMAKE_SOURCE_DIR}/engine/src PREFIX "Engine Source" FILES ${AURORA_ENGINE_SRC})
source_group(TREE ${CMAKE_SOURCE_DIR}/src PREFIX "Legacy Source" FILES ${AURORA_LEGACY_SRC})

# Original executable (will later become a sample or be removed)
add_executable(aurora3d src/main.cpp)
target_link_libraries(aurora3d PRIVATE aurora_engine)

# Sample minimal game using engine API
add_executable(minimal_game samples/MinimalGame/main.cpp)
target_link_libraries(minimal_game PRIVATE aurora_engine)

if(AURORA_FORCE_VALIDATION)
  message(STATUS "Aurora3D: forcing validation layers ON (AURORA_FORCE_VALIDATION=ON)")
  target_compile_definitions(aurora3d PRIVATE AURORA_ENABLE_VALIDATION)
elseif(CMAKE_BUILD_TYPE MATCHES Debug)
  message(STATUS "Aurora3D: enabling validation layers for Debug build")
  target_compile_definitions(aurora3d PRIVATE AURORA_ENABLE_VALIDATION)
else()
  message(STATUS "Aurora3D: validation layers disabled")
endif()

target_link_libraries(aurora_engine PRIVATE ${GLFW_LIB} Vulkan::Vulkan)
target_link_libraries(aurora3d PRIVATE aurora_engine)
target_link_libraries(minimal_game PRIVATE aurora_engine)

if(APPLE)
  target_link_libraries(aurora3d PRIVATE "-framework Cocoa" "-framework IOKit" "-framework CoreFoundation")
endif()

set_target_properties(aurora3d PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set_target_properties(minimal_game PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# --- Shader compilation (triangle example) ---
find_program(GLSLANG_VALIDATOR glslangValidator HINTS $ENV{VULKAN_SDK}/bin)
if(GLSLANG_VALIDATOR)
  message(STATUS "Found glslangValidator: ${GLSLANG_VALIDATOR}")
  set(SHADER_SRC ${CMAKE_SOURCE_DIR}/src/shaders)
  set(SHADER_OUT ${CMAKE_BINARY_DIR}/shaders)
  file(MAKE_DIRECTORY ${SHADER_OUT})
  add_custom_command(
    OUTPUT ${SHADER_OUT}/triangle.vert.spv
    COMMAND ${GLSLANG_VALIDATOR} -V ${SHADER_SRC}/triangle.vert -o ${SHADER_OUT}/triangle.vert.spv
    DEPENDS ${SHADER_SRC}/triangle.vert
  )
  add_custom_command(
    OUTPUT ${SHADER_OUT}/triangle.frag.spv
    COMMAND ${GLSLANG_VALIDATOR} -V ${SHADER_SRC}/triangle.frag -o ${SHADER_OUT}/triangle.frag.spv
    DEPENDS ${SHADER_SRC}/triangle.frag
  )
  add_custom_target(shaders ALL DEPENDS ${SHADER_OUT}/triangle.vert.spv ${SHADER_OUT}/triangle.frag.spv)
  add_dependencies(aurora3d shaders)
else()
  message(STATUS "glslangValidator not found; ensure SPIR-V shaders exist in build/shaders or install Vulkan SDK")
endif()
