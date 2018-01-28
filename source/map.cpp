#include "map.h"

#include "entity.cpp"

extern "C" {
#include "ext/perlin.c"
}

enum {
    TILE_NONE,
    TILE_DIRT,
    TILE_ROCK,
    TILE_MAGMA,

    TILE_URANIUM,
    TILE_COPPER,
    TILE_DIAMOND,
    TILE_GOLD,

    MAX_TILE
};

struct {
    i16 tx, ty;
} tile_data[MAX_TILE] = {
    { 0, 0 },
    { 8, 32 },
    { 8, 48 },
    { 8, 80 },

    { 32, 48 },
    { 40, 64 },
    { 32, 56 },
    { 40, 56 },
};

Map generate_map() {
    Map m;

    foreach(i, MAP_WIDTH) {
        foreach(j, MAP_HEIGHT) {
            r32 noise = pnoise2d(i*0.05, j*0.05, 10, 1, 1234);
            if(noise > (r32)j / MAP_HEIGHT) {
                m.tiles[i][j] = TILE_DIRT;
            }
            else if(noise > 1 - ((r32)j / MAP_HEIGHT)) {
                m.tiles[i][j] = TILE_MAGMA;
            }
            else if(noise > 0.2) {
                m.tiles[i][j] = TILE_ROCK;
            }
            else {
                m.tiles[i][j] = 0;
            }
        }
    }

    m.entity_count = 0;
    for(i16 i = 0; i < MAX_ENTITY; i++) {
        m.entities[i].id = -1;
        m.entity_ids[i] = -1;
    }

    return m;
}

i16 add_entity(Map *m, Entity e) {
    for(i16 i = 0; i < MAX_ENTITY; i++) {
        if(m->entities[i].id < 0) {
            e.id = i;
            m->entities[i] = e;
            m->entity_ids[m->entity_count++] = e.id;
            return i;
        }
    }
    return -1;
}

void delete_entity(Map *m, i16 id) {
    for(i16 i = 0; i < m->entity_count; i++) {
        if(m->entity_ids[i] == id) {
            memmove(m->entity_ids + i, m->entity_ids + i + 1, (m->entity_count - i - 1)*sizeof(i16));
            break;
        }
    }
    clean_up_entity(m->entities+id);
    --m->entity_count;
}

void draw_map(Map *m, Camera *c, r32 bound_x, r32 bound_y) {
    for(i16 i = (i16)c->x/8; i < (i16)c->x/8 + bound_x/8 + 1; i++) {
        for(i16 j = (i16)c->y/8; j < (i16)c->y/8 + bound_y/8 + 1; j++) {
            if(i >= 0 && i < MAP_WIDTH && j >= 0 && j < MAP_HEIGHT && m->tiles[i][j]) {
                draw_texture_region(&textures[TEX_SPRITES], 0,
                                    tile_data[m->tiles[i][j]].tx, tile_data[m->tiles[i][j]].ty, 8, 8,
                                    i*8 - (i32)c->x, j*8 - (i32)c->y, 0);
            }
        }
    }

    for(i16 i = 0; i < m->entity_count; i++) {
        draw_entity(m->entities[m->entity_ids[i]], c);
    }
}
