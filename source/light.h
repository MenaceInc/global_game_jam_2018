#ifndef LIGHT_H
#define LIGHT_H

#define MAX_LIGHT 1024

struct Light {
    r32 x, y, radius, intensity,
        r, g, b;
};

struct LightState {
    Light lights[MAX_LIGHT];
    i16 light_count;
    r32 default_light;
};

void do_light(LightState *l, r32 x, r32 y, r32 radius, r32 intensity, r32 r, r32 g, r32 b);

#endif
