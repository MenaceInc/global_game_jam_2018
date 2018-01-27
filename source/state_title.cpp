#define UI_SRC_ID 100

#include "state.h"

struct TitleData {

};

State init_title() {
    State s;
    s.type = STATE_TITLE;
    s.memory = malloc(sizeof(TitleData));

    return s;
}

void clean_up_title(State *s) {
    free(s->memory);
    s->memory = NULL;
    s->type = 0;
}

void update_title() {
    SplashData *s = (SplashData *)state.memory;
    ui_focus(0);
    {
        if(do_button(GEN_ID, CRT_W/2 - 64, CRT_H/2 - 48, 128, 32, "New Game", 0.3)) {
            next_state = init_game();
        }
        if(do_button(GEN_ID, CRT_W/2 - 64, CRT_H/2 - 17, 128, 32, "Settings", 0.3)) {

        }
        if(do_button(GEN_ID, CRT_W/2 - 64, CRT_H/2 + 14, 128, 32, "Quit", 0.3)) {
            glfwSetWindowShouldClose(window, 1);
        }
    }
    ui_defocus();
}

#undef UI_SRC_ID
