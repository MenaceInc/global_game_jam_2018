#include "map.h"

enum {
    TILE_NONE,
    TILE_ROCK,
    MAX_TILE
};

struct {
    i16 tx, ty;
} tile_data[MAX_TILE] = {
    { 0, 0 },
    { 0, 0 },
};

Map generate_map() {
    Map m;

    foreach(i, MAP_W) {
        foreach(j, MAP_H) {
            m.tiles[i][j] = 1;
        }
    }

    return m;
}
