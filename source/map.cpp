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

    memset(m.tiles, 0, MAP_W*MAP_H*2);

    return m;
}
