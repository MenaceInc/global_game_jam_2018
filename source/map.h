#ifndef MAP_H
#define MAP_H

#include "entity.h"

#define MAX_ENTITY 256
#define MAP_WIDTH 512
#define MAP_HEIGHT 1536

struct Map {
    i8 tiles[MAP_WIDTH][MAP_HEIGHT];

    Entity entities[MAX_ENTITY];
    i16 entity_ids[MAX_ENTITY],
        entity_count;
};

#endif
