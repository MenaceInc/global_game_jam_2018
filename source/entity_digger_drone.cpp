#include "entity.h"
#include "light.h"

struct DiggerDrone {
    i8 drill_type,
       armor_type;
};

Entity init_digger_drone(i16 id, r32 x, r32 y, i8 drill_type, i8 armor_type) {
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
    e.digger->drill_type = drill_type;
    e.digger->armor_type = armor_type;
    return e;
}

void clean_up_digger_drone(Entity *e) {
    free(e->data);
}

void update_digger_drone(Entity *e, Map *m, GameState *g, LightState lighting[MAX_EXPLORER]) {
    DiggerDrone *d = (DiggerDrone *)e->data;
    do_light(lighting + EXPLORER_VIS, e->x + e->w/2, e->y + e->h/2, 256, 4, 1, 1, 1);
}

void draw_digger_drone(Entity *e, Camera *c) {
    DiggerDrone *d = (DiggerDrone *)e->data;
    draw_texture_region(&textures[TEX_SPRITES], e->direction * FLIP_HORIZONTAL,
                        180, 0, 20, 20,
                        e->x - 4 - c->x, e->y - 4 - c->y, e->x_vel * 4);
}

