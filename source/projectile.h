#ifndef PROJECTILE_H
#define PROJECTILE_H

struct Projectile {
    i16 source_id;
    i32 tiles_left;
    r32 x, y, x_vel, y_vel,
        damage;
};

#endif
