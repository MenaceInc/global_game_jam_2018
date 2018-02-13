#ifndef UI_H
#define UI_H

#define MAX_UI_RENDER 200
#define GEN_ID (UI_SRC_ID+__LINE__)

enum ElementType {
    ELEMENT_TEXT_BUTTON,
    ELEMENT_TOGGLER,
    ELEMENT_SLIDER,
    ELEMENT_LINE_EDIT,
    ELEMENT_CONTAINER,
};

struct UIRender {
    r64 id;
    i16 type;
    i8 updated;
    r32 x, y, w, h, t_hot, t_active, font_scale;
    hmm_vec4 clip;
    const char *text;

    union {
        struct { //line-edit
            char *edit_text;
        };
        struct { //container
            FBO *fbo;
        };
        struct { //toggler
            Texture *texture;
            i16 tx, ty, tw, th;
            i8 checked;
        };
        struct { //slider
            r32 value;
        };
    };
};

struct UIState {
    UIRender renders[MAX_UI_RENDER];
    r64 focused_ids[MAX_UI_RENDER], hot, active, container_id;
    i8 focusing, caret_wait;
    i16 render_count,
        focused_id_count, current_focused_id, current_focus_group,
        text_edit_pos,
        update_pos;
    r32 container_x, container_y, container_w, container_h, main_title_y;
    FBO *container_fbo;
    const char *container_title;
    const char *main_title;
};

#endif
