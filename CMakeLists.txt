cmake_minimum_required(VERSION 3.23)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_subdirectory(vendor/mozjs)

project(d2bs)

set(D2BS_SRCS
  src/Map/Diablo_II/ActMap.cpp
  src/Map/Diablo_II/LevelMap.cpp
  src/Map/Diablo_II/Points.cpp

  src/AutoRoot.cpp
  src/CommandLine.cpp
  src/Console.cpp
  src/Control.cpp
  src/Core.cpp
  src/D2BS.cpp
  src/D2Handlers.cpp
  src/D2Helpers.cpp
  src/D2Intercepts.cpp
  src/D2NetHandlers.cpp
  src/dde.cpp
  src/Events.cpp
  src/File.cpp
  src/Game.cpp
  src/Hash.cpp
  src/Helpers.cpp
  src/js32.cpp
  src/JSArea.cpp
  src/JSControl.cpp
  src/JSCore.cpp
  src/JSDirectory.cpp
  src/JSExits.cpp
  src/JSFile.cpp
  src/JSFileTools.cpp
  src/JSGame.cpp
  src/JSGlobalClasses.cpp
  src/JSHash.cpp
  src/JSMenu.cpp
  src/JSParty.cpp
  src/JSPresetUnit.cpp
  src/JSProfile.cpp
  src/JSRoom.cpp
  src/JSSandbox.cpp
  src/JSScreenHook.cpp
  src/JSScript.cpp
  src/JSSocket.cpp
  src/JSSQLite.cpp
  src/JSUnit.cpp
  src/MPQStats.cpp
  src/Offset.cpp
  src/Profile.cpp
  src/Room.cpp
  src/ScreenHook.cpp
  src/Script.cpp
  src/ScriptEngine.cpp
  src/StackWalker.cpp
  src/stringhash.cpp
  src/sqlite3.c
  src/Unit.cpp
  src/version.rc)

add_library(${PROJECT_NAME} SHARED ${D2BS_SRCS})
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

string(REGEX REPLACE "/W[0-4] " "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
string(REGEX REPLACE "/W[0-4]$" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
string(REGEX REPLACE "/W[0-4] " "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
string(REGEX REPLACE "/W[0-4]$" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
target_compile_options(${PROJECT_NAME} PRIVATE /W0)
message(STATUS "D2BS: Maximum warning setting configured, here we go!")

# These are all trash definitions from d2bs solution required to successfully
# compile... at least for now :)
target_compile_definitions(${PROJECT_NAME} PRIVATE
  WIN32_LEAN_AND_MEAN

  XP_WIN
  STATIC_JS_API)

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
