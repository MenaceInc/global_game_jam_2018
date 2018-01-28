#include "entity.h"
#include "light.h"

enum {
    WEAPON_GUN,
    WEAPON_LASER,
    WEAPON_BHG,
    MAX_WEAPON
};

struct FighterDrone {
    r32 target_x, target_y;
    i8 weapon_type,
       armor_type,
       shoot_timer;
};

Entity init_fighter_drone(i16 id, r32 x, r32 y, i8 armor_type, i8 antenna_type, i8 weapon_type) {
    Entity e;
    e.direction = 0;
    e.id = id;
    e.type = ENTITY_FIGHTER_DRONE;
    e.x = x;
    e.y = y;
    e.w = 12;
    e.h = 12;
    e.x_vel = 0;
    e.y_vel = 0;
    e.health = 1;
    e.defense = armor_data[armor_type].defense;
    e.data = malloc(sizeof(FighterDrone));
    e.fighter->target_x = x;
    e.fighter->target_y = y;
    e.fighter->armor_type = armor_type;
    e.fighter->weapon_type = 0;
    return e;
}

void clean_up_fighter_drone(Entity *e) {
    free(e->data);
}

void update_fighter_drone(Entity *e, LightState lighting[MAX_EXPLORER]) {
    do_light(lighting + EXPLORER_VIS, e->x + e->w/2, e->y + e->h/2, 256, 4, 1, 1, 1);


}

void draw_fighter_drone(Entity *e, Camera *c) {
    FighterDrone *f = (FighterDrone *)e->data;
    r32 angle = atan2(f->target_y - e->y, f->target_x - e->x);
    draw_texture_region(&textures[TEX_SPRITES], e->direction * FLIP_HORIZONTAL,
                        240, 0, 20, 20,
                        e->x - 4 - c->x, e->y - 4 - c->y, angle * (180 / PI));
}

