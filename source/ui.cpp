#define UI_SRC_ID 3000

#include "ui.h"

global UIState ui;

#define ui_id_equal(id1, id2) ((int)(id1*10000.0) == (int)(id2*10000.0))
#define mouse_over(x, y, w, h) (mouse_x >= x && mouse_x <= x+w && mouse_y >= y && mouse_y <= y+h)

#define do_checkbox(id, x, y, w, h, checked, text, font_scale)  do_toggler(id, x, y, w, h, checked, &textures[TEX_UI], 96, 64, 48, 48, text, font_scale)
#define do_radio(id, x, y, w, h, checked, text, font_scale)     do_toggler(id, x, y, w, h, checked, &textures[TEX_UI], 48, 64, 48, 48, text, font_scale)

#define play_ui_hot_sound() play_sound(&sounds[SOUND_BUTTON], 0.2, random(0.9, 1), 0, AUDIO_UI)
#define play_ui_fire_sound() play_sound(&sounds[SOUND_BUTTON], 0.2, 1.5, 0, AUDIO_UI)

#define reset_ui_current_focus() { ui.current_focused_id = ui.current_focused_id >= 0 ? 0 : -1; ui.current_focus_group = 0; }

#define ui_down_press     (last_key == KEY_DOWN || gamepad_button_pressed(12))
#define ui_up_press       (last_key == KEY_UP || gamepad_button_pressed(10))
#define ui_left_press     (last_key == KEY_LEFT || gamepad_button_pressed(13))
#define ui_right_press    (last_key == KEY_RIGHT || gamepad_button_pressed(11))
#define ui_escape_press   (last_key == KEY_ESCAPE || gamepad_button_pressed(7))
#define ui_accept_press   (last_key == KEY_ENTER || gamepad_button_pressed(0))

#define ui_down_hold     (key_down[KEY_DOWN] || gamepad_button_down(12))
#define ui_up_hold       (key_down[KEY_UP] || gamepad_button_down(10))
#define ui_left_hold     (key_down[KEY_LEFT] || gamepad_button_down(13))
#define ui_right_hold    (key_down[KEY_RIGHT] || gamepad_button_down(11))

UIRender init_ui_render(r64 id, i16 type, r32 x, r32 y, r32 w, r32 h, r32 font_scale, const char *text) {
    UIRender r;
    r.id = id;
    r.type = type;
    r.updated = 1;
    r.x = x;
    r.y = y;
    r.w = w;
    r.h = h;
    r.t_hot = 0;
    r.t_active = 0;
    r.font_scale = font_scale;
    r.clip = clip;
    r.text = text;
    return r;
}

void init_ui() {
    ui.hot = -1;
    ui.active = -1;
    ui.focusing = 0;
    ui.render_count = 0;
    ui.focused_id_count = 0;
    ui.current_focused_id = 0;
    ui.current_focus_group = 0;
    ui.last_mouse_x = mouse_x;
    ui.last_mouse_y = mouse_y;
    ui.container_x = 0;
    ui.container_y = 0;
    ui.container_fbo = NULL;
    ui.container_title = NULL;
    ui.main_title = NULL;
    ui.main_title_y = 0;
}

void ui_begin() {
    ui.focused_id_count = 0;
    ui.update_pos = 0;
    ui.main_title = NULL;
}

