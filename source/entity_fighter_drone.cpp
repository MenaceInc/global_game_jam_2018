#include "entity.h"
#include "light.h"

enum {
    WEAPON_GUN,
    WEAPON_LASER,
    WEAPON_BHG,
    MAX_WEAPON
};

struct {
    r32 strength;
    i32 max_shots,
        max_tile_shots;
} weapon_data[MAX_WEAPON] = {
    { 0.2, 3, 1 },
    { 0.4, 6, 5 },
    { 0.8, 15, 10 }
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
    e.hurt_cooldown = 0;
    e.defense = armor_data[armor_type].defense;
    e.data = malloc(sizeof(FighterDrone));
    e.fighter->target_x = x;
    e.fighter->target_y = y;
    e.fighter->armor_type = armor_type;
    e.fighter->weapon_type = 0;
    e.fighter->shoot_timer = 8;
    return e;
}

void clean_up_fighter_drone(Entity *e) {
    free(e->data);
}

void update_fighter_drone(Entity *e, Map *m, Projectile **projectiles, LightState lighting[MAX_EXPLORER]) {
    do_light(lighting + EXPLORER_VIS, e->x + e->w/2, e->y + e->h/2, 256, 4, 1, 1, 1);

    if(!--e->fighter->shoot_timer) {
        i32 shots_fired = 0;
        for(i16 i = 0; i < m->entity_count && shots_fired < weapon_data[e->fighter->weapon_type].max_shots; i++) {
            Entity *ent = m->entities + m->entity_ids[i];
            if(ent->type == ENTITY_BRAIN_ALIEN && distance2_32(ent->x, ent->y, e->x, e->y) < 256*256) {
                shoot(e->id, e->x + e->w/2, e->y + e->h/2, ent->x + ent->w/2, ent->y + ent->h/2, weapon_data[e->fighter->weapon_type].strength,
                      weapon_data[e->fighter->weapon_type].max_tile_shots,
                      projectiles);
                ++shots_fired;
            }
        }
        e->fighter->shoot_timer = random(8, 16);
    }
}

void draw_fighter_drone(Entity *e, Camera *c) {
    FighterDrone *f = (FighterDrone *)e->data;
    r32 angle = atan2(f->target_y - e->y, f->target_x - e->x);
    draw_texture_region(&textures[TEX_SPRITES], e->direction * FLIP_HORIZONTAL,
                        240, 0, 20, 20,
                        e->x - 4 - c->x, e->y - 4 - c->y, angle * (180 / PI));
}

