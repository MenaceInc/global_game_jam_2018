cmake_minimum_required(VERSION 3.9)
project(global_game_jam_2018)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -msse4.1")

include_directories(src)

# Set output folder to bin folder
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE})

add_executable(global_game_jam_2018
        src/ext/perlin.c
        src/ext/stb_vorbis.c
        src/audio.cpp
        src/camera.cpp
        src/draw.cpp
        src/entity.cpp
        src/entity_brain_alien.cpp
        src/entity_digger_drone.cpp
        src/entity_explorer_drone.cpp
        src/entity_fighter_drone.cpp
        src/globals.cpp
        src/light.cpp
        src/main.cpp
        src/map.cpp
        src/particle.cpp
        src/player_controller.cpp
        src/resources.cpp
        src/state.cpp
        src/state_game.cpp
        src/state_splash.cpp
        src/state_title.cpp
        src/ui.cpp)

# Copy ID1 folder to bin folder
add_custom_command(TARGET global_game_jam_2018 POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/resources ${EXECUTABLE_OUTPUT_PATH}/resources)