void ui_end() {
    ++ui.caret_wait;
    if(ui.caret_wait >= 60) {
        ui.caret_wait = 0;
    }

    for(i16 i = ui.render_count-1; i >= 0 && i < ui.render_count; --i) {
        if(ui.renders[i].updated) {
            switch(ui.renders[i].type) {
                case ELEMENT_TEXT_BUTTON:
                case ELEMENT_TOGGLER:
                case ELEMENT_SLIDER:
                case ELEMENT_LINE_EDIT: {
                    draw_filled_rect(0, 0, 0, 0.3+ui.renders[i].t_hot*0.1,
                                     ui.renders[i].x,
                                     ui.renders[i].y,
                                     ui.renders[i].w,
                                     ui.renders[i].h);

                    draw_filled_rect(0, 0, 0, ui.renders[i].t_hot*0.5,
                                     ui.renders[i].x,
                                     ui.renders[i].y,
                                     ui.renders[i].w,
                                     ui.renders[i].h*ui.renders[i].t_hot);

                    draw_rect(0.8+ui.renders[i].t_hot*0.2,
                              0.8+ui.renders[i].t_hot*0.2,
                              0.8+ui.renders[i].t_hot*0.2,
                              0.8+ui.renders[i].t_hot*0.2,
                              ui.renders[i].x,
                              ui.renders[i].y,
                              ui.renders[i].w,
                              ui.renders[i].h,
                              1);

                    if(ui.renders[i].type == ELEMENT_LINE_EDIT) {
                        if(strlen(ui.renders[i].edit_text) || ui_id_equal(ui.active, ui.renders[i].id)) {
                            set_render_clip(ui.renders[i].x, ui.renders[i].y,
                                            ui.renders[i].x + ui.renders[i].w,
                                            ui.renders[i].y + ui.renders[i].h);

                            r32 text_length = text_width_n(&fonts[FONT_BASE], ui.renders[i].edit_text, ui.text_edit_pos) * ui.renders[i].font_scale,
                                caret_pos = ui.renders[i].x + 16 + text_length,
                                text_draw_pos = ui.renders[i].x + 16;

                            if(caret_pos >= ui.renders[i].x + ui.renders[i].w - 16) {
                                text_draw_pos -= caret_pos - (ui.renders[i].x + ui.renders[i].w - 16);
                                caret_pos = ui.renders[i].x + ui.renders[i].w - 16;
                            }

                            if(ui.caret_wait >= 30) {
                                draw_line(ui.renders[i].t_active, ui.renders[i].t_active, ui.renders[i].t_active, ui.renders[i].t_active,
                                             caret_pos, ui.renders[i].y + 8,
                                             caret_pos, ui.renders[i].y + ui.renders[i].h - 8);
                            }

                            draw_text(&fonts[FONT_BASE], ALIGN_CENTER_Y,
                                      1,
                                      1,
                                      1,
                                      1,
                                      text_draw_pos, ui.renders[i].y + 2 + ui.renders[i].h/2, ui.renders[i].font_scale,
                                      0.8, 0.3, ui.renders[i].edit_text);

                            reset_render_clip();
                        }
                        else {
                            draw_text(&fonts[FONT_BASE], ALIGN_CENTER_Y,
                                      0.6 * (1-ui.renders[i].t_active),
                                      0.6 * (1-ui.renders[i].t_active),
                                      0.6 * (1-ui.renders[i].t_active),
                                      0.6 * (1-ui.renders[i].t_active),
                                      ui.renders[i].x+16, ui.renders[i].y + 2 + ui.renders[i].h/2, ui.renders[i].font_scale,
                                      0.8 + 0.2*ui.renders[i].t_hot, 0.3 + 0.2*ui.renders[i].t_hot, ui.renders[i].text);
                        }
                    }
                    else {
                        if(ui.renders[i].type == ELEMENT_SLIDER) {
                            draw_filled_rect(0.9, 0.7, 0.1,
                                             0.5 + ui.renders[i].t_hot*0.1 + ui.renders[i].t_active*0.1,
                                             ui.renders[i].x + 1,
                                             ui.renders[i].y + 1,
                                             (ui.renders[i].w-2)*ui.renders[i].value,
                                             ui.renders[i].h - 2);
                        }

                        if(ui.renders[i].type == ELEMENT_TOGGLER) {
                            min_filter = GL_LINEAR;
                            tint = HMM_Vec4(0.7 + 0.2*ui.renders[i].t_hot,
                                            0.7 + 0.2*ui.renders[i].t_hot,
                                            0.7 + 0.2*ui.renders[i].t_hot,
                                            0.7 + 0.2*ui.renders[i].t_hot);

                            draw_scaled_texture_region(&textures[TEX_UI], 0,
                                                       0, 64,
                                                       48, 48,
                                                       ui.renders[i].x + ui.renders[i].w - 40,
                                                       ui.renders[i].y + 8,
                                                       32, 32, 0);
                            if(ui.renders[i].checked) {
                                draw_scaled_texture_region(ui.renders[i].texture, 0,
                                                           ui.renders[i].tx,
                                                           ui.renders[i].ty,
                                                           ui.renders[i].tw,
                                                           ui.renders[i].th,
                                                           ui.renders[i].x + ui.renders[i].w - 36,
                                                           ui.renders[i].y + 11.5,
                                                           25, 25, 0);
                            }
                            tint = HMM_Vec4(1, 1, 1, 1);

                            draw_text(&fonts[FONT_BASE], ALIGN_CENTER_Y,
                                      1, 1, 1, 1,
                                      ui.renders[i].x+16,
                                      ui.renders[i].y + 2 + ui.renders[i].h/2,
                                      ui.renders[i].font_scale,
                                      0.8 + 0.2*ui.renders[i].t_hot, 0.3 + 0.2*ui.renders[i].t_hot, ui.renders[i].text);
                        }
                        else {
                            draw_text(&fonts[FONT_BASE], ALIGN_CENTER_X | ALIGN_CENTER_Y,
                                      1, 1, 1, 1,
                                      ui.renders[i].x+ui.renders[i].w/2,
                                      ui.renders[i].y + 2 + ui.renders[i].h/2,
                                      ui.renders[i].font_scale,
                                      0.8 + 0.2*ui.renders[i].t_hot, 0.3 + 0.2*ui.renders[i].t_hot, ui.renders[i].text);
                        }
                    }

                    break;
                }
                case ELEMENT_CONTAINER: {
                    draw_filled_rect(0, 0, 0, 0.6, ui.renders[i].x, ui.renders[i].y, ui.renders[i].w, ui.renders[i].h);
                    draw_rect(0.8, 0.8, 0.8, 0.8, ui.renders[i].x, ui.renders[i].y, ui.renders[i].w, ui.renders[i].h, 1);
                    if(ui.renders[i].text) {
                        draw_text(&fonts[FONT_BASE], 0, 1, 1, 1, 1, ui.renders[i].x + 16, ui.renders[i].y + 16, ui.renders[i].font_scale, 0.8, 0.3, ui.renders[i].text);
                    }
                    if(ui.renders[i].fbo) {
                        draw_fbo(ui.renders[i].fbo, 0, ui.renders[i].x, ui.renders[i].y);
                    }
                    break;
                }
                default: break;
            }
            ui.renders[i].updated = 0;
        }
        else {
            if(ui_id_equal(ui.hot, ui.renders[i].id)) {
                ui.hot = -1;
            }
            if(ui_id_equal(ui.active, ui.renders[i].id)) {
                ui.active = -1;
            }
            memmove(ui.renders + i, ui.renders + i + 1, sizeof(UIRender)*(ui.render_count-i-1));
            --ui.render_count;
        }
    }

    if(ui.current_focused_id >= 0 && ui.focused_id_count) {
        if((i32)ui.last_mouse_x != (i32)mouse_x ||
           (i32)ui.last_mouse_y != (i32)mouse_y) {
            ui.current_focused_id = -1;
        }
        else {
            if(!keyboard_used) {
                if(ui_down_press) {
                    ui.active = -1;
                    ++ui.current_focused_id;
                    play_ui_hot_sound();
                }
                else if(ui_up_press) {
                    ui.active = -1;
                    --ui.current_focused_id;
                    play_ui_hot_sound();
                }
            }

            if(ui.current_focused_id >= ui.focused_id_count) {
                ui.current_focused_id = 0;
            }
            else if(ui.current_focused_id < 0) {
                ui.current_focused_id = ui.focused_id_count - 1;
            }

            ui.hot = ui.focused_ids[ui.current_focused_id];
        }
    }
    else {
        ui.current_focused_id = -1;
        if(ui.focused_id_count) {
            if((ui_up_press || ui_down_press) && !keyboard_used) {
                ui.current_focused_id = 0;
                play_ui_hot_sound();
            }
        }
    }

    ui.last_mouse_x = mouse_x;
    ui.last_mouse_y = mouse_y;

    ui.focused_id_count = 0;

    if(ui.main_title) {
        draw_text(&fonts[FONT_TITLE], ALIGN_CENTER_X, 1, 1, 1, 1, window_w/2, ui.main_title_y, 0.5, 0.7, 0.2, ui.main_title);
    }
}

