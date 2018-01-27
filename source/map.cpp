#include "map.h"

#include "entity.cpp"

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
            m.tiles[i][j] = pnoise2d(i*0.05, j*0.05, 10, 1, 1234) > 1 - ((r32)j / (MAP_HEIGHT / 2)) ? 1 : 0;
        }
    }

    m.entity_count = 0;
    for(i16 i = 0; i < MAX_ENTITY; i++) {
        m.entities[i].id = -1;
        m.entity_ids[i] = -1;
    }

    return m;
}

void add_entity(Map *m, Entity e) {
    for(i16 i = 0; i < MAX_ENTITY; i++) {
        if(m->entities[i].id < 0) {
            e.id = i;
            m->entities[i] = e;
            m->entity_ids[m->entity_count++] = e.id;
            break;
        }
    }
}

void update_map(Map *m) {
    for(i16 i = 0; i < m->entity_count; i++) {
        update_entity(m, m->entities + m->entity_ids[i]);
    }
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
