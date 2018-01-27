#ifndef LIGHT_H
#define LIGHT_H

#define MAX_LIGHT 32

struct Light {
    r32 x, y, radius, intensity,
        r, g, b;
};

struct LightState {
    Light lights[MAX_LIGHT];
    i16 light_count;
};

#endif
