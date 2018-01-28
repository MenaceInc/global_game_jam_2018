#define UI_SRC_ID 100

#include "state.h"

struct TitleData {
    i8 settings_state, selected_control;
    SoundSource *bg_music_src;
};

State init_title() {
    State s;
    s.type = STATE_TITLE;
    s.memory = malloc(sizeof(TitleData));
    TitleData *t = (TitleData *)s.memory;
    t->settings_state = -1;
    t->selected_control = -1;
    t->bg_music_src = reserve_sound_source();

    request_sound(SOUND_THEME);

    return s;
}

void clean_up_title(State *s) {
    unrequest_sound(SOUND_THEME);

    TitleData *t = (TitleData *)s->memory;
    stop_source(t->bg_music_src);
    unreserve_sound_source(t->bg_music_src);

    free(s->memory);
    s->memory = NULL;
    s->type = 0;
}

void update_title() {
    TitleData *t = (TitleData *)state.memory;

    if(!source_playing(t->bg_music_src)) {
        play_source(t->bg_music_src, &sounds[SOUND_THEME], 1-state_t, 1, 1, AUDIO_MUSIC);
    }
    else {
        set_source_volume(t->bg_music_src, 1-state_t);
    }

    if(t->settings_state >= 0) {
        do_settings_menu(&t->settings_state, &t->selected_control);
    }
    else {
        ui_focus(0);
        {
            if(do_button(GEN_ID, CRT_W/2 - 64, CRT_H/2, 128, 32, "New Game", 0.3)) {
                next_state = init_game();
            }
            if(do_button(GEN_ID, CRT_W/2 - 64, CRT_H/2 + 31, 128, 32, "Settings", 0.3)) {
                t->settings_state = 0;
                reset_ui_current_focus();
            }
            if(do_button(GEN_ID, CRT_W/2 - 64, CRT_H/2 + 62, 128, 32, "Quit", 0.3)) {
                glfwSetWindowShouldClose(window, 1);
            }
        }
        ui_defocus();

        bind_fbo(&crt_render);
        draw_text(&fonts[FONT_BASE], ALIGN_CENTER_X, 1, 1, 1, 1, CRT_W/2, 128, 0.45, "Lunar Drone");
        bind_fbo(NULL);
    }
}

#undef UI_SRC_ID
