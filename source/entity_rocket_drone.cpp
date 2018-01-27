#include "entity.h"

Entity init_rocket_drone(i16 id, r32 x, r32 y) {
    Entity e;
    e.id = id;
    e.type = ENTITY_ROCKET_DRONE;
    e.x = x;
    e.y = y;
    e.w = 72;
    e.h = 72;
    e.x_vel = 0;
    e.y_vel = 0;
    e.data = malloc(sizeof(RocketDrone));
}

void update_rocket_drone(Entity *e) {
    RocketDrone *r = (RocketDrone *)e->data;
}

void draw_rocket_drone(Entity *e) {
    RocketDrone *r = (RocketDrone *)e->data;

}
