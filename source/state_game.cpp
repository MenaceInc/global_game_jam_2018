#define UI_SRC_ID 0

#include "state.h"

#include "player_controller.cpp"
#include "camera.cpp"
#include "map.cpp"
#include "particle.cpp"
#include "light.cpp"

#define MAX_DRONES 8

enum {
    MENU_NONE,
    MENU_DRONE,
};

struct GameData {
    Map map;
    LightState lighting;
    Camera camera;
    PlayerController controller;
    ParticleGroup particles[MAX_PARTICLE];
    i16 target_entity_id,
        drone_count,
        drone_ids[MAX_DRONES];
    FBO map_render;
    i8 menu_state;
};

State init_game() {
    State s;
    s.type = STATE_GAME;
    s.memory = malloc(sizeof(GameData));

    GameData *g = (GameData *)s.memory;
    g->lighting = init_light_state();
    g->camera = init_camera(MAP_WIDTH*4, 0);
    for(i8 i = 0; i < MAX_PARTICLE; i++) {
        g->particles[i] = init_particle_group(i);
        request_particle_group_resources(g->particles + i);
    }
    g->target_entity_id = -1;
    g->drone_count = 0;
    for(i8 i = 0; i < MAX_DRONES; i++) {
        g->drone_ids[i] = -1;
    }
    g->map_render = init_fbo(CRT_W, CRT_H);
    g->menu_state = 0;

    request_shader(SHADER_LIGHTING);
    request_texture(TEX_SPRITES);
    request_sound(SOUND_EXPLODE_1);
    request_sound(SOUND_EXPLODE_2);
    request_sound(SOUND_HURT);

    return s;
}

void init_game_heavy() {
    GameData *g = (GameData *)state.memory;
    g->map = generate_map();
}

void clean_up_game(State *s) {
    GameData *g = (GameData *)s->memory;

    clean_up_fbo(&g->map_render);

    for(i8 i = 0; i < MAX_PARTICLE; i++) {
        unrequest_particle_group_resources(g->particles + i);
        clean_up_particle_group(g->particles + i);
    }

    unrequest_sound(SOUND_HURT);
    unrequest_sound(SOUND_EXPLODE_2);
    unrequest_sound(SOUND_EXPLODE_1);
    unrequest_texture(TEX_SPRITES);
    unrequest_shader(SHADER_LIGHTING);

    free(s->memory);
    s->memory = NULL;
    s->type = 0;
}

