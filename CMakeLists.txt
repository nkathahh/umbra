cmake_minimum_required(VERSION 3.16)
project(umbra_cp1 CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
if(MSVC)
  add_compile_options(/W3)
  add_compile_definitions(_USE_MATH_DEFINES)  # M_PI on MSVC
else()
  add_compile_options(-Wall -Wextra)
endif()

include_directories(include)

# Library: every src/*.cpp except the demos. Adding a new module only needs a
# new .cpp in src/ (re-run cmake), so team members never have to edit this file.
file(GLOB CORE_SOURCES CONFIGURE_DEPENDS src/*.cpp)
list(FILTER CORE_SOURCES EXCLUDE REGEX ".*/demo_[^/]*\\.cpp$")
add_library(umbra_core ${CORE_SOURCES})

# Demos: every src/demo_*.cpp becomes an executable named after the file.
file(GLOB DEMO_SOURCES CONFIGURE_DEPENDS src/demo_*.cpp)
foreach(demo_src ${DEMO_SOURCES})
  get_filename_component(demo_name ${demo_src} NAME_WE)
  add_executable(${demo_name} ${demo_src})
  target_link_libraries(${demo_name} PRIVATE umbra_core)
endforeach()

# Tests: every tests/test_*.cpp becomes a CTest test (run with: ctest --test-dir build).
enable_testing()
file(GLOB TEST_SOURCES CONFIGURE_DEPENDS tests/test_*.cpp)
foreach(test_src ${TEST_SOURCES})
  get_filename_component(test_name ${test_src} NAME_WE)
  add_executable(${test_name} ${test_src})
  target_link_libraries(${test_name} PRIVATE umbra_core)
  # Run from the repository root so tests can read levels/ and write to output/.
  add_test(NAME ${test_name} COMMAND ${test_name} WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
endforeach()

# ---- The OpenGL game (final version) ---------------------------------------
# umbra_game opens a window and renders the level in real 3D with OpenGL 3.3:
# textured walls, torch lighting in shaders, sprites, a minimap and a HUD.
# GLFW (window and keyboard) is downloaded from GitHub the first time CMake
# runs, so that first configure needs an internet connection. The OpenGL
# function loader (glad) is generated code kept in external/glad.
option(UMBRA_BUILD_GAME "Build the OpenGL game (downloads GLFW on first configure)" ON)

# Optional: point at a GLFW that is already built instead of downloading it.
# Normally left empty. (Used for testing on machines without GLFW's build
# dependencies.)
set(UMBRA_GLFW_INCLUDE_DIR "" CACHE PATH "Folder holding GLFW/glfw3.h (optional)")
set(UMBRA_GLFW_LIBRARY "" CACHE FILEPATH "A prebuilt GLFW library (optional)")

if(UMBRA_BUILD_GAME)
  enable_language(C)

  if(UMBRA_GLFW_INCLUDE_DIR AND UMBRA_GLFW_LIBRARY)
    add_library(glfw UNKNOWN IMPORTED)
    set_target_properties(glfw PROPERTIES
      IMPORTED_LOCATION "${UMBRA_GLFW_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${UMBRA_GLFW_INCLUDE_DIR}")
  else()
    include(FetchContent)
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(glfw
      GIT_REPOSITORY https://github.com/glfw/glfw.git
      GIT_TAG 3.4
      GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(glfw)
  endif()

  add_library(glad STATIC external/glad/src/gl.c)
  target_include_directories(glad PUBLIC external/glad/include)

  file(GLOB GAME_SOURCES CONFIGURE_DEPENDS game/*.cpp)
  add_executable(umbra_game ${GAME_SOURCES})
  target_link_libraries(umbra_game PRIVATE umbra_core glad glfw ${CMAKE_DL_LIBS})
endif()