void ui_focus(i8 group) {
    if(ui.current_focus_group == group || !group) {
        ui.focusing = 1;
    }
    else {
        ui.focusing = 0;
    }
}

void ui_defocus() {
    ui.focusing = 0;
}

i8 do_button(r64 id, r32 x, r32 y, r32 w, r32 h, const char *text, r32 font_scale) {
    x += ui.container_x;
    y += ui.container_y;

    if(ui.focusing) {
        ui.focused_ids[ui.focused_id_count++] = id;
    }

    i8 fired = 0;
    if(ui.current_focused_id < 0) {
        if(ui.hot < 0 || ui_id_equal(id, ui.hot)) {
            if(mouse_over(x, y, w, h)) {
                if(ui.hot < 0) {
                    play_ui_hot_sound();
                }
                mouse_position_used = 1;
                ui.hot = id;
            }
            else {
                ui.hot = -1;
            }
        }
        if(ui_id_equal(id, ui.hot)) {
            if(left_mouse_pressed) {
                ui.active = id;
                mouse_buttons_used = 1;
            }
        }
        if(ui_id_equal(id, ui.active)) {
            if(!left_mouse_down) {
                fired = ui_id_equal(id, ui.hot);
                mouse_buttons_used = 1;
                ui.active = -1;
            }
        }
    }
    else {
        fired = !keyboard_used && ui_accept_press && ui_id_equal(id, ui.focused_ids[ui.current_focused_id]);
        if(fired) {
            keyboard_used = 1;
        }
    }

    UIRender *prev_render = NULL;
    for(i16 i = 0; i < ui.render_count; i++) {
        if(ui_id_equal(ui.renders[i].id, id) && i == ui.update_pos) {
            prev_render = &ui.renders[i];
            break;
        }
    }

    if(prev_render) {
        prev_render->updated = 1;
        prev_render->x = x;
        prev_render->y = y;
        prev_render->w = w;
        prev_render->h = h;
        prev_render->clip = clip;
        prev_render->t_hot += ((ui_id_equal(ui.hot, id) ? 1.0 : 0.0) - prev_render->t_hot) * 0.2;
        prev_render->t_active += ((ui_id_equal(ui.active, id) ? 1.0 : 0.0) - prev_render->t_active) * 0.2;
    }
    else {
        ui.renders[ui.render_count++] = init_ui_render(id, ELEMENT_TEXT_BUTTON, x, y, w, h, font_scale, text);
    }

    if(fired) {
        play_ui_fire_sound();
    }

    ++ui.update_pos;
    return fired;
}

