#ifndef STATE_H
#define STATE_H

// state types
enum {
    STATE_NULL,
    STATE_SPLASH,
    STATE_TITLE,
    STATE_GAME,
    MAX_STATE
};

struct State {
    i8 type;
    void *memory;
};

// declare state init functions here so they can for sure be used anywhere.
State init_splash();
State init_title();
State init_game();

#endif
