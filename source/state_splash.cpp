#include "state.h"

// struct that holds all data necessary for the state
// to update
struct SplashData {
    i16 wait;
};

// init a splash state
State init_splash() {
    State s;
    s.type = STATE_SPLASH;
    s.memory = malloc(sizeof(SplashData));
    ((SplashData *)s.memory)->wait = 0;

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
    SplashData *s = (SplashData *)state.memory;
    ++s->wait;

    const char *message_list[8] = {
        "Booting",
        "Allocating storage",
        "Initializing production systems",
        "Compiling drone software",
        "Dabbing on h8rs",
        "Checking privilege",
        "Praising God Emperor",
        "Finishing...",
    };

    i32 message_waits[8] = {
        0,
        16,
        16,
        32,
        64,
        2,
        12,
        64,
    };

    bind_fbo(&crt_render);
    i16 wait_sum = 0;
    for(i16 i = 0; i < 8; i++) {
        wait_sum += message_waits[i];
        if(s->wait >= wait_sum) {
            draw_text(&fonts[FONT_BASE], 0, 1, 1, 1, 1, 32, 32+i*20, 0.2, 0.9, 0.2, message_list[i]);
            if(i < 7) {
                r32 loading_percentage = ((r32)s->wait - wait_sum) / (message_waits[i+1]);
                loading_percentage > 1 ? loading_percentage = 1 : loading_percentage;
                draw_text(&fonts[FONT_BASE], 0, 1, 1, 1, 1, 264, 32+i*20, 0.2, 0.9, 0.2, "[");
                const char *loading_bar = "########################";
                draw_textn(&fonts[FONT_BASE], 0, 0, 1, 0, 1, 272, 32+i*20, 0.2, 0.9, 0.2, loading_bar, loading_percentage < 0.99 ? loading_percentage*strlen(loading_bar) : strlen(loading_bar));
                draw_text(&fonts[FONT_BASE], 0, 1, 1, 1, 1, 274 + text_width(&fonts[FONT_BASE], loading_bar) * 0.2, 32+i*20, 0.2, 0.9, 0.2, "]");
            }
        }
    }
    bind_fbo(NULL);

    if(s->wait >= wait_sum + 128) {
        next_state = init_title();
    }
}
