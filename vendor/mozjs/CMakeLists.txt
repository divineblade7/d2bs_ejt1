cmake_minimum_required(VERSION 3.23)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# TODO: Make it an option to chose /MD or /MT
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

option(MOZJS_BUILD_TESTS "Build tests for mozjs" OFF)

project(mozjs)

include(ConfigurePlatform)

add_subdirectory(js)
add_subdirectory(mfbt)

if(MOZJS_BUILD_TESTS)
  enable_testing()
  include(PrepareTest)
  add_subdirectory(tests)
endif()
