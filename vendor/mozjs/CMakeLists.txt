cmake_minimum_required(VERSION 3.23)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# TODO: Make it an option to chose /MD or /MT
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(mozjs)

include(ConfigurePlatform)

add_subdirectory(js)
add_subdirectory(mfbt)

enable_testing()
include(PrepareTest)
add_subdirectory(tests)
