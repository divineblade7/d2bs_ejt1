cmake_minimum_required(VERSION 3.23)

include(cmake/d2bs_version.cmake)
d2bs_extract_version()

project(d2bs VERSION ${D2BS_VERSION})
message(STATUS "Build d2bs ${D2BS_VERSION}")

# Set default built to release
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose Release or Debug" FORCE)
endif()

# Set runtime linking to /MT
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 20)
  set(CMAKE_CXX_STANDARD_REQUIRED True)
endif()

option(D2BS_ENABLE_PROFILING "Enable profiling" OFF)

option(D2BS_NO_EXCEPTIONS "Enable profiling" OFF)

# Set D2BS sources
set(D2BS_SRCS
  src/d2bs/core/ActMap.cpp
  src/d2bs/core/Control.cpp
  src/d2bs/core/Core.cpp
  src/d2bs/core/File.cpp
  src/d2bs/core/Game.cpp
  src/d2bs/core/Hash.cpp
  src/d2bs/core/LevelMap.cpp
  src/d2bs/core/MPQStats.cpp
  src/d2bs/core/Profile.cpp
  src/d2bs/core/Room.cpp
  src/d2bs/core/ScreenHook.cpp
  src/d2bs/core/Unit.cpp
  
  src/d2bs/diablo/handlers/D2Handlers.cpp
  src/d2bs/diablo/handlers/D2NetHandlers.cpp
  src/d2bs/diablo/patches/D2Intercepts.cpp
  src/d2bs/diablo/D2Helpers.cpp

  src/d2bs/script/api/JSArea.cpp
  src/d2bs/script/api/JSControl.cpp
  src/d2bs/script/api/JSCore.cpp
  src/d2bs/script/api/JSDirectory.cpp
  src/d2bs/script/api/JSExits.cpp
  src/d2bs/script/api/JSFile.cpp
  src/d2bs/script/api/JSFileTools.cpp
  src/d2bs/script/api/JSGame.cpp
  src/d2bs/script/api/JSGlobalClasses.cpp
  src/d2bs/script/api/JSHash.cpp
  src/d2bs/script/api/JSMenu.cpp
  src/d2bs/script/api/JSParty.cpp
  src/d2bs/script/api/JSPresetUnit.cpp
  src/d2bs/script/api/JSProfile.cpp
  src/d2bs/script/api/JSRoom.cpp
  src/d2bs/script/api/JSSandbox.cpp
  src/d2bs/script/api/JSScreenHook.cpp
  src/d2bs/script/api/JSScript.cpp
  src/d2bs/script/api/JSSocket.cpp
  src/d2bs/script/api/JSSQLite.cpp
  src/d2bs/script/api/JSUnit.cpp
  
  src/d2bs/script/AutoRoot.cpp
  src/d2bs/script/js32.cpp
  src/d2bs/script/Script.cpp
  src/d2bs/script/ScriptEngine.cpp

  src/d2bs/utils/CommandLine.cpp
  src/d2bs/utils/Console.cpp
  src/d2bs/utils/dde.cpp
  src/d2bs/utils/Events.cpp
  src/d2bs/utils/Helpers.cpp
  src/d2bs/utils/Offset.cpp
  src/d2bs/utils/StackWalker.cpp
  src/d2bs/utils/sqlite3.c
  src/d2bs/utils/stringhash.cpp

  src/d2bs/D2BS.cpp)

if(WIN32)
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.rc.in
    ${CMAKE_CURRENT_BINARY_DIR}/version.rc @ONLY)
  list(APPEND D2BS_SRCS ${CMAKE_CURRENT_BINARY_DIR}/version.rc)
endif()

add_library(${PROJECT_NAME} SHARED ${D2BS_SRCS})
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Enable all warnings
string(REGEX REPLACE "/W[0-4] " "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
string(REGEX REPLACE "/W[0-4]$" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
string(REGEX REPLACE "/W[0-4] " "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
string(REGEX REPLACE "/W[0-4]$" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
target_compile_options(${PROJECT_NAME} PRIVATE /W4)
message(STATUS "D2BS: Maximum warning setting configured, here we go!")

target_compile_definitions(${PROJECT_NAME} PRIVATE
  WIN32_LEAN_AND_MEAN

  XP_WIN
  STATIC_JS_API
  D2BS_VERSION="${D2BS_VERSION}")

# Set option definitions
foreach(D2BS_OPTION
  D2BS_ENABLE_PROFILING
  D2BS_NO_EXCEPTIONS)
  if(${D2BS_OPTION})
    target_compile_definitions(d2bs PRIVATE ${SPDLOG_OPTION})
  endif()
endforeach()

target_link_libraries(${PROJECT_NAME} PRIVATE
  dbghelp
  shlwapi

  js)

install(TARGETS d2bs
  RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)

install(FILES
  resources/api.html
  resources/d2bs.ini
  resources/HISTORY.txt
  resources/LICENSE.txt
  resources/version.bmp
  DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
  
add_subdirectory(vendor/mozjs)