i8 do_toggler(r64 id, r32 x, r32 y, r32 w, r32 h, i8 checked, Texture *texture, i16 tx, i16 ty, i16 tw, i16 th, const char *text, r32 font_scale) {
    x += ui.container_x;
    y += ui.container_y;

    if(ui.focusing) {
        ui.focused_ids[ui.focused_id_count++] = id;
    }

    i8 fired = 0;
    if(ui.current_focused_id < 0) {
        if(ui.hot < 0 || ui_id_equal(id, ui.hot)) {
            if(mouse_over(x, y, w, h)) {
                if(ui.hot < 0) {
                    play_ui_hot_sound();
                }
                mouse_position_used = 1;
                ui.hot = id;
            }
            else {
                ui.hot = -1;
            }
        }
        if(ui_id_equal(id, ui.hot)) {
            if(left_mouse_pressed) {
                ui.active = id;
                mouse_buttons_used = 1;
            }
        }
        if(ui_id_equal(id, ui.active)) {
            if(!left_mouse_down) {
                fired = ui_id_equal(id, ui.hot);
                ui.active = -1;
                mouse_buttons_used = 1;
            }
        }
    }
    else {
        fired = !keyboard_used && ui_accept_press && ui_id_equal(id, ui.focused_ids[ui.current_focused_id]);
        if(fired) {
            keyboard_used = 1;
        }
    }

    if(fired) {
        checked = !checked;
        play_ui_fire_sound();
    }

    UIRender *prev_render = NULL;
    for(i16 i = 0; i < ui.render_count; i++) {
        if(ui_id_equal(ui.renders[i].id, id) && i == ui.update_pos) {
            prev_render = &ui.renders[i];
            break;
        }
    }

    if(prev_render) {
        prev_render->updated = 1;
        prev_render->x = x;
        prev_render->y = y;
        prev_render->w = w;
        prev_render->h = h;
        prev_render->clip = clip;
        prev_render->t_hot += ((ui_id_equal(ui.hot, id) ? 1.0 : 0.0) - prev_render->t_hot) * 0.2;
        prev_render->t_active += ((ui_id_equal(ui.active, id) ? 1.0 : 0.0) - prev_render->t_active) * 0.2;
        prev_render->checked = checked;
    }
    else {
        ui.renders[ui.render_count] = init_ui_render(id, ELEMENT_TOGGLER, x, y, w, h, font_scale, text);
        ui.renders[ui.render_count].tx = tx;
        ui.renders[ui.render_count].ty = ty;
        ui.renders[ui.render_count].tw = tw;
        ui.renders[ui.render_count].th = th;
        ui.renders[ui.render_count].texture = texture;
        ui.renders[ui.render_count].checked = checked;
        ++ui.render_count;
    }
    ++ui.update_pos;
    return checked;
}

