cmake_minimum_required(VERSION 3.9)
project(global_game_jam_2018)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -msse4.1 -pthread")

add_subdirectory(lib/libSOIL/)

find_package(PkgConfig REQUIRED)
pkg_search_module(GLFW REQUIRED glfw3)
pkg_search_module(GLEW REQUIRED glew)
pkg_search_module(OPENAL REQUIRED openal)

include_directories(${GLFW_INCLUDE_DIRS} ${GLEW_INCLUDE_DIRS} ${OPENAL_INCLUDE_DIRS})
set(LIBS ${GLFW_LIBRARIES} ${GLEW_LIBRARIES} ${OPENAL_LIBRARIES} soil)

# Set output folder to bin folder
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE})

add_executable(global_game_jam_2018
        src/main.cpp)
target_link_libraries(global_game_jam_2018 ${LIBS})

# Copy resources folder to bin folder
add_custom_command(TARGET global_game_jam_2018 POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/resources ${EXECUTABLE_OUTPUT_PATH}/resources)
