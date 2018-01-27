#ifndef ENTITY_H
#define ENTITY_H

enum {
    ENTITY_ROCKET_DRONE,
    MAX_ENTITY
};

struct RocketDrone {

};

struct Entity {
    i16 id, type;
    r32 x, y, w, h,
        x_vel, y_vel;

    union {
        RocketDrone *rocket_drone;
        void *data;
    };
};

#endif
