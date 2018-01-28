#include "entity.h"

enum {
    DRONE_EXPLORER,
    DRONE_DIGGER,
    DRONE_FIGHTER,
    MAX_DRONE
};

struct {
    const char *name;
} drone_data[MAX_DRONE] = {
    { "Explorer" },
    { "Digger" },
    { "Fighter" },
};

enum {
    ARMOR_STEEL,
    ARMOR_REACTIVE,
    ARMOR_ENERGY,
    MAX_ARMOR
};

struct {
    const char *name;
    r32 defense;
} armor_data[MAX_ARMOR] = {
    { "Steel Armor", 0.4 },
    { "Reactive Armor", 0.6 },
    { "Energinium Armor", 0.9 },
};

enum {
    ANTENNA_STANDARD,
    ANTENNA_HIGH_BAND,
    ANTENNA_LOW_BAND,
    MAX_ANTENNA
};

struct {
    const char *name;
    r32 range;
} antenna_data[MAX_ANTENNA] = {
    { "Standard Antenna", 2000 },
    { "High-Band Antenna", 5000 },
    { "Low-Band Antenna", 20000 }
};

#include "entity_explorer_drone.cpp"
#include "entity_digger_drone.cpp"
#include "entity_fighter_drone.cpp"
#include "entity_brain_alien.cpp"

#define draw_entity(e, c) {\
            switch(e.type) {\
                case ENTITY_EXPLORER_DRONE: { draw_explorer_drone(&e, c); break; }\
                case ENTITY_DIGGER_DRONE:   { draw_digger_drone(&e, c); break; }\
                case ENTITY_FIGHTER_DRONE:  { draw_fighter_drone(&e, c); break; }\
                case ENTITY_BRAIN_ALIEN:    { draw_brain_alien(&e, c); break; }\
                default: break;\
            }\
        }

void clean_up_entity(Entity *e) {
    switch(e->type) {
        case ENTITY_EXPLORER_DRONE: {
            clean_up_explorer_drone(e);
            break;
        }
        case ENTITY_DIGGER_DRONE: {
            clean_up_digger_drone(e);
            break;
        }
        case ENTITY_FIGHTER_DRONE: {
            clean_up_fighter_drone(e);
            break;
        }
        case ENTITY_BRAIN_ALIEN: {
            clean_up_brain_alien(e);
            break;
        }
        default: break;
    }
    e->type = -1;
    e->id = -1;
    e->data = NULL;
}

void hurt_entity(Entity *e, r32 amount) {
    play_sound_at_point(&sounds[SOUND_HURT], e->x + e->w/2, e->y + e->h/2, amount * 2, random(0.8, 1.2), 0, AUDIO_ENTITY);
    e->health -= amount*(1-e->defense);
    e->hurt_cooldown = 1;
}

void update_entity(Map *m, GameState *game_state, Projectile **projectiles, Entity *e, LightState lighting[MAX_EXPLORER]) {
    r32 new_x = e->x+e->x_vel+e->w/2,
        new_y = e->y+e->y_vel+e->h/2;

    e->x_vel *= 0.99;
    e->y_vel += 0.01;
    e->hurt_cooldown *= 0.99;

    for(i16 i = (new_x - e->w/2 - 8)/8; i <= (new_x + e->w/2 + 8)/8; i++) {
        for(i16 j = (new_y - e->h/2 - 8)/8; j <= (new_y + e->h/2 + 8)/8; j++) {
            if(i >= 0 && i < MAP_WIDTH && j >= 0 && j < MAP_HEIGHT) {
                if(m->tiles[i][j] &&
                   new_x + e->w/2 >= i*8 && new_x - e->w/2 <= i*8+8 &&
                   new_y + e->h/2 >= j*8 && new_y - e->h/2 <= j*8+8) {
                    r32 x_overlap = 0,
                        y_overlap = 0;

                    if(new_x >= i*8+4) {
                        x_overlap = (new_x-e->w/2)-(i*8+8);
                    }
                    else {
                        x_overlap = (new_x+e->w/2)-(i*8);
                    }

                    if(new_y >= j*8+4) {
                        y_overlap = (new_y-e->h/2)-(j*8+8);
                    }
                    else {
                        y_overlap = (new_y+e->h/2)-(j*8);
                    }

                    if(fabs(x_overlap) > fabs(y_overlap)) {
                        if(e->type != ENTITY_BRAIN_ALIEN && fabs(e->y_vel) > 2) {
                            hurt_entity(e, (fabs(e->y_vel) / 32));
                            m->tiles[i][j] = 0;
                        }
                        e->y -= y_overlap;
                        e->y_vel *= -0.1;
                    }
                    else {
                        if(e->type != ENTITY_BRAIN_ALIEN && fabs(e->x_vel) > 2) {
                            hurt_entity(e, (fabs(e->y_vel) / 32));
                            m->tiles[i][j] = 0;
                        }
                        e->x -= x_overlap;
                        e->x_vel *= -0.1;
                    }
                }
            }
        }
    }

    foreach(i, da_size(*projectiles)) {
        if((*projectiles)[i].source_id != e->id &&
           (*projectiles)[i].x >= e->x && (*projectiles)[i].x <= e->x + e->w &&
           (*projectiles)[i].y >= e->y && (*projectiles)[i].y <= e->y + e->h) {
            hurt_entity(e, (*projectiles)[i].damage);
            r32 angle = atan2(e->y - (*projectiles)[i].y, e->x - (*projectiles)[i].x);
            e->x_vel += cos(angle)*((*projectiles)[i].damage)*4;
            e->y_vel += sin(angle)*((*projectiles)[i].damage)*4;

            da_erase(*projectiles, i);
            --i;
        }
    }

    e->x += e->x_vel;
    e->y += e->y_vel;

    switch(e->type) {
        case ENTITY_EXPLORER_DRONE: { update_explorer_drone(e, lighting); break; }
        case ENTITY_DIGGER_DRONE:   { update_digger_drone(e, m, game_state, lighting); break; }
        case ENTITY_FIGHTER_DRONE:  { update_fighter_drone(e, m, projectiles, lighting); break; }
        case ENTITY_BRAIN_ALIEN:    { update_brain_alien(e, m, lighting); break; }
        default: break;
    }
}
