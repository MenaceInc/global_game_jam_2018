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

    request_shader(SHADER_CRT);

    return s;
}

// clean up a splash state
void clean_up_splash(State *s) {
    unrequest_shader(SHADER_CRT);

    free(s->memory);
    s->memory = NULL;
    s->type = 0;
}

// update the global state variable as if it's a
// splash state.
void update_splash() {
    SplashData *s = (SplashData *)state.memory;
    s->sin_pos += 0.007;

    r32 sin_val = sin(s->sin_pos);
    sin_val *= sin_val;

    if((sin_val <= 0.001 && s->sin_pos >= 1) || last_key || left_mouse_pressed) {
        next_state = init_game();
    }

    bind_fbo(&crt_render);
    if(sin_val > 0.3) {
        draw_text(&fonts[FONT_BASE], ALIGN_CENTER_X | ALIGN_CENTER_Y, (sin_val - 0.3)*5, 1, 1, 1, CRT_W/2, CRT_H/2, 0.5, 0.85, 0.3,
                  "Booting...");
    }
    bind_fbo(NULL);
}
