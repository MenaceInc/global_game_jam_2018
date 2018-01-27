#include "entity.h"

Entity init_rocket_drone(i16 id, r32 x, r32 y) {
    Entity e;
    e.id = id;
    e.type = ENTITY_ROCKET_DRONE;
    e.x = x;
    e.y = y;
    e.w = 12;
    e.h = 12;
    e.x_vel = 0;
    e.y_vel = 0;
    e.data = malloc(sizeof(RocketDrone));
    return e;
}

void clean_up_rocket_drone(Entity *e) {
    free(e->data);
}

void update_rocket_drone(Entity *e) {
    RocketDrone *r = (RocketDrone *)e->data;
}

void draw_rocket_drone(Entity *e, Camera *c) {
    RocketDrone *r = (RocketDrone *)e->data;
    draw_texture_region(&textures[TEX_SPRITES], 0, 20, 0, 20, 20, e->x - 4 - c->x, e->y - 4 - c->y, 0);
}
