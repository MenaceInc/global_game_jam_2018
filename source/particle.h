#ifndef PARTICLE_H
#define PARTICLE_H

struct ParticleGroup {
    i8 type;
    r32 *x_pos,
        *y_pos,
        *x_vel,
        *y_vel,
        *life;

    InstancedList il;
};

#endif
