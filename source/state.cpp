#include "state.h"

// state transition variable, used for fading in/out between state changes
global r32 state_t = 1;
// two states: state, next_state. when next_state has been initialized, a state_change will
// automatically take place. to change the state, all one needs to do is:
//
// next_state = init_another_state();
// (obviously init_another_state() being a function that returns a properly
// prepared state)
global State state, next_state;

// we'll include all state .cpp files in here.
#include "state_splash.cpp"
#include "state_game.cpp"

// these functions are to operate on the "state" object.

// update state just calls the appropriate update function
// depending on state.type
void update_state() {
    switch(state.type) {
        case STATE_SPLASH: { update_splash(); break; }
        case STATE_GAME:   { update_game(); break; }
        default: break;
    }
}

// similarly, clean_up_state calls the appropriate clean up function
// depending on state.type
void clean_up_state() {
    switch(state.type) {
        case STATE_SPLASH: { clean_up_splash(&state); break; }
        case STATE_GAME:   { clean_up_game(&state); break; }
        default: break;
    }
}
