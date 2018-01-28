#include "entity.h"
#include "light.h"

enum {
    DRILL_STEEL,
    DRILL_DIAMOND,
    DRILL_GIGA,
    MAX_DRILL
};

struct DiggerDrone {
    r32 target_x, target_y;
    i8 drill_type,
       armor_type,
       dig_timer;
};

Entity init_digger_drone(i16 id, r32 x, r32 y, i8 armor_type, i8 antenna_type, i8 drill_type) {
    Entity e;
    e.direction = 0;
    e.id = id;
    e.type = ENTITY_DIGGER_DRONE;
    e.x = x;
    e.y = y;
    e.w = 12;
    e.h = 12;
    e.x_vel = 0;
    e.y_vel = 0;
    e.health = 1;
    e.defense = armor_data[armor_type].defense;
    e.data = malloc(sizeof(DiggerDrone));
    e.digger->target_x = x;
    e.digger->target_y = y;
    e.digger->drill_type = drill_type;
    e.digger->armor_type = armor_type;
    e.digger->dig_timer = 30;
    return e;
}

void clean_up_digger_drone(Entity *e) {
    free(e->data);
}

void update_digger_drone(Entity *e, Map *m, GameState *g, LightState lighting[MAX_EXPLORER]) {
    DiggerDrone *d = (DiggerDrone *)e->data;
    do_light(lighting + EXPLORER_VIS, e->x + e->w/2, e->y + e->h/2, 256, 4, 1, 1, 1);

    if(fabs(e->x - d->target_x) < 2 &&
       fabs(e->y - d->target_y) < 2) {
        i8 found_target = 0;
        for(i16 i = (e->x - 128) / 8; i <= (e->x + 128) / 8; ++i) {
            for(i16 j = (e->y - 128) / 8; j <= (e->y + 128) / 8; ++j) {
                if(i >= 0 && i < MAP_WIDTH && j >= 0 && j < MAP_HEIGHT &&
                   m->tiles[i][j] >= TILE_COPPER) {
                    d->target_x = i*8;
                    d->target_y = j*8;
                    found_target = 1;
                    break;
                }
            }
        }

        if(!found_target) {
            d->target_x = e->x;
            d->target_y = e->y;
            for(i16 i = 0; i < m->entity_count; i++) {
                Entity *ent = &m->entities[m->entity_ids[i]];
                if(ent->id >= 0 && ent->type == ENTITY_EXPLORER_DRONE && distance2_32(e->x, e->y, ent->x, ent->y) < 128*128) {
                    r32 angle = atan2(ent->y - e->y, ent->x - e->x);
                    e->x_vel += (1*cos(angle) - e->x_vel) * 0.1;
                    e->y_vel += (1*sin(angle) - e->y_vel) * 0.1;
                    break;
                }
            }
        }
    }
    else {
        r32 angle = atan2(d->target_y - e->y, d->target_x - e->x);
        e->x_vel += (1*cos(angle) - e->x_vel) * 0.1;
        e->y_vel += (1*sin(angle) - e->y_vel) * 0.1;
        if(!--d->dig_timer) {
            d->dig_timer = 30;
            mine(e->x + e->w/2, e->y + e->h/2, 32, m, g);
        }
    }
}

void draw_digger_drone(Entity *e, Camera *c) {
    DiggerDrone *d = (DiggerDrone *)e->data;
    r32 angle = atan2(d->target_y - e->y, d->target_x - e->x);
    draw_texture_region(&textures[TEX_SPRITES], e->direction * FLIP_HORIZONTAL,
                        180, 0, 20, 20,
                        e->x - 4 - c->x, e->y - 4 - c->y, angle * (180 / PI));
}