void update_game() {
    GameData *g = (GameData *)state.memory;

    begin_player_controller_update(&g->controller);
    update_player_controller_keyboard(&g->controller);

    if(key_control_pressed(KEY_CONTROL_SPAWN)) {
        g->menu_state = g->menu_state == MENU_DRONE ? 0 : MENU_DRONE;
        play_ui_hot_sound();
    }

    if(g->target_entity_id >= 0) {
        Entity *e = g->map.entities+g->target_entity_id;
        if(e->id >= 0) {
            if(g->controller.controls[CONTROL_MOVE_UP]) {
                e->y_vel -= 0.05;
            }
            if(g->controller.controls[CONTROL_MOVE_LEFT]) {
                e->x_vel -= 0.05;
                e->direction = LEFT;
            }
            if(g->controller.controls[CONTROL_MOVE_DOWN]) {
                e->y_vel += 0.05;
            }
            if(g->controller.controls[CONTROL_MOVE_RIGHT]) {
                e->direction = RIGHT;
                e->x_vel += 0.05;
            }
            if(g->controller.controls[CONTROL_SUICIDE]) {
                r32 e_center_x = e->x + e->w/2,
                    e_center_y = e->y + e->h/2;

                for(i16 i = (e_center_x - 96)/8; i < (e_center_x + 96)/8; i++) {
                    for(i16 j = (e_center_y - 96)/8; j < (e_center_y + 96)/8; j++) {
                        if(i >= 0 && i < MAP_WIDTH && j >= 0 && j < MAP_HEIGHT &&
                           distance2_32(i*8+4, j*8 + 4, e_center_x, e_center_y) <= 96*96) {
                            g->map.tiles[i][j] = 0;
                        }
                    }
                }

                for(i16 i = 0; i < 300; i++) {
                    r32 angle = random(0, 2*PI);
                    do_particle(g->particles + PARTICLE_FIRE, e->x + e->w/2, e->y + e->h/2, cos(angle)*random(2, 4), sin(angle)*random(2, 4));
                }
                for(i16 i = 0; i < 100; i++) {
                    r32 angle = random(-PI/2 - 0.2, -PI/2 + 0.2);
                    do_particle(g->particles + PARTICLE_SMOKE, e->x + e->w/2, e->y + e->h/2, cos(angle)*random(0.1, 1), sin(angle)*random(0.1, 1));
                }

                delete_entity(&g->map, g->target_entity_id);
                g->target_entity_id = -1;
                play_sound(&sounds[SOUND_EXPLODE_2], 1, random(0.8, 1.2), 0, AUDIO_ENTITY);
            }

            g->camera.target_x = e->x + e->w/2 - g->map_render.w/2;
            g->camera.target_y = e->y + e->h/2 - g->map_render.h/2;
        }
        else {
            g->target_entity_id = -1;
        }
    }

    for(i8 i = 0; i < g->drone_count; i++) {
        if(g->drone_ids[i] >= 0 && g->map.entities[g->drone_ids[i]].id < 0) {
            memmove(g->drone_ids+i, g->drone_ids+i+1, g->drone_count - i - 1);
            --g->drone_count;
        }
    }

    //do_light(&g->lighting, mouse_x + g->camera.x, mouse_y + g->camera.y, 512, 1.5, 1, 0.7, 0.4);

    update_camera(&g->camera);

    update_map(&g->map);

    //update_light_state(&g->lighting, &g->camera, g->map_render.w, g->map_render.h);
    clear_fbo(&g->map_render);
    bind_fbo(&g->map_render);
    {
        draw_map(&g->map, &g->camera, CRT_W, CRT_H);
        for(i8 i = 0; i < MAX_PARTICLE; i++) {
            update_particle_group(g->particles + i, &g->camera, 0, 0, &g->map);
        }
    }

    bind_fbo(&crt_render);
    {
        draw_scaled_fbo(&g->map_render, 0, 0, 0, CRT_W, CRT_H);

        if(g->target_entity_id >= 0) {
            Entity *e = g->map.entities + g->target_entity_id;
            draw_text(&fonts[FONT_BASE], 0, 1, 1, 1, 1, 32, 32, 0.2, "Rocket Drone");
            struct StatusMessage {
                const char *msg;
                r32 r, g, b;
            };
            StatusMessage status_msg;

            if(e->health > 0.66) {
                status_msg.msg = "Status: GOOD";
                status_msg.r = 0;
                status_msg.g = 1;
                status_msg.b = 0;
            }
            else if(e->health > 0.33) {
                status_msg.msg = "Status: DAMAGED";
                status_msg.r = 1;
                status_msg.g = 1;
                status_msg.b = 0;
            }
            else {
                status_msg.msg = "Status: CRITICAL";
                status_msg.r = 1;
                status_msg.g = 0.6;
                status_msg.b = 0;
            }

            draw_text(&fonts[FONT_BASE], 0, status_msg.r, status_msg.g, status_msg.b, 1, 32, 48, 0.2, status_msg.msg);
        }
        else {
            draw_text(&fonts[FONT_BASE], 0, 1, 0.6, 0.6, 1, 32, 32, 0.2, "ERROR: NO TARGET DRONE");
        }

        char active_drone_str[32] = { 0 };
        sprintf(active_drone_str, "Responding Drones: %i", g->drone_count);
        draw_text(&fonts[FONT_BASE], ALIGN_RIGHT, 1, 1, 1, 1, CRT_W-32, 32, 0.2, active_drone_str);

        switch(g->menu_state) {
            case MENU_DRONE: {
                draw_filled_rect(0, 0, 0, 0.5, 0, 0, CRT_W, CRT_H);
                ui_focus(0);
                {
                    r32 block_height = 3*32,
                        element_y = CRT_H/2 - block_height/2;

                    ui.main_title = "Spawn Drone";
                    ui.main_title_y = element_y - 48;

                    if(do_button(GEN_ID, CRT_W/2 - 96, element_y, 192, 32, "Rocket Drone", 0.3)) {
                        g->target_entity_id = add_entity(&g->map, init_rocket_drone(-1, MAP_WIDTH*4, 0));
                        g->drone_ids[g->drone_count++] = g->target_entity_id;
                        g->menu_state = 0;
                    }
                    element_y += 31;
                    if(do_button(GEN_ID, CRT_W/2 - 96, element_y, 192, 32, "Rocket Drone", 0.3)) {
                        g->target_entity_id = add_entity(&g->map, init_rocket_drone(-1, MAP_WIDTH*4, 0));
                        g->drone_ids[g->drone_count++] = g->target_entity_id;
                        g->menu_state = 0;
                    }
                    element_y += 31;
                    if(do_button(GEN_ID, CRT_W/2 - 96, element_y, 192, 32, "Rocket Drone", 0.3)) {
                        g->target_entity_id = add_entity(&g->map, init_rocket_drone(-1, MAP_WIDTH*4, 0));
                        g->drone_ids[g->drone_count++] = g->target_entity_id;
                        g->menu_state = 0;
                    }
                    element_y += 48;
                    if(do_button(GEN_ID, CRT_W/2 - 96, element_y, 192, 32, "Cancel", 0.3)) {
                        g->menu_state = 0;
                    }
                }
                ui_defocus();
                break;
            }
            default: break;
        }
    }
    bind_fbo(NULL);
}

#undef UI_SRC_ID
