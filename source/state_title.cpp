#define UI_SRC_ID 100

#include "state.h"

struct TitleData {
    i8 settings_state, selected_control;
};

State init_title() {
    State s;
    s.type = STATE_TITLE;
    s.memory = malloc(sizeof(TitleData));
    TitleData *t = (TitleData *)s.memory;
    t->settings_state = -1;
    t->selected_control = -1;

    return s;
}

void clean_up_title(State *s) {
    free(s->memory);
    s->memory = NULL;
    s->type = 0;
}

void update_title() {
    TitleData *t = (TitleData *)state.memory;
    if(t->settings_state >= 0) {
        do_settings_menu(&t->settings_state, &t->selected_control);
    }
    else {
        ui_focus(0);
        {
            if(do_button(GEN_ID, CRT_W/2 - 64, CRT_H/2 - 48, 128, 32, "New Game", 0.3)) {
                next_state = init_game();
            }
            if(do_button(GEN_ID, CRT_W/2 - 64, CRT_H/2 - 17, 128, 32, "Settings", 0.3)) {
                t->settings_state = 0;
            }
            if(do_button(GEN_ID, CRT_W/2 - 64, CRT_H/2 + 14, 128, 32, "Quit", 0.3)) {
                glfwSetWindowShouldClose(window, 1);
            }
        }
        ui_defocus();
    }
}

#undef UI_SRC_ID
