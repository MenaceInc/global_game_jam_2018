CC := g++
CC_FLAGS := -Wall -g -msse4.1 -std=c++11
LIB := -lSOIL -lglfw3 -lglew32 -lopengl32 -lgdi32 -lopenal32

INC_DIR := ./source
SRC_DIR := ./source

EXEC := game_jam

make:
	$(CC) $(CC_FLAGS) $(SRC_DIR)/main.cpp $(LIB) -I $(INC_DIR) -o $(EXEC)