r32 do_slider(r64 id, r32 x, r32 y, r32 w, r32 h, r32 value, const char *text, r32 font_scale) {
    x += ui.container_x;
    y += ui.container_y;

    if(ui.focusing) {
        ui.focused_ids[ui.focused_id_count++] = id;
    }

    if(ui.current_focused_id < 0) {
        if(ui.hot < 0 || ui_id_equal(id, ui.hot) || ui_id_equal(id, ui.active)) {
            if(mouse_over(x, y, w, h) || ui_id_equal(id, ui.active)) {
                if(ui.hot < 0) {
                    play_ui_hot_sound();
                }
                mouse_position_used = 1;
                ui.hot = id;
            }
            else {
                ui.hot = -1;
            }
        }
        if(ui_id_equal(id, ui.hot)) {
            if(left_mouse_pressed) {
                ui.active = id;
                mouse_buttons_used = 1;
            }
        }
        if(ui_id_equal(id, ui.active)) {
            if(left_mouse_down) {
                value = (mouse_x - x) / w;
                mouse_buttons_used = 1;
            }
            else {
                ui.active = -1;
            }
        }
    }
    else {
        ui.active = ui_id_equal(id, ui.hot) ? id : ui.active;
        if(ui_id_equal(id, ui.active)) {
            if(ui_right_hold && !keyboard_used) {
                value += 0.02;
                keyboard_used = 1;
            }
            if(ui_left_hold && !keyboard_used) {
                value -= 0.02;
                keyboard_used = 1;
            }
        }
    }

    if(value > 1) {
        value = 1;
    }
    else if(value < 0) {
        value = 0;
    }

    UIRender *prev_render = NULL;
    for(i16 i = 0; i < ui.render_count; i++) {
        if(ui_id_equal(ui.renders[i].id, id) && i == ui.update_pos) {
            prev_render = &ui.renders[i];
            break;
        }
    }

    if(prev_render) {
        prev_render->updated = 1;
        prev_render->x = x;
        prev_render->y = y;
        prev_render->w = w;
        prev_render->h = h;
        prev_render->clip = clip;
        prev_render->t_hot += ((ui_id_equal(ui.hot, id) ? 1.0 : 0.0) - prev_render->t_hot) * 0.2;
        prev_render->t_active += ((ui_id_equal(ui.active, id) ? 1.0 : 0.0) - prev_render->t_active) * 0.2;
        prev_render->value = value;
    }
    else {
        ui.renders[ui.render_count] = init_ui_render(id, ELEMENT_SLIDER, x, y, w, h, font_scale, text);
        ui.renders[ui.render_count].value = value;
        ++ui.render_count;
    }

    ++ui.update_pos;
    return value;
}

char *do_line_edit(r64 id, r32 x, r32 y, r32 w, r32 h, const char *blank_text, char *text, r32 font_scale, u16 max_chars) {
    x += ui.container_x;
    y += ui.container_y;

    if(ui.focusing) {
        ui.focused_ids[ui.focused_id_count++] = id;
    }

    i8 text_change = 0;

    if(ui.current_focused_id < 0) {
        if(ui.hot < 0 || ui_id_equal(id, ui.hot)) {
            if(mouse_over(x, y, w, h)) {
                if(ui.hot < 0) {
                    play_ui_hot_sound();
                }
                mouse_position_used = 1;
                ui.hot = id;
            }
            else {
                ui.hot = -1;
            }
        }
        if(left_mouse_pressed) {
            if(ui_id_equal(ui.hot, id)) {
                ui.active = id;
                mouse_buttons_used = 1;
            }
            else {
                if(ui_id_equal(ui.active, id)) {
                    ui.active = -1;
                }
            }
        }
    }
    else {
        ui.active = ui_id_equal(id, ui.focused_ids[ui.current_focused_id]) ? id : ui.active;
    }

    if(ui_id_equal(id, ui.active) && !keyboard_used) {
        if(last_char) {
            if(strlen(text) < max_chars) {
                memmove(text + ui.text_edit_pos + 1, text + ui.text_edit_pos, strlen(text) + 1 - ui.text_edit_pos);
                text[ui.text_edit_pos] = last_char;
                ++ui.text_edit_pos;
                last_char = 0;
                text_change = 1;
            }
            keyboard_used = 1;
        }
        else if(last_key == KEY_BACKSPACE) {
            if(strlen(text) > 0) {
                if(ui.text_edit_pos > 0) {
                    text_change = 1;
                    if(key_down[KEY_LEFT_CONTROL]) {
                        for(i16 i = (i16)ui.text_edit_pos-1; i >= 0; --i) {
                            if(text[i] == ' ' || !i) {
                                memmove(text + i, text + ui.text_edit_pos, strlen(text) + 1 - ui.text_edit_pos);
                                ui.text_edit_pos = i;
                                break;
                            }
                        }
                    }
                    else {
                        memmove(text + ui.text_edit_pos - 1, text + ui.text_edit_pos, strlen(text) - ui.text_edit_pos + 1);
                        --ui.text_edit_pos;
                        last_char = 0;
                    }
                }
            }
            keyboard_used = 1;
        }
        else {
            if(last_key == KEY_RIGHT) {
                i16 last_edit_pos = ui.text_edit_pos,
                    position_change = 0;

                if(key_down[KEY_LEFT_CONTROL] || key_down[KEY_RIGHT_CONTROL]) {
                    for(i16 i = last_edit_pos; i <= (i16)strlen(text); i++) {
                        ++position_change;
                        if(text[i] == ' ' || i == (i16)strlen(text)) {
                            break;
                        }
                    }
                }
                else {
                    position_change = 1;
                }

                ui.text_edit_pos += position_change;

                ui.caret_wait = 30;
            }
            else if(last_key == KEY_LEFT) {
                i16 position_change = 0;

                if(key_down[KEY_LEFT_CONTROL] || key_down[KEY_RIGHT_CONTROL]) {
                    position_change -= 2;
                    for(i16 i = ui.text_edit_pos-2; i >= 0; i--) {
                        if(text[i] != ' ' && i) {
                            --position_change;
                        }
                        else {
                            break;
                        }
                    }
                }
                else {
                    position_change = -1;
                }

                ui.text_edit_pos += position_change;
                ui.caret_wait = 30;
            }
        }

        if(ui.text_edit_pos < 0) {
            ui.text_edit_pos = 0;
        }
        if(ui.text_edit_pos > (i16)strlen(text)) {
            ui.text_edit_pos = (i16)strlen(text);
        }
    }

    UIRender *prev_render = NULL;
    for(i16 i = 0; i < ui.render_count; i++) {
        if(ui_id_equal(ui.renders[i].id, id) && i == ui.update_pos) {
            prev_render = &ui.renders[i];
            break;
        }
    }

    if(prev_render) {
        prev_render->updated = 1;
        prev_render->x = x;
        prev_render->y = y;
        prev_render->w = w;
        prev_render->h = h;
        prev_render->clip = clip;
        prev_render->t_hot += ((ui_id_equal(ui.hot, id) ? 1.0 : 0.0) - prev_render->t_hot) * 0.2;
        prev_render->t_active += ((ui_id_equal(ui.active, id) ? 1.0 : 0.0) - prev_render->t_active) * 0.2;
        if(text_change) {
            ui.caret_wait = 30;
        }
    }
    else {
        ui.renders[ui.render_count] = init_ui_render(id, ELEMENT_LINE_EDIT, x, y, w, h, font_scale, blank_text);
        ui.renders[ui.render_count].edit_text = text;
        ++ui.render_count;
    }

    ++ui.update_pos;
    return text;
}

