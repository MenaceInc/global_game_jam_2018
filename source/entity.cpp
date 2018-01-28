#include "entity.h"

enum {
    ARMOR_STEEL,
    ARMOR_REACTIVE,
    ARMOR_ENERGY,
    MAX_ARMOR
};

struct {
    r32 defense;
} armor_data[MAX_ARMOR] = {
    { 0.4 },
    { 0.6 },
    { 0.9 },
};

#include "entity_explorer_drone.cpp"
#include "entity_digger_drone.cpp"

#define draw_entity(e, c) {\
            switch(e.type) {\
                case ENTITY_EXPLORER_DRONE: { draw_explorer_drone(&e, c); break; }\
                case ENTITY_DIGGER_DRONE:   { draw_digger_drone(&e, c); break; }\
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
        default: break;
    }
    e->type = -1;
    e->id = -1;
    e->data = NULL;
}

void update_entity(Map *m, GameState *game_state, Entity *e, LightState lighting[MAX_EXPLORER]) {
    r32 new_x = e->x+e->x_vel+e->w/2,
        new_y = e->y+e->y_vel+e->h/2;

    e->x_vel *= 0.99;
    e->y_vel += 0.01;

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
                        if(fabs(e->y_vel) > 2) {
                            play_sound_at_point(&sounds[SOUND_HURT], e->x, e->y, 0.5, 1, 0, AUDIO_ENTITY);
                            e->health -= (1-e->defense) * (fabs(e->y_vel) / 32);
                            m->tiles[i][j] = 0;
                        }
                        e->y -= y_overlap;
                        e->y_vel *= -0.1;
                    }
                    else {
                        if(fabs(e->x_vel) > 2) {
                            play_sound_at_point(&sounds[SOUND_HURT], e->x, e->y, 0.5, 1, 0, AUDIO_ENTITY);
                            e->health -= (1-e->defense) * (fabs(e->x_vel) / 32);
                            m->tiles[i][j] = 0;
                        }
                        e->x -= x_overlap;
                        e->x_vel *= -0.1;
                    }
                }
            }
        }
    }

    e->x += e->x_vel;
    e->y += e->y_vel;

    switch(e->type) {
        case ENTITY_EXPLORER_DRONE: { update_explorer_drone(e, lighting); break; }
        case ENTITY_DIGGER_DRONE:   { update_digger_drone(e, m, game_state, lighting); break; }
        default: break;
    }
}
