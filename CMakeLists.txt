cmake_minimum_required(VERSION 3.23)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_subdirectory(vendor/mozjs)

project(d2bs)

set(D2BS_SRCS
  src/core/ActMap.cpp
  src/core/Control.cpp
  src/core/Core.cpp
  src/core/dde.cpp
  src/core/File.cpp
  src/core/Game.cpp
  src/core/Hash.cpp
  src/core/LevelMap.cpp
  src/core/MPQStats.cpp
  src/core/Profile.cpp
  src/core/Room.cpp
  src/core/ScreenHook.cpp
  src/core/Unit.cpp
  
  src/diablo/handlers/D2Handlers.cpp
  src/diablo/handlers/D2NetHandlers.cpp
  src/diablo/patches/D2Intercepts.cpp
  src/diablo/D2Helpers.cpp

  src/script/api/JSArea.cpp
  src/script/api/JSControl.cpp
  src/script/api/JSCore.cpp
  src/script/api/JSDirectory.cpp
  src/script/api/JSExits.cpp
  src/script/api/JSFile.cpp
  src/script/api/JSFileTools.cpp
  src/script/api/JSGame.cpp
  src/script/api/JSGlobalClasses.cpp
  src/script/api/JSHash.cpp
  src/script/api/JSMenu.cpp
  src/script/api/JSParty.cpp
  src/script/api/JSPresetUnit.cpp
  src/script/api/JSProfile.cpp
  src/script/api/JSRoom.cpp
  src/script/api/JSSandbox.cpp
  src/script/api/JSScreenHook.cpp
  src/script/api/JSScript.cpp
  src/script/api/JSSocket.cpp
  src/script/api/JSSQLite.cpp
  src/script/api/JSUnit.cpp
  
  src/script/engine/AutoRoot.cpp
  src/script/engine/Script.cpp
  src/script/engine/ScriptEngine.cpp

  src/script/js32.cpp

  src/utils/CommandLine.cpp
  src/utils/Console.cpp
  src/utils/Events.cpp
  src/utils/Helpers.cpp
  src/utils/Offset.cpp
  src/utils/StackWalker.cpp
  src/utils/sqlite3.c
  src/utils/stringhash.cpp

  src/D2BS.cpp
  src/version.rc)

add_library(${PROJECT_NAME} SHARED ${D2BS_SRCS})
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

string(REGEX REPLACE "/W[0-4] " "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
string(REGEX REPLACE "/W[0-4]$" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
string(REGEX REPLACE "/W[0-4] " "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
string(REGEX REPLACE "/W[0-4]$" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
target_compile_options(${PROJECT_NAME} PRIVATE /W4)
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
