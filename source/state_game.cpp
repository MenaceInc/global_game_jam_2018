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

enum {
    MATERIAL_COPPER,
    MATERIAL_STEEL,
    MATERIAL_GOLD,
    MATERIAL_PALLADIUM,
    MATERIAL_SILICON,
    MATERIAL_SULFUR,
    MATERIAL_DIAMOND,
    MATERIAL_ENERGINIUM,
    MATERIAL_UNOBTAINIUM,
    MAX_MATERIAL
};

struct GameState {
    i32 materials[MAX_RESOURCE];
    i8 drone_capacity;
};

struct GameData {
    Map map;
    LightState lighting[MAX_EXPLORER];
    Camera camera;
    PlayerController controller;
    ParticleGroup particles[MAX_PARTICLE];

    GameState game_state;
    i16 target_entity_id, vision_type,
        drone_ids[MAX_DRONES];
    FBO map_render;
    i8 menu_state;

    const char *error_msg;
    r32 error_t;
};

void game_error(GameData *g, const char *msg) {
    g->error_msg = msg;
    g->error_t = 1;
}

State init_game() {
    State s;
    s.type = STATE_GAME;
    s.memory = malloc(sizeof(GameData));

    GameData *g = (GameData *)s.memory;
    for(i8 i = 0; i < MAX_EXPLORER; i++) {
        g->lighting[i] = init_light_state();
    }

    g->lighting[EXPLORER_VIS].default_light = 0.7;
    g->lighting[EXPLORER_IR].default_light = 0.5;
    g->lighting[EXPLORER_EM].default_light = 0.5;

    g->camera = init_camera(MAP_WIDTH*4, 0);
    for(i8 i = 0; i < MAX_PARTICLE; i++) {
        g->particles[i] = init_particle_group(i);
        request_particle_group_resources(g->particles + i);
    }

    g->game_state.drone_capacity = 2;

    for(i16 i = 0; i < MAX_MATERIAL; i++) {
        g->game_state.materials[i] = 0;
    }

    g->target_entity_id = -1;
    g->vision_type = 0;
    for(i8 i = 0; i < MAX_DRONES; i++) {
        g->drone_ids[i] = -1;
    }
    g->map_render = init_fbo(CRT_W, CRT_H);
    g->menu_state = 0;

    g->error_msg = NULL;
    g->error_t = 0;

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

void do_explosion(r32 x, r32 y, r32 radius, GameData *g) {
    Map *m = &g->map;

    for(i16 i = (x - radius)/8; i < (x + radius)/8; i++) {
        for(i16 j = (y - radius)/8; j < (y + radius)/8; j++) {
            if(i >= 0 && i < MAP_WIDTH && j >= 0 && j < MAP_HEIGHT &&
               distance2_32(i*8+4, j*8 + 4, x, y) <= radius*radius) {
                g->map.tiles[i][j] = 0;
            }
        }
    }

    for(i16 i = 0; i < 300; i++) {
        r32 angle = random(0, 2*PI);
        do_particle(g->particles + PARTICLE_FIRE, x, y, cos(angle)*random(2, 4), sin(angle)*random(2, 4));
    }
    for(i16 i = 0; i < 100; i++) {
        r32 angle = random(-PI/2 - 0.2, -PI/2 + 0.2);
        do_particle(g->particles + PARTICLE_SMOKE, x, y, cos(angle)*random(0.1, 1), sin(angle)*random(0.1, 1));
    }
    play_sound(&sounds[SOUND_EXPLODE_2], 1, random(0.8, 1.2), 0, AUDIO_ENTITY);

    for(i16 i = 0; i < m->entity_count; i++) {
        Entity *e = m->entities + m->entity_ids[i];
        r32 distance2 = distance2_32(x, y, e->x + e->w/2, e->y + e->h/2);
        if(distance2 < radius*radius) {
            r32 power = (1-(distance2/(radius*radius))),
                velocity = power*6,
                angle = atan2((e->y + e->h/2) - y, (e->x + e->w/2) - x);
            e->health -= power;
            e->x_vel += velocity*cos(angle);
            e->y_vel += velocity*sin(angle);
        }
    }
}

void update_game() {
    GameData *g = (GameData *)state.memory;

    begin_player_controller_update(&g->controller);
    update_player_controller_keyboard(&g->controller);

    if(key_control_pressed(KEY_CONTROL_SPAWN)) {
        i8 i = 0;
        for(i = 0; i < g->game_state.drone_capacity; i++) {
            if(g->drone_ids[i] < 0) {
                g->menu_state = g->menu_state == MENU_DRONE ? 0 : MENU_DRONE;
                break;
            }
        }
        if(i == g->game_state.drone_capacity) {
            game_error(g, "ERROR: System can not support more drones");
        }
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

                do_explosion(e_center_x, e_center_y, 96, g);

                delete_entity(&g->map, g->target_entity_id);
                g->target_entity_id = -1;

            }

            g->camera.target_x = e->x + e->w/2 - g->map_render.w/2;
            g->camera.target_y = e->y + e->h/2 - g->map_render.h/2;
        }
        else {
            g->target_entity_id = -1;
            for(i8 i = 0; i < g->game_state.drone_capacity; i++) {
                if(g->drone_ids[i] >= 0) {
                    g->target_entity_id = g->drone_ids[i];
                    break;
                }
            }
        }
    }

    for(i8 i = 0; i < g->game_state.drone_capacity; ++i) {
        if(g->drone_ids[i] >= 0 && g->map.entities[g->drone_ids[i]].id < 0) {
            g->drone_ids[i] = -1;
        }
    }

    update_camera(&g->camera);

    for(i16 i = 0; i < g->map.entity_count;) {
        update_entity(&g->map, g->map.entities + g->map.entity_ids[i], g->lighting);
        if(g->map.entities[g->map.entity_ids[i]].health <= 0) {
            Entity *e = g->map.entities + g->map.entity_ids[i];
            do_explosion(e->x + e->w/2, e->y + e->h/2, 16, g);
            delete_entity(&g->map, g->map.entity_ids[i]);
        }
        else {
            ++i;
        }
    }

    clear_fbo(&g->map_render);
    bind_fbo(&g->map_render);
    {
        draw_map(&g->map, &g->camera, CRT_W, CRT_H);
        for(i8 i = 0; i < MAX_PARTICLE; i++) {
            update_particle_group(g->particles + i, &g->camera, 0, 0, &g->map);
        }
    }

    for(i8 i = 0; i < g->game_state.drone_capacity; i++) {
        if(g->drone_ids[i] == g->target_entity_id && g->drone_ids[i] >= 0) {
            Entity *e = g->map.entities + g->target_entity_id;
            if(e->type == ENTITY_EXPLORER_DRONE) {
                g->vision_type = e->explorer->type;
            }
        }
    }

    update_light_state(g->lighting + g->vision_type, &g->camera, CRT_W, CRT_H);

    bind_fbo(&crt_render);
    {
        active_shader = shaders[SHADER_LIGHTING].id;
        draw_scaled_fbo(&g->map_render, 0, 0, 0, CRT_W, CRT_H);
        active_shader = 0;

        if(g->target_entity_id >= 0) {

            Entity *e = g->map.entities + g->target_entity_id;
            char current_drone_str[32] = { 0 };
            i8 active_drone = -1;
            for(i8 i = 0; i < g->game_state.drone_capacity; i++) {
                if(g->drone_ids[i] == g->target_entity_id) {
                    active_drone = i;
                    break;
                }
            }
            if(active_drone >= 0) {
                sprintf(current_drone_str, "%i: %s", active_drone+1, explorer_data[e->explorer->type].name);
            }
            draw_text(&fonts[FONT_BASE], 0, 1, 1, 1, 1, 32, 32, 0.2, current_drone_str);

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

        {
            i8 active_drones = 0;
            for(i8 i = 0; i < g->game_state.drone_capacity; i++) {
                if(g->drone_ids[i] >= 0) {
                    ++active_drones;
                }
            }
            char active_drone_str[32] = { 0 };
            sprintf(active_drone_str, "Responding Drones: %i", active_drones);
            draw_text(&fonts[FONT_BASE], ALIGN_RIGHT, 1, 1, 1, 1, CRT_W-32, 32, 0.2, active_drone_str);
        }

        for(i8 i = 0; i < g->game_state.drone_capacity; i++) {
            if(g->drone_ids[i] >= 0) {
                if(key_pressed[KEY_1 + i]) {
                    g->target_entity_id = g->drone_ids[i];
                }

                Entity *e = &g->map.entities[g->drone_ids[i]];
                char drone_name_str[32] = { 0 };
                sprintf(drone_name_str, "%i. %s", i+1, explorer_data[e->explorer->type].name);
                draw_text(&fonts[FONT_BASE], ALIGN_RIGHT, 1, 1, 1, 1, CRT_W-32, 48+i*16, 0.2, drone_name_str);
            }
            else {
                draw_text(&fonts[FONT_BASE], ALIGN_RIGHT, 1, 0.7, 0.5, 1, CRT_W-32, 48+i*16, 0.2, "................");
            }
        }

        switch(g->menu_state) {
            case MENU_DRONE: {
                draw_filled_rect(0, 0, 0, 0.5, 0, 0, CRT_W, CRT_H);
                ui_focus(0);
                {
                    r32 block_height = 3*32,
                        element_y = CRT_H/2 - block_height/2;

                    ui.main_title = "Spawn Drone";
                    ui.main_title_y = element_y - 48;

                    if(do_button(GEN_ID, CRT_W/2 - 96, element_y, 192, 32, "Explorer", 0.3)) {
                        for(i8 i = 0; i < g->game_state.drone_capacity; i++) {
                            if(g->drone_ids[i] < 0) {
                                g->target_entity_id = add_entity(&g->map, init_explorer_drone(-1, MAP_WIDTH*4, 0, EXPLORER_VIS));
                                g->drone_ids[i] = g->target_entity_id;
                                break;
                            }
                        }
                        g->menu_state = 0;
                    }
                    element_y += 31;
                    if(do_button(GEN_ID, CRT_W/2 - 96, element_y, 192, 32, "Digger", 0.3)) {
                        //g->target_entity_id = add_entity(&g->map, init_rocket_drone(-1, MAP_WIDTH*4, 0));
                        //g->drone_ids[g->drone_count++] = g->target_entity_id;
                        g->menu_state = 0;
                    }
                    element_y += 31;
                    if(do_button(GEN_ID, CRT_W/2 - 96, element_y, 192, 32, "Fighter", 0.3)) {
                        //g->target_entity_id = add_entity(&g->map, init_rocket_drone(-1, MAP_WIDTH*4, 0));
                        //g->drone_ids[g->drone_count++] = g->target_entity_id;
                        g->menu_state = 0;
                    }
                    element_y += 48;
                    if(do_button(GEN_ID, CRT_W/2 - 96, element_y, 192, 32, "Cancel", 0.3)) {
                        g->menu_state = 0;
                    }
                }
                ui_defocus();

                for(i8 i = 0; i < MAX_MATERIAL; i++) {
                    draw_texture_region(&textures[TEX_SPRITES], 0, 24 + i*12, 72, 12, 12, CRT_W/2 - MAX_MATERIAL*20 + i*40 + 14, CRT_H - 32, 0);
                    char number_text[16] = { 0 };
                    sprintf(number_text, "x%i", g->game_state.materials[i]);
                    draw_text(&fonts[FONT_BASE], ALIGN_CENTER_X, 1, 1, 1, 1, CRT_W/2  - MAX_MATERIAL*20 + i*40 + 16, CRT_H - 44, 0.25, number_text);
                }

                break;
            }
            default: break;
        }

        g->error_t *= 0.992;
        if(g->error_t >= 0.001 && g->error_msg) {
            draw_text(&fonts[FONT_BASE], ALIGN_CENTER_X, 1*g->error_t, 0.5*g->error_t, 0.5*g->error_t, 1*g->error_t, CRT_W/2, CRT_H - 52, 0.3, g->error_msg);
        }
    }
    bind_fbo(NULL);
}

#undef UI_SRC_ID
