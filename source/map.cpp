#include "map.h"

extern "C" {
#include "ext/perlin.c"
}

enum {
    TILE_NONE,
    TILE_DIRT,
    TILE_ROCK,
    TILE_MAGMA,

    TILE_COPPER,
    TILE_STEEL,
    TILE_GOLD,
    TILE_PALLADIUM,
    TILE_SILICON,
    TILE_SULFUR,
    TILE_DIAMOND,
    TILE_ENERGINIUM,
    TILE_UNOBTAINIUM,

    MAX_TILE
};

struct {
    i16 tx, ty;
    i8 retrieved_material;
} tile_data[MAX_TILE] = {
    { 0, 0,  -1 },
    { 8, 32, -1 },
    { 8, 48, -1 },
    { 8, 80, -1 },

    { 24, 48, MATERIAL_COPPER },
    { 32, 48, MATERIAL_STEEL },
    { 40, 48, MATERIAL_GOLD },
    { 24, 56, MATERIAL_PALLADIUM },
    { 32, 56, MATERIAL_SILICON },
    { 40, 56, MATERIAL_SULFUR },
    { 24, 64, MATERIAL_DIAMOND },
    { 32, 64, MATERIAL_ENERGINIUM },
    { 40, 64, MATERIAL_UNOBTAINIUM },
};

#include "entity.cpp"

Map generate_map() {
    Map m;

    i32 seed = random(1000, 10000);

    foreach(i, MAP_WIDTH) {
        foreach(j, MAP_HEIGHT) {
            m.tiles[i][j] = 0;
            r32 noise = pnoise2d(i*0.05, j*0.05, 10, 1, seed);
            if(noise > (r32)j / MAP_HEIGHT) {
                m.tiles[i][j] = TILE_DIRT;
            }
            else if(noise > 1 - ((r32)j / MAP_HEIGHT)) {
                m.tiles[i][j] = TILE_MAGMA;
            }
            else if(noise > 0.125) {
                r32 ore_gen = random(0, 1000);
                if(ore_gen < 500) {
                    m.tiles[i][j] = TILE_ROCK;
                }
                else if(ore_gen < 570) {
                    m.tiles[i][j] = TILE_COPPER;
                }
                else if(ore_gen < 600) {
                    m.tiles[i][j] = TILE_STEEL;
                }
                else if(ore_gen < 620) {
                    m.tiles[i][j] = TILE_GOLD;
                }
                else if(ore_gen < 640 && j >= MAP_HEIGHT/2) {
                    m.tiles[i][j] = TILE_PALLADIUM;
                }
                else if(ore_gen < 660 && j >= MAP_HEIGHT/2) {
                    m.tiles[i][j] = TILE_SILICON;
                }
                else if(ore_gen < 700 && j >= MAP_HEIGHT/2) {
                    m.tiles[i][j] = TILE_SULFUR;
                }
                else if(ore_gen < 710 && j >= MAP_HEIGHT * 0.6) {
                    m.tiles[i][j] = TILE_DIAMOND;
                }
                else if(ore_gen < 750 && j >= MAP_HEIGHT * 0.8) {
                    m.tiles[i][j] = TILE_ENERGINIUM;
                }
                else if(ore_gen < 800 && j >= MAP_HEIGHT * 0.8) {
                    m.tiles[i][j] = TILE_UNOBTAINIUM;
                }
                else {
                    m.tiles[i][j] = TILE_ROCK;
                }
            }
            else {
                m.tiles[i][j] = 0;
            }
        }
    }

    m.entity_count = 0;
    for(i16 i = 0; i < MAX_ENTITY; i++) {
        m.entities[i].id = -1;
        m.entities[i].type = -1;
        m.entity_ids[i] = -1;
    }

    return m;
}

void clean_up_map(Map *m) {
    for(i16 i = 0; i < MAX_ENTITY; i++) {
        if(m->entities[i].type >= 0 && m->entities[i].id >= 0) {
            clean_up_entity(m->entities + i);
        }
    }
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
        tint = HMM_Vec4(1,
                        1-m->entities[m->entity_ids[i]].hurt_cooldown,
                        1-m->entities[m->entity_ids[i]].hurt_cooldown,
                        1);

        draw_entity(m->entities[m->entity_ids[i]], c);
    }
    tint = HMM_Vec4(1, 1, 1, 1);
}
