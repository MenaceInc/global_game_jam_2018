#include "map.h"

extern "C" {
#include "ext/perlin.c"
}

enum {
    TILE_NONE,
    TILE_ROCK,
    MAX_TILE
};

struct {
    i16 tx, ty;
} tile_data[MAX_TILE] = {
    { 0, 0 },
    { 0, 20 },
};

Map generate_map() {
    Map m;

    foreach(i, MAP_WIDTH) {
        foreach(j, MAP_HEIGHT) {
            m.tiles[i][j] = pnoise2d(i*0.3, j*0.3, 0.2, 1, 1234) > 1 - ((r32)j / (MAP_HEIGHT / 2)) ? 1 : 0;
        }
    }

    return m;
}
