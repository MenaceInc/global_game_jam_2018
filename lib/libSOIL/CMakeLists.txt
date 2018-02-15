cmake_minimum_required(VERSION 3.5)
project(SOIL)

set(LIB_FILES SOIL/image_DXT.c SOIL/image_helper.c SOIL/SOIL.c SOIL/stb_image_aug.c)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Define the MinWinPP library
add_library(soil STATIC ${LIB_FILES})

# Set the current directory as an include directory
target_include_directories(soil PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
