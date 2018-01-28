#include "entity.h"
#include "light.h"

struct DiggerDrone {
    i8 drill_type,
       armor_type;
};

Entity init_digger_drone(i16 id, r32 x, r32 y, i8 type) {
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
    e.defense = 0.1;
    e.data = malloc(sizeof(DiggerDrone));
    e.digger->drill_type = 0;
    e.digger->armor_type = 0;
    return e;
}

void clean_up_digger_drone(Entity *e) {
    free(e->data);
}

void update_digger_drone(Entity *e, LightState lighting[MAX_EXPLORER]) {
    ExplorerDrone *r = (ExplorerDrone *)e->data;
    switch(r->type) {
        case EXPLORER_VIS: {
            do_light(lighting + r->type, e->x + e->w/2, e->y + e->h/2, 256, 3, 1, 1, 1);
            break;
        }
        default: break;
    }
}

void draw_digger_drone(Entity *e, Camera *c) {
    DiggerDrone *d = (DiggerDrone *)e->data;
    draw_texture_region(&textures[TEX_SPRITES], e->direction * FLIP_HORIZONTAL,
                        explorer_data[r->type].tx, explorer_data[r->type].ty,
                        explorer_data[r->type].w, explorer_data[r->type].h,
                        e->x - 4 - c->x, e->y - 4 - c->y, e->x_vel * 4);
}

