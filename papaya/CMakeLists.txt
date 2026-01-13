cmake_minimum_required(VERSION 3.16..4.18)
project(papaya LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Default to Debug build to include symbols for easier debugging if not set
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# sources (add or remove files as appropriate)
set(PAPAYA_SOURCES
  src/papaya.cpp
  src/Saves/map.cpp
  src/Entities/Entity.h        # optional for IDE visibility
  src/Entities/Dummy.h
  src/Entities/Enemy.h
  src/Entities/Player.h
  src/Entities/Wall.h
  src/GUI/GUI_raylib.hpp
  src/Scenes/main_menu.hpp
  src/Scenes/scene_functions.hpp
  src/Scenes/scene_test_room.h
  src/Saves/entities.hpp 
  src/Saves/map.hpp
  src/Saves/saves.hpp
  # add other .cpp/.h here
)

# Prefer listing .cpp files explicitly; adjust to match your tree:
set(PAPAYA_CPP
  src/papaya.cpp
  src/Saves/map.cpp
  src/Saves/saves.cpp
  src/Saves/entities.cpp
  src/external_headers/tinyfiledialogs.cpp
  # add other .cpp files
)

add_executable(papaya ${PAPAYA_CPP} ${PAPAYA_SOURCES})

# Include directories so headers are found with #include "..."
target_include_directories(papaya PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(papaya PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/external_headers)

include(FetchContent)

FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz)
FetchContent_MakeAvailable(json)

target_link_libraries(papaya PRIVATE nlohmann_json::nlohmann_json)

# Note that the namespaced target will always be available regardless of the
# import method
target_link_libraries(papaya PRIVATE nlohmann_json::nlohmann_json)

# Example: enable more warnings (optional)
if (MSVC)
  target_compile_options(papaya PRIVATE /W4)
else()
  target_compile_options(papaya PRIVATE -Wall -Wextra -Wpedantic)
endif()

# If you use external libs, find and link them:
find_package(raylib REQUIRED)

# Link against raylib. Different raylib CMake packages export different targets/variables
if (TARGET raylib)
  target_link_libraries(papaya PRIVATE raylib)
elseif (TARGET raylib::raylib)
  target_link_libraries(papaya PRIVATE raylib::raylib)
elseif (DEFINED RAYLIB_LIBRARIES)
  target_include_directories(papaya PRIVATE ${RAYLIB_INCLUDE_DIRS})
  target_link_libraries(papaya PRIVATE ${RAYLIB_LIBRARIES})
else()
  message(FATAL_ERROR "Found raylib but could not determine target to link. Please set RAYLIB_LIBRARIES or ensure raylib provides a CMake target (raylib or raylib::raylib).")
endif()

# Put binaries in build/bin
set_target_properties(papaya PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Copy assets to the runtime output directory so code using "Assets/..." finds them
add_custom_command(TARGET papaya POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/assets
    ${CMAKE_BINARY_DIR}/bin/assets
)
