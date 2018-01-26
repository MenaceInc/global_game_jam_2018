#ifndef STATE_H
#define STATE_H

// state types
enum {
    STATE_NULL,
    STATE_SPLASH,
    MAX_STATE
};

struct State {
    i8 type;
    void *memory;
};

// declare state init functions here so they can for sure be used anywhere.
State init_splash();

#endif