void start_container(r64 id, r32 x, r32 y, r32 w, r32 h, const char *title, FBO *fbo) {
    clip = HMM_Vec4(x, y, x+w, y+h);
    ui.container_title = title;
    ui.container_fbo = fbo;
    ui.container_x = x;
    ui.container_y = y;
    ui.container_w = w;
    ui.container_h = h;
    ui.container_id = id;
}

void start_container(r64 id, r32 x, r32 y, r32 w, r32 h) {
    start_container(id, x, y, w, h, NULL, NULL);
}

void start_container(r64 id, r32 x, r32 y, r32 w, r32 h, FBO *fbo) {
    start_container(id, x, y, w, h, NULL, fbo);
}

void start_container(r64 id, r32 x, r32 y, r32 w, r32 h, const char *title) {
    start_container(id, x, y, w, h, title, NULL);
}

void end_container() {
    UIRender *prev_render = NULL;
    for(i16 i = 0; i < ui.render_count; i++) {
        if(ui_id_equal(ui.renders[i].id, ui.container_id) && i == ui.update_pos) {
            prev_render = &ui.renders[i];
            break;
        }
    }

    if(prev_render) {
        prev_render->x = ui.container_x;
        prev_render->y = ui.container_y;
        prev_render->w = ui.container_w;
        prev_render->h = ui.container_h;
        prev_render->clip = clip;
        prev_render->updated = 1;
    }
    else {
        ui.renders[ui.render_count] = init_ui_render(ui.container_id, ELEMENT_CONTAINER, ui.container_x, ui.container_y, ui.container_w, ui.container_h, 0.35, ui.container_title);
        ui.renders[ui.render_count].fbo = ui.container_fbo;
        ++ui.render_count;
    }

    clip = HMM_Vec4(0, 0, window_w, window_h);
    ui.container_x = 0;
    ui.container_y = 0;

    ++ui.update_pos;
}

