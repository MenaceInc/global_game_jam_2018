# global_game_jam_2018

This is the source code for Lunar Drone which was written for the 2018 Global Game Jam. It's definitely imperfect code (we only had 48 hours, so we had to rush a bit), but it runs well. 

The source code is entirely C-style C++ with OpenGL rendering. The game uses a few externally compiled libraries: GLFW, OpenAL Soft, and SOIL. All other external software is compiled with the project; it can be found in the source/ext folder.

There is only one compilation unit when compiling the game (unity build); eventually, all things are included into source/main.cpp and then only that unit is compiled.

All of the resources/DLLs necessary for running the game can be found in this repository, so the game is ready to run (on Windows) once compiled.
