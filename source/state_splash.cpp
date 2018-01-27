#include "state.h"

// struct that holds all data necessary for the state
// to update
struct SplashData {
    r32 sin_pos;
};

// init a splash state
State init_splash() {
    State s;
    s.type = STATE_SPLASH;
    s.memory = malloc(sizeof(SplashData));
    ((SplashData *)s.memory)->sin_pos = 0;
    return s;
}

// clean up a splash state
void clean_up_splash(State *s) {
    free(s->memory);
    s->memory = NULL;
    s->type = 0;
}

// update the global state variable as if it's a
// splash state.
void update_splash() {
    prepare_for_2d();

    SplashData *s = (SplashData *)state.memory;
    s->sin_pos += 0.007;

    r32 sin_val = sin(s->sin_pos);
    sin_val *= sin_val;

    if((sin_val <= 0.001 && s->sin_pos >= 1) || last_key || left_mouse_pressed) {
        next_state = init_game();
    }

    r32 zoom_val = 0.4 + s->sin_pos / 10;

    tint = HMM_Vec4(sin_val, sin_val, sin_val, sin_val);
    draw_text(&fonts[FONT_TITLE], ALIGN_CENTER_X | ALIGN_CENTER_Y, 1, 1, 1, 1, window_w/2, window_h/2, zoom_val, 0.75, 0.3,
              "Splash Screen Text");

    tint = HMM_Vec4(1, 1, 1, 1);
}
