cmake_minimum_required(VERSION 3.23)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_subdirectory(vendor/mozjs)

project(d2bs)

set(D2BS_SRCS
  Map/Diablo_II/ActMap.cpp
  Map/Diablo_II/LevelMap.cpp
  Map/Diablo_II/Points.cpp

  AutoRoot.cpp
  CommandLine.cpp
  Console.cpp
  Control.cpp
  Core.cpp
  D2BS.cpp
  D2Handlers.cpp
  D2Helpers.cpp
  D2Intercepts.cpp
  D2NetHandlers.cpp
  dde.cpp
  Events.cpp
  File.cpp
  Game.cpp
  Hash.cpp
  Helpers.cpp
  js32.cpp
  JSArea.cpp
  JSControl.cpp
  JSCore.cpp
  JSDirectory.cpp
  JSExits.cpp
  JSFile.cpp
  JSFileTools.cpp
  JSGame.cpp
  JSGlobalClasses.cpp
  JSHash.cpp
  JSMenu.cpp
  JSParty.cpp
  JSPresetUnit.cpp
  JSProfile.cpp
  JSRoom.cpp
  JSSandbox.cpp
  JSScreenHook.cpp
  JSScript.cpp
  JSSocket.cpp
  JSSQLite.cpp
  JSUnit.cpp
  MPQStats.cpp
  Offset.cpp
  Profile.cpp
  Room.cpp
  ScreenHook.cpp
  Script.cpp
  ScriptEngine.cpp
  StackWalker.cpp
  stringhash.cpp
  sqlite3.c
  Unit.cpp)

add_library(${PROJECT_NAME} SHARED ${D2BS_SRCS})
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

string(REGEX REPLACE "/W[0-4] " "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
string(REGEX REPLACE "/W[0-4]$" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
string(REGEX REPLACE "/W[0-4] " "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
string(REGEX REPLACE "/W[0-4]$" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
target_compile_options(${PROJECT_NAME} PRIVATE /W0)
message(STATUS "D2BS: Completely ignore warnings!!!!!! Remove later...")

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