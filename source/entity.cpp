#include "entity.h"

#include "entity_explorer_drone.cpp"

#define draw_entity(e, c) {\
            switch(e.type) {\
                case ENTITY_EXPLORER_DRONE: { draw_explorer_drone(&e, c); break; }\
                default: break;\
            }\
        }

void clean_up_entity(Entity *e) {
    switch(e->type) {
        case ENTITY_EXPLORER_DRONE: {
            clean_up_explorer_drone(e);
            break;
        }
        default: break;
    }
    e->type = -1;
    e->id = -1;
}

void update_entity(Map *m, Entity *e, LightState lighting[MAX_EXPLORER]) {
    r32 old_x = e->x+e->w/2,
        old_y = e->y+e->h/2,
        new_x = e->x+e->x_vel+e->w/2,
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
                            play_sound(&sounds[SOUND_HURT], 0.5, 1, 0, AUDIO_ENTITY);
                            e->health -= (1-e->defense) * (fabs(e->y_vel) / 32);
                        }
                        e->y -= y_overlap;
                        e->y_vel *= -0.1;
                    }
                    else {
                        if(fabs(e->x_vel) > 2) {
                            play_sound(&sounds[SOUND_HURT], 0.5, 1, 0, AUDIO_ENTITY);
                            e->health -= (1-e->defense) * (fabs(e->x_vel) / 32);
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
        default: break;
    }
}
