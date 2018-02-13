#include "entity.h"
#include "light.h"

enum {
    DRILL_STEEL,
    DRILL_DIAMOND,
    DRILL_GIGA,
    MAX_DRILL
};

struct {
    i32 wait, yield;
} drill_data[MAX_DRILL] = {
    { 30, 10 },
    { 25, 20 },
    { 20, 25 },
};

struct DiggerDrone {
    i16 target_x, target_y;
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
    e.hurt_cooldown = 0;
    e.defense = armor_data[armor_type].defense;
    e.data = malloc(sizeof(DiggerDrone));
    e.digger->target_x = -1;
    e.digger->target_y = -1;
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

    if((d->target_x < 0 || d->target_y < 0) ||
       !m->tiles[d->target_x][d->target_y]) {
        i8 found_target = 0;

        i16 smallest_x = -1, smallest_y = -1;
        r32 smallest_distance2 = -1;

        for(i16 i = (e->x - 128) / 8; i <= (e->x + 128) / 8; ++i) {
            for(i16 j = (e->y - 128) / 8; j <= (e->y + 128) / 8; ++j) {
                r32 distance2 = distance2_32(i*8 + 4, j*8 + 4, e->x + e->w/2, e->y + e->h/2);
                if(i >= 0 && i < MAP_WIDTH && j >= 0 && j < MAP_HEIGHT &&
                   m->tiles[i][j] >= TILE_COPPER &&
                   (smallest_distance2 < 0 || distance2 < smallest_distance2)) {
                    smallest_x = i;
                    smallest_y = j;
                    smallest_distance2 = distance2;
                    found_target = 1;
                    break;
                }
            }
        }

        d->target_x = smallest_x;
        d->target_y = smallest_y;

        if(!found_target) {
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
    else if(d->target_x >= 0 && d->target_y >= 0) {
        r32 angle = atan2(d->target_y*8 + 4 - e->y - e->h/2, d->target_x*8 + 4 - e->x - e->w/2);
        e->x_vel += (1*cos(angle) - e->x_vel) * 0.1;
        e->y_vel += (1*sin(angle) - e->y_vel) * 0.1;
        if(!--d->dig_timer) {
            d->dig_timer = drill_data[d->drill_type].wait;
            mine(e->x + e->w/2, e->y + e->h/2, 32, m, g, drill_data[d->drill_type].yield);
        }
    }
}

void draw_digger_drone(Entity *e, Camera *c) {
    DiggerDrone *d = (DiggerDrone *)e->data;
    if(d->target_x >= 0 && d->target_y >= 0) {
        r32 angle = atan2(d->target_y*8 + 4 - e->y - e->h/2, d->target_x*8 + 4 - e->x - e->w/2);
        draw_texture_region(&textures[TEX_SPRITES], e->direction * FLIP_HORIZONTAL,
                            180, 0, 20, 20,
                            e->x - 4 - c->x, e->y - 4 - c->y, angle * (180 / PI));
    }
    else {
        draw_texture_region(&textures[TEX_SPRITES], e->direction * FLIP_HORIZONTAL,
                            180, 0, 20, 20,
                            e->x - 4 - c->x, e->y - 4 - c->y, e->x_vel*4);
    }
}

