cmake_minimum_required(VERSION 3.16..4.18)
project(papaya LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# ============================================================================
# PLIKI èR”D£OWE (.cpp)
# ============================================================================
set(PAPAYA_SOURCES
  src/papaya.cpp
  
  src/Map/map.cpp
  
  src/Entities/entities.cpp

  src/Saves/saves.cpp

  src/Core/GameWrapper.cpp

  # Audio
  src/Audio/AudioManager.cpp
  
  # Scenes
  src/Scenes/scene_game.cpp
  
  # Entities - Player
  src/Entities/Player/Player.cpp
  
  # External
  src/external_headers/tinyfiledialogs.cpp
)

# ============================================================================
# NAG£”WKI (dla IDE - opcjonalne ale przydatne)
# ============================================================================
set(PAPAYA_HEADERS
  # Core utilities
  src/Core/InputHelper.h
  src/Core/MathUtils.h
  src/Core/GameWrapper.hpp
  src/Core/Physics.hpp
  src/Core/GameConstants.hpp
  
  # Audio
  src/Audio/AudioManager.hpp

  # Entities - Base
  src/Entities/Entity.h
  
  # Entities - Player
  src/Entities/Player/Player.h
  src/Entities/Player/PlayerConstants.h
  src/Entities/Player/PlayerTypes.h


  # Entities - Environment
  src/Entities/Environment/Wall.h
  
  # Entities - Enemies
  src/Entities/Enemies/rabbit.h
  src/Entities/Enemies/mage_boss.h
  
  # GUI
  src/GUI/GUI_raylib.hpp
  
  # Scenes
  src/Scenes/main_menu.hpp
  src/Scenes/scene_functions.hpp
  
  src/Scenes/scene_game.hpp
  
  # Saves
  src/Entities/entities.hpp 
  src/Map/map.hpp
  src/Saves/saves.hpp
)

add_executable(papaya ${PAPAYA_SOURCES} ${PAPAYA_HEADERS})

# ============================================================================
# INCLUDE DIRECTORIES
# ============================================================================
target_include_directories(papaya PRIVATE 
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/src/external_headers
)

# ============================================================================
# DEPENDENCIES
# ============================================================================

# nlohmann/json
include(FetchContent)
FetchContent_Declare(json 
  URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(json)
target_link_libraries(papaya PRIVATE nlohmann_json::nlohmann_json)

# raylib
find_package(raylib REQUIRED)

if (TARGET raylib)
  target_link_libraries(papaya PRIVATE raylib)
elseif (TARGET raylib::raylib)
  target_link_libraries(papaya PRIVATE raylib::raylib)
elseif (DEFINED RAYLIB_LIBRARIES)
  target_include_directories(papaya PRIVATE ${RAYLIB_INCLUDE_DIRS})
  target_link_libraries(papaya PRIVATE ${RAYLIB_LIBRARIES})
else()
  message(FATAL_ERROR "Found raylib but could not determine target to link.")
endif()

# ============================================================================
# COMPILER OPTIONS
# ============================================================================
if (MSVC)
  target_compile_options(papaya PRIVATE /W4)
else()
  target_compile_options(papaya PRIVATE -Wall -Wextra -Wpedantic)
endif()

# ============================================================================
# OUTPUT
# ============================================================================
set_target_properties(papaya PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Copy assets
add_custom_command(TARGET papaya POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/assets
    ${CMAKE_BINARY_DIR}/bin/assets
)