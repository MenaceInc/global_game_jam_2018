#ifndef MAP_H
#define MAP_H

#include "entity.h"

#define MAX_ENTITY 1024
#define MAP_WIDTH 256
#define MAP_HEIGHT 1024

struct Map {
    i8 tiles[MAP_WIDTH][MAP_HEIGHT];
    InstancedList tiles_il;

    Entity entities[MAX_ENTITY];
    i16 entity_ids[MAX_ENTITY],
        entity_count;
};

#endif
