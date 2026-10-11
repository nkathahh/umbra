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
  add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()