void do_settings_menu(i8 *settings_state, i8 *selected_control) {
    enum {
        SETTINGS_MAIN,
        SETTINGS_CONTROLS_MAIN,
        SETTINGS_CONTROLS_KEYBOARD,
        SETTINGS_CONTROLS_GAMEPAD,
        SETTINGS_AUDIO,
        SETTINGS_GRAPHICS,
        SETTINGS_SCREEN,
    };

    const char *settings_state_names[] = {
        "Settings",
        "Controls",
        "Keyboard Controls",
        "Gamepad Controls",
        "Audio",
        "Graphics",
        "Screen"
    };

    if(*settings_state >= 0) {
        ui.main_title = settings_state_names[*settings_state];
    }

    switch(*settings_state) {
        case SETTINGS_MAIN: {
            ui_focus(0);
            {
                r32 block_height = 269,
                    element_y = window_h/2 - block_height/2;

                ui.main_title_y = element_y - 64;

                if(do_button(GEN_ID, window_w/2 - 128, element_y, 256, 48, "Controls", 0.35)) {
                    *settings_state = SETTINGS_CONTROLS_MAIN;
                    *selected_control = -1;
                    reset_ui_current_focus();
                }
                element_y += 47;
                if(do_button(GEN_ID, window_w/2 - 128, element_y, 256, 48, "Audio", 0.35)) {
                    *settings_state = SETTINGS_AUDIO;
                    reset_ui_current_focus();
                }
                element_y += 47;
                if(do_button(GEN_ID, window_w/2 - 128, element_y, 256, 48, "Graphics", 0.35)) {
                    *settings_state = SETTINGS_GRAPHICS;
                    reset_ui_current_focus();
                }
                element_y += 47;
                if(do_button(GEN_ID, window_w/2 - 128, element_y, 256, 48, "Screen", 0.35)) {
                    *settings_state = SETTINGS_SCREEN;
                    reset_ui_current_focus();
                }
                element_y += 80;
                if(do_button(GEN_ID, window_w/2 - 128, element_y, 256, 48, "Back", 0.35)) {
                    *settings_state = -1;
                    save_settings();
                    reset_ui_current_focus();
                }
            }
            ui_defocus();
            break;
        }
        case SETTINGS_CONTROLS_MAIN: {
            ui_focus(0);
            {
                r32 block_height = 47*2 + 80,
                    element_y = window_h/2 - block_height/2;

                ui.main_title_y = element_y - 64;

                if(do_button(GEN_ID, window_w/2 - 128, element_y, 256, 48, "Keyboard Controls", 0.35)) {
                    *settings_state = SETTINGS_CONTROLS_KEYBOARD;
                    *selected_control = -1;
                    reset_ui_current_focus();
                    ui.current_focus_group = 1;
                }
                element_y += 47;
                if(do_button(GEN_ID, window_w/2 - 128, element_y, 256, 48, "Gamepad Controls", 0.35)) {
                    *selected_control = -1;
                    *settings_state = SETTINGS_CONTROLS_GAMEPAD;
                    reset_ui_current_focus();
                }
                element_y += 80;

                if(do_button(GEN_ID, window_w/2 - 128, element_y, 256, 48, "Back", 0.35)) {
                    *settings_state = SETTINGS_MAIN;
                    reset_ui_current_focus();
                }
            }
            ui_defocus();

            break;
        }
        case SETTINGS_CONTROLS_KEYBOARD: {
            r32 block_height = 5*47 + 80,
                element_x = window_w/2 - 352,
                element_y = window_h/2 - block_height/2;

            ui.main_title_y = element_y - 64;

            if(ui_left_press || ui_right_press) {
                ui.current_focus_group = ui.current_focus_group == 1 ? 2 : 1;
                play_ui_hot_sound();
            }

            i8 current_focus_group = 1;

            if(*selected_control >= 0) {
                if(left_mouse_pressed) {
                    *selected_control = -1;
                }
                if(last_key) {
                    key_control_maps[*selected_control] = last_key;
                    *selected_control = -1;
                    keyboard_used = 1;
                }
            }

            for(i16 i = 0; i < MAX_KEY_CONTROL; i++) {
                ui_focus(current_focus_group);
                {
                    if(do_button(GEN_ID+ i/100.f, element_x, element_y, 192, 48, key_control_names[i], 0.35)) {
                        *selected_control = i;
                    }

                    draw_filled_rect(0, 0, 0, 0.8, element_x + 191, element_y, 144, 48);
                    draw_rect(0.8, 0.8, 0.8, 0.8, element_x + 191, element_y, 144, 48, 1);
                    draw_text(&fonts[FONT_BASE], 0,
                              *selected_control == i ? 1 : 0.7,
                              *selected_control == i ? 1 : 0.7,
                              *selected_control == i ? 1 : 0.7,
                              *selected_control == i ? 1 : 0.7,
                              element_x + 204, element_y + 12, 0.35, key_name(key_control_maps[i]));

                    element_y += 47;
                    if(element_y >= window_h/2 + block_height/2 - 80) {
                        element_y = window_h/2 - block_height/2;
                        element_x += 368;
                        ++current_focus_group;
                    }
                }
                ui_defocus();
            }

            ui_focus(0);
            {
                if(do_button(GEN_ID, window_w/2 - 128, window_h/2 + block_height/2 - 47, 256, 48, "Back", 0.35)) {
                    *settings_state = SETTINGS_CONTROLS_MAIN;
                    reset_ui_current_focus();
                }
            }
            ui_defocus();

            break;
        }
        case SETTINGS_CONTROLS_GAMEPAD: {
            ui_focus(0);
            {
                r32 block_height = MAX_GP_CONTROL*47 + 80,
                    element_y = window_h/2 - block_height/2;

                ui.main_title_y = element_y - 64;

                if(*selected_control >= 0) {
                    if(left_mouse_pressed) {
                        *selected_control = -1;
                    }
                    for(i32 i = 0; i < gamepad_button_count; i++) {
                        if(gamepad_button_pressed(i)) {
                            gamepad_control_maps[*selected_control] = i;
                            *selected_control = -1;
                            keyboard_used = 1;
                            break;
                        }
                    }
                }

                for(i8 i = 0; i < MAX_GP_CONTROL; i++) {
                    if(do_button(GEN_ID + i/100.f, window_w/2 - 168, element_y, 192, 48, gamepad_control_names[i], 0.35)) {
                        *selected_control = i;
                    }

                    draw_filled_rect(0, 0, 0, 0.8, window_w/2 + 24, element_y, 144, 48);
                    draw_rect(0.8, 0.8, 0.8, 0.8, window_w/2 + 24, element_y, 144, 48, 1);

                    char num_str[16] = { 0 };
                    sprintf(num_str, "%i", gamepad_control_maps[i]);

                    draw_text(&fonts[FONT_BASE], 0,
                              *selected_control == i ? 1 : 0.7,
                              *selected_control == i ? 1 : 0.7,
                              *selected_control == i ? 1 : 0.7,
                              *selected_control == i ? 1 : 0.7,
                              window_w/2 + 40, element_y + 12, 0.35, num_str);

                    element_y += 47;
                }

                element_y += 32;

                if(do_button(GEN_ID, window_w/2 - 128, element_y, 256, 48, "Back", 0.35)) {
                    *settings_state = SETTINGS_CONTROLS_MAIN;
                    reset_ui_current_focus();
                }
            }
            ui_defocus();

            break;
        }
        case SETTINGS_AUDIO: {
            ui_focus(0);
            {
                r32 block_height = MAX_AUDIO*47 + 79,
                    element_y = window_h/2 - block_height/2;

                ui.main_title_y = element_y - 64;

                for(i8 i = 0; i < MAX_AUDIO; i++) {
                    audio_type_volumes[i] = do_slider(GEN_ID + (i/100.f),
                                                      window_w/2 - 128,
                                                      element_y,
                                                      256, 48, audio_type_volumes[i], audio_type_names[i], 0.35);
                    element_y += 47;
                }

                element_y += 32;

                if(do_button(GEN_ID, window_w/2 - 128, element_y, 256, 48, "Back", 0.35)) {
                    *settings_state = SETTINGS_MAIN;
                    reset_ui_current_focus();
                }
            }
            ui_defocus();

            break;
        }
        case SETTINGS_GRAPHICS: {
            ui_focus(0);
            {
                r32 block_height = 175,
                    element_y = window_h/2 - block_height/2;

                ui.main_title_y = element_y - 64;

                if(do_button(GEN_ID, window_w/2 -128, element_y + 122, 256, 48, "Back", 0.35)) {
                    *settings_state = SETTINGS_MAIN;
                    reset_ui_current_focus();
                }
            }
            ui_defocus();
            break;
        }
        case SETTINGS_SCREEN: {
            ui_focus(0);
            {
                r32 block_height = 128,
                    element_y = window_h/2 - block_height/2;

                ui.main_title_y = element_y - 64;
                fullscreen = do_checkbox(GEN_ID, window_w/2 - 128, element_y, 256, 48, fullscreen, "Fullscreen", 0.35);

                element_y += 80;
                if(do_button(GEN_ID, window_w/2 - 128, element_y, 256, 48, "Back", 0.35)) {
                    *settings_state = SETTINGS_MAIN;
                    reset_ui_current_focus();
                }
            }
            ui_defocus();
            break;
        }
        default: {
            *settings_state = SETTINGS_MAIN;
            reset_ui_current_focus();
            break;
        }
    }
}

#undef UI_SRC_ID
