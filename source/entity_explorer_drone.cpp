#include "entity.h"
#include "light.h"

enum {
    EXPLORER_VIS,
    EXPLORER_IR,
    EXPLORER_EM,
    MAX_EXPLORER
};

struct ExplorerDrone {
    i8 type;
};

global struct {
    i16 tx, ty, w, h;
    const char *name;
} explorer_data[MAX_EXPLORER] = {
    { 20, 0, 20, 20, "Light Explorer" },
    { 140, 0, 20, 20, "Infrared Explorer" },
    { 80, 0, 20, 20, "Electromagnetic Explorer" },
};

Entity init_explorer_drone(i16 id, r32 x, r32 y, i8 type) {
    Entity e;
    e.direction = 0;
    e.id = id;
    e.type = ENTITY_EXPLORER_DRONE;
    e.x = x;
    e.y = y;
    e.w = 12;
    e.h = 12;
    e.x_vel = 0;
    e.y_vel = 0;
    e.health = 1;
    e.defense = 0.1;
    e.data = malloc(sizeof(ExplorerDrone));
    e.explorer->type = type;
    return e;
}

void clean_up_explorer_drone(Entity *e) {
    free(e->data);
}

void update_explorer_drone(Entity *e, LightState lighting[MAX_EXPLORER]) {
    ExplorerDrone *r = (ExplorerDrone *)e->data;
    switch(r->type) {
        case EXPLORER_VIS: {
            do_light(lighting + r->type, e->x + e->w/2, e->y + e->h/2, 256, 3, 1, 1, 1);
            break;
        }
        default: break;
    }
}

void draw_explorer_drone(Entity *e, Camera *c) {
    ExplorerDrone *r = (ExplorerDrone *)e->data;
    draw_texture_region(&textures[TEX_SPRITES], e->direction * FLIP_HORIZONTAL,
                        explorer_data[r->type].tx, explorer_data[r->type].ty,
                        explorer_data[r->type].w, explorer_data[r->type].h,
                        e->x + e->w/2 - 10 - c->x, e->y + e->h/2 - 10 - c->y, e->x_vel * 4);
}
