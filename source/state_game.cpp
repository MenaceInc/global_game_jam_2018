#include "state.h"

struct GameData {

};

State init_game() {
    State s;
    s.type = STATE_SPLASH;
    s.memory = malloc(sizeof(SplashData));
    ((SplashData *)s.memory)->sin_pos = 0;
    return s;
}

void clean_up_game(State *s) {
    free(s->memory);
    s->memory = NULL;
    s->type = 0;
}

void update_game() {

}

