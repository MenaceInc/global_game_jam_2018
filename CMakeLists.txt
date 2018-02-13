cmake_minimum_required(VERSION 3.9)
project(global_game_jam_2018)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -msse4.1")

include_directories(src lib)
link_directories(lib/soil)

# Set output folder to bin folder
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE})

add_executable(global_game_jam_2018
        src/main.cpp)
target_link_libraries(global_game_jam_2018 glfw3 glew32 soil opengl32 gdi32 openal )

# Copy resources folder to bin folder
add_custom_command(TARGET global_game_jam_2018 POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/resources ${EXECUTABLE_OUTPUT_PATH}/resources)
