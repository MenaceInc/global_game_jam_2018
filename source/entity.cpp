#include "entity.h"

#include "entity_rocket_drone.cpp"

#define draw_entity(e, c) {\
            switch(e.type) {\
                case ENTITY_ROCKET_DRONE: { draw_rocket_drone(&e, c); break; }\
                default: break;\
            }\
        }

void update_entity(Map *m, Entity *e) {
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
                        e->y -= y_overlap;
                        e->y_vel *= -0.1;
                    }
                    else {
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
        case ENTITY_ROCKET_DRONE: { update_rocket_drone(e); break; }
        default: break;
    }
}
