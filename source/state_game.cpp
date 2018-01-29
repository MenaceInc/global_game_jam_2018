#define UI_SRC_ID 0

#include "state.h"
#include "projectile.h"

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
    i32 materials[MAX_MATERIAL];
    i8 drone_capacity;
    i32 capacity_increase_requirement; // steel
};

#include "player_controller.cpp"
#include "camera.cpp"
#include "map.cpp"
#include "particle.cpp"
#include "light.cpp"

#define MAX_DRONES 8

enum {
    MENU_NONE,
    MENU_PAUSE,
    MENU_DRONE,
};

struct GameData {
    Map map;
    Projectile *projectiles;
    LightState lighting[MAX_EXPLORER];
    Camera camera;
    PlayerController controller;
    ParticleGroup particles[MAX_PARTICLE];

    GameState game_state;
    i16 target_entity_id, vision_type, enemy_spawn_wait,
        drone_ids[MAX_DRONES];
    FBO map_render;
    i8 menu_state, settings_state, selected_control;

    i8 selected_drone_type, selected_armor, selected_antenna, selected_unique_upgrade;

    const char *error_msg;
    r32 error_t;
    char **notifications;
    r32 *notifications_t;

    SoundSource *bg_music_sources[4];
    r32 bg_music_volumes[4];
};

void game_error(GameData *g, const char *msg) {
    g->error_msg = msg;
    g->error_t = 1;
}

void game_notification(GameData *g, char msg[64]) {
    char *str = (char *)calloc(strlen(msg)+1, 1);
    strcpy(str, msg);
    da_push(g->notifications, str);
    r32 t = 1;
    da_push(g->notifications_t, t);
    play_ui_hot_sound();
}

State init_game() {
    State s;
    s.type = STATE_GAME;
    s.memory = malloc(sizeof(GameData));

    GameData *g = (GameData *)s.memory;
    g->projectiles = NULL;

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

    g->game_state.drone_capacity = 3;
    g->game_state.capacity_increase_requirement = 1000;

    for(i16 i = 0; i < MAX_MATERIAL; i++) {
        g->game_state.materials[i] = 0;
    }
    g->game_state.materials[MATERIAL_STEEL] = 500;
    g->game_state.materials[MATERIAL_COPPER] = 100;

    g->target_entity_id = -1;
    g->vision_type = 0;
    g->enemy_spawn_wait = 600;
    for(i8 i = 0; i < MAX_DRONES; i++) {
        g->drone_ids[i] = -1;
    }
    g->map_render = init_fbo(CRT_W, CRT_H);
    g->menu_state = 0;
    g->settings_state = -1;
    g->selected_control = -1;

    g->selected_drone_type = 0;
    g->selected_armor = 0;
    g->selected_antenna = 0;
    g->selected_unique_upgrade = 0;

    g->error_msg = NULL;
    g->error_t = 0;

    g->notifications = NULL;
    g->notifications_t = NULL;

    request_shader(SHADER_LIGHTING);
    request_shader(SHADER_TILES);
    request_texture(TEX_SPRITES);
    request_sound(SOUND_EXPLODE_1);
    request_sound(SOUND_EXPLODE_2);
    request_sound(SOUND_HURT);

    request_sound(SOUND_UNKNOWN_WORLD);
    request_sound(SOUND_STONE);
    request_sound(SOUND_MAGMA);
    request_sound(SOUND_DANGER_ZONE);

    for(i8 i = 0; i < 4; i++) {
        g->bg_music_sources[i] = reserve_sound_source();
        g->bg_music_volumes[i] = 0;
    }

    return s;
}

void init_game_heavy() {
    GameData *g = (GameData *)state.memory;
    g->map = generate_map();
}

void clean_up_game(State *s) {
    GameData *g = (GameData *)s->memory;

    for(i8 i = 0; i < 4; i++) {
        unreserve_sound_source(g->bg_music_sources[i]);
    }

    foreach(i, da_size(g->notifications)) {
        free(g->notifications[i]);
    }
    da_free(g->notifications);
    da_free(g->notifications_t);

    da_free(g->projectiles);
    clean_up_map(&g->map);

    clean_up_fbo(&g->map_render);

    for(i8 i = 0; i < MAX_PARTICLE; i++) {
        unrequest_particle_group_resources(g->particles + i);
        clean_up_particle_group(g->particles + i);
    }

    unrequest_sound(SOUND_HURT);
    unrequest_sound(SOUND_EXPLODE_2);
    unrequest_sound(SOUND_EXPLODE_1);
    unrequest_texture(TEX_SPRITES);
    unrequest_shader(SHADER_TILES);
    unrequest_shader(SHADER_LIGHTING);

    free(s->memory);
    s->memory = NULL;
    s->type = 0;
}

void mine(r32 x, r32 y, r32 radius, Map *m, GameState *g, i32 yield) {
    i16 destroyed_tiles = 0;

    for(i16 i = (x - radius)/8; i < (x + radius)/8; i++) {
        for(i16 j = (y - radius)/8; j < (y + radius)/8; j++) {
            if(i >= 0 && i < MAP_WIDTH && j >= 0 && j < MAP_HEIGHT &&
               distance2_32(i*8+4, j*8 + 4, x, y) <= radius*radius &&
               m->tiles[i][j]) {

                if(g && tile_data[m->tiles[i][j]].retrieved_material >= 0) {
                    g->materials[tile_data[m->tiles[i][j]].retrieved_material] += yield;
                }
                m->tiles[i][j] = 0;
                ++destroyed_tiles;
            }
        }
    }

    if(destroyed_tiles) {
        play_sound_at_point(&sounds[SOUND_EXPLODE_1], x, y, 0.05, random(0.8, 1.2), 0, AUDIO_ENTITY);
    }
}

void shoot(i16 source_id, r32 x, r32 y, r32 target_x, r32 target_y, r32 strength, i32 tiles_left, Projectile **projectiles) {
    r32 angle = atan2(target_y - y, target_x - x);
    Projectile p = { source_id, tiles_left, x, y, cos(angle)*8, sin(angle)*8, strength };
    da_push(*projectiles, p);
}

void do_explosion(i8 harvest, r32 x, r32 y, r32 radius, GameData *g) {
    Map *m = &g->map;

    for(i16 i = (x - radius)/8; i < (x + radius)/8; i++) {
        for(i16 j = (y - radius)/8; j < (y + radius)/8; j++) {
            if(i >= 0 && i < MAP_WIDTH && j >= 0 && j < MAP_HEIGHT &&
               distance2_32(i*8+4, j*8 + 4, x, y) <= radius*radius) {
                if(tile_data[g->map.tiles[i][j]].retrieved_material >= 0) {
                    g->game_state.materials[tile_data[g->map.tiles[i][j]].retrieved_material] += 10;
                }
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
    play_sound_at_point(&sounds[SOUND_EXPLODE_2], x, y, radius / 256, random(0.8, 1.2), 0, AUDIO_ENTITY);

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

    /*
    if(key_pressed[KEY_Q]) {
        add_entity(&g->map, init_brain_alien(-1, g->camera.x, g->camera.y));
    }
    */

    {
        r32 center_y = g->camera.y + CRT_H/2;
        i8 bg_song = 0;

        for(i16 i = 0; i < g->map.entity_count; i++) {
            Entity *e = g->map.entities + g->map.entity_ids[i];
            if(e->id >= 0 && e->type == ENTITY_BRAIN_ALIEN && e->brain->target_entity >= 0) {
                bg_song = 3;
            }
        }

        if(!bg_song) {
            for(i8 i = 0; i < 3; i++) {
                if(center_y <= (i+1)*(MAP_HEIGHT/3)*8) {
                    bg_song = i;
                    break;
                }
            }
        }

        for(i8 i = 0; i < 4; i++) {
            if(bg_song == i && state_t < 0.05) {
                if(source_playing(g->bg_music_sources[i])) {
                    set_source_volume(g->bg_music_sources[i], (1 - get_source_volume(g->bg_music_sources[i])) * 0.05);
                }
                else {
                    play_source(g->bg_music_sources[i], &sounds[SOUND_UNKNOWN_WORLD+i], 0, 1, 1, AUDIO_MUSIC);
                }
            }
            else {
                set_source_volume(g->bg_music_sources[i], get_source_volume(g->bg_music_sources[i]) * 0.98);
            }
        }
    }

    GameState last_game_state = g->game_state;

    begin_player_controller_update(&g->controller);
    update_player_controller_keyboard(&g->controller);

    if(key_control_pressed(KEY_CONTROL_SPAWN)) {
        g->menu_state = g->menu_state == MENU_DRONE ? 0 : MENU_DRONE;
        g->selected_drone_type = -1;
        play_ui_hot_sound();
    }
    else if(key_pressed[KEY_ESCAPE]) {
        g->menu_state = g->menu_state == MENU_PAUSE ? 0 : MENU_PAUSE;
        play_ui_hot_sound();
    }

    if(g->target_entity_id >= 0) {
        if(!--g->enemy_spawn_wait) {
            i16 count = 0;
            for(i32 i = 0; i < g->map.entity_count; i++) {
                if(g->map.entities[g->map.entity_ids[i]].type == ENTITY_BRAIN_ALIEN &&
                   g->map.entities[g->map.entity_ids[i]].id >= 0) {
                    ++count;
                }
            }

            if(count < 50) {
                i8 added = 0;
                for(i16 i = (i16)(g->camera.x - 128)/8; i < (i16)(g->camera.x + 128)/8 + CRT_W/8 + 1; i++) {
                    for(i16 j = (i16)(g->camera.y - 128)/8; j < (i16)(g->camera.y + 128)/8 + CRT_H/8 + 1; j++) {
                        if((i < (g->camera.x)/8 || i > (i16)(g->camera.x)/8 + CRT_W/8 + 1) &&
                           (j < (g->camera.y)/8 || j > (i16)(g->camera.y)/8 + CRT_H/8 + 1)) {
                            add_entity(&g->map, init_brain_alien(-1, i*8, j*8));
                            mine(i*8, j*8, 40, &g->map, NULL, 0);
                            added = 1;
                            break;
                        }
                    }
                    if(added) { break; }
                }
            }
            g->enemy_spawn_wait = random(1000, 6000);
        }

        Entity *e = g->map.entities+g->target_entity_id;
        if(e->id >= 0) {
            if(e->type == ENTITY_EXPLORER_DRONE) {
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
            }
            if(g->controller.controls[CONTROL_SUICIDE]) {
                r32 e_center_x = e->x + e->w/2,
                    e_center_y = e->y + e->h/2;

                do_explosion(1, e_center_x, e_center_y, 96, g);

                delete_entity(&g->map, g->target_entity_id);
                g->target_entity_id = -1;

                for(i8 i = 0; i < g->game_state.drone_capacity; i++) {
                    if(g->drone_ids[i] >= 0) {
                        g->target_entity_id = g->drone_ids[i];
                        break;
                    }
                }
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
    set_listener_position(g->camera.x, g->camera.y, 0);

    for(i16 i = 0; i < g->map.entity_count;) {
        Entity *e = g->map.entities + g->map.entity_ids[i];
        update_entity(&g->map, &g->game_state, &g->projectiles, e, g->lighting);
        if(e->health <= 0) {
            do_explosion(0, e->x + e->w/2, e->y + e->h/2, 16, g);
            delete_entity(&g->map, e->id);
        }
        else {
            if(e->type != ENTITY_BRAIN_ALIEN) {
                for(i16 j = 0; j < g->map.entity_count; j++) {
                    Entity *ent = g->map.entities + g->map.entity_ids[j];
                    if(ent->type == ENTITY_BRAIN_ALIEN) {
                        if(ent->x + ent->w >= e->x && ent->x <= e->x + e->w &&
                           ent->y + ent->h >= e->y && ent->y <= e->y + e->h) {
                            r32 angle = atan2(e->y - ent->y, e->x - ent->x);
                            e->x_vel += 3*cos(angle);
                            e->y_vel += 3*sin(angle);
                            hurt_entity(e, 0.2);
                        }
                    }
                }

                if(e->health < 0.33) {
                    for(i8 j = 0; j < 3; ++j) {
                        do_particle(g->particles + PARTICLE_SMOKE, e->x + e->w/2, e->y + e->h/2, random(-1, 1), -3);
                        do_particle(g->particles + PARTICLE_FIRE, e->x + e->w/2, e->y + e->h/2, random(-1, 1), -3);
                    }
                }
                else if(e->health < 0.66) {
                    do_particle(g->particles + PARTICLE_SMOKE, e->x + e->w/2, e->y + e->h/2, random(-1, 1), -3);
                }
            }
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
    for(i8 i = 0; i < MAX_EXPLORER; i++) {
        if(i != g->vision_type) {
            g->lighting[i].light_count = 0;
        }
    }

    bind_fbo(&crt_render);
    {
        active_shader = shaders[SHADER_LIGHTING].id;
        draw_scaled_fbo(&g->map_render, 0, 0, 0, CRT_W, CRT_H);
        active_shader = 0;
        for(u32 i = 0; i < da_size(g->projectiles);) {
            g->projectiles[i].x += g->projectiles[i].x_vel;
            g->projectiles[i].y += g->projectiles[i].y_vel;
            draw_filled_rect(1, 0, 0, 1, g->projectiles[i].x - 1 - g->camera.x, g->projectiles[i].y - 1 - g->camera.y, 2, 2);
            i16 x = (i16)g->projectiles[i].x / 8,
                y = (i16)g->projectiles[i].y / 8;

            if(x >= 0 && x < MAP_WIDTH && y >= 0 && y < MAP_WIDTH &&
               g->map.tiles[x][y]) {
                mine(x*8, y*8, 16, &g->map, NULL, 0);
                if(!--g->projectiles[i].tiles_left) {
                    da_erase(g->projectiles, i);
                    continue;
                }
            }
            ++i;
        }

        if(g->target_entity_id >= 0) {
            Entity *e = g->map.entities + g->target_entity_id;
            char current_drone_str[32] = { 0 };
            i8 active_drone = -1;
            for(i8 i = 0; i < g->game_state.drone_capacity; i++) {
                if(g->drone_ids[i] == g->target_entity_id && g->drone_ids[i] >= 0) {
                    active_drone = i;
                    break;
                }
            }
            if(active_drone >= 0) {
                if(e->type == ENTITY_EXPLORER_DRONE) {
                    sprintf(current_drone_str, "%i: %s", active_drone+1, explorer_data[e->explorer->type].name);
                }
                else if(e->type == ENTITY_DIGGER_DRONE) {
                    sprintf(current_drone_str, "%i: Digger", active_drone+1);
                }
                else if(e->type == ENTITY_FIGHTER_DRONE) {
                    sprintf(current_drone_str, "%i: Fighter", active_drone+1);
                }
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

        /* DRAW NOTIFICATIONS */
        {
            r32 notification_y = 64;
            for(i32 i = (i32)da_size(g->notifications) - 1; i >= 0 && i < (i32)da_size(g->notifications); i--) {
                g->notifications_t[i] *= 0.98;
                draw_text(&fonts[FONT_BASE], 0, 0.8, 1, 0.7, 1, 32, notification_y, 0.2, g->notifications[i]);
                notification_y += 16;
                if(g->notifications_t[i] < 0.005) {
                    free(g->notifications[i]);
                    da_erase(g->notifications, i);
                    da_erase(g->notifications_t, i);
                }
            }
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
                if(e->type == ENTITY_EXPLORER_DRONE) {
                    sprintf(drone_name_str, "%i: %s", i+1, explorer_data[e->explorer->type].name);
                }
                else if(e->type == ENTITY_DIGGER_DRONE) {
                    sprintf(drone_name_str, "%i: %s", i+1, "Digger");
                }
                else if(e->type == ENTITY_FIGHTER_DRONE) {
                    sprintf(drone_name_str, "%i: %s", i+1, "Fighter");
                }
                draw_text(&fonts[FONT_BASE], ALIGN_RIGHT, 1, 1, 1, 1, CRT_W-32, 48+i*16, 0.2, drone_name_str);
            }
            else {
                draw_text(&fonts[FONT_BASE], ALIGN_RIGHT, 1, 0.7, 0.5, 1, CRT_W-32, 48+i*16, 0.2, "................");
            }
        }

        if(g->menu_state) {
            draw_filled_rect(0, 0, 0, 0.5, 0, 0, CRT_W, CRT_H);
            switch(g->menu_state) {
                case MENU_DRONE: {
                    r32 spawn_x = MAP_WIDTH*4,
                        spawn_y = -64;

                    if(g->target_entity_id >= 0) {
                        spawn_x = g->map.entities[g->target_entity_id].x;
                        spawn_y = g->map.entities[g->target_entity_id].y;
                    }

                    if(g->selected_drone_type >= 0) {
                        r32 block_height = MAX_ARMOR*32 + 16 + 32*3,
                            element_y = CRT_H/2 - block_height/2,
                            initial_element_y = element_y;

                        ui.main_title = "Upgrade Drone";
                        ui.main_title_y = element_y - 48;

                        if(ui_left_press || ui_right_press) {
                            ui.current_focus_group = ui.current_focus_group == 1 ? 2 : 1;
                            play_ui_hot_sound();
                        }
                        else if(ui.current_focus_group != 1 && ui.current_focus_group != 2) {
                            ui.current_focus_group = 1;
                        }

                        ui_focus(1);
                        {
                            for(i8 i = 0; i < MAX_ARMOR; i++) {
                                if(do_radio(GEN_ID + (i/100.f), CRT_W/2 - 200, element_y, 192, 32, g->selected_armor == i, armor_data[i].name, 0.25)) {
                                    g->selected_armor = i;
                                }
                                element_y += 31;
                            }
                            element_y += 16;
                        }
                        ui_defocus();

                        r32 last_element_y = element_y;
                        element_y = initial_element_y;
                        ui_focus(2);
                        {
                            for(i8 i = 0; i < MAX_ANTENNA; i++) {
                                if(do_radio(GEN_ID + (i/100.f), CRT_W/2 + 8, element_y, 192, 32, g->selected_antenna == i, antenna_data[i].name, 0.25)) {
                                    g->selected_antenna = i;
                                }
                                element_y += 31;
                            }
                            element_y += 16;
                        }
                        ui_defocus();
                        element_y = last_element_y;

                        ui_focus(1);
                        {
                            switch(g->selected_drone_type) {
                                case DRONE_EXPLORER: {
                                    if(do_radio(GEN_ID, CRT_W/2 - 200, element_y, 192, 32, g->selected_unique_upgrade == EXPLORER_VIS, "Light Sensor", 0.25)) {
                                        g->selected_unique_upgrade = EXPLORER_VIS;
                                    }
                                    element_y += 31;
                                    if(do_radio(GEN_ID, CRT_W/2 - 200, element_y, 192, 32, g->selected_unique_upgrade == EXPLORER_IR, "Infrared Sensor", 0.25)) {
                                        g->selected_unique_upgrade = EXPLORER_IR;
                                    }
                                    element_y += 31;
                                    if(do_radio(GEN_ID, CRT_W/2 - 200, element_y, 192, 32, g->selected_unique_upgrade == EXPLORER_EM, "Electric Sensor", 0.25)) {
                                        g->selected_unique_upgrade = EXPLORER_EM;
                                    }
                                    break;
                                }
                                case DRONE_DIGGER: {
                                    if(do_radio(GEN_ID, CRT_W/2 - 200, element_y, 192, 32, g->selected_unique_upgrade == DRILL_STEEL, "Steel Drill", 0.25)) {
                                        g->selected_unique_upgrade = DRILL_STEEL;
                                    }
                                    element_y += 31;
                                    if(do_radio(GEN_ID, CRT_W/2 - 200, element_y, 192, 32, g->selected_unique_upgrade == DRILL_DIAMOND, "Diamond Drill", 0.25)) {
                                        g->selected_unique_upgrade = DRILL_DIAMOND;
                                    }
                                    element_y += 31;
                                    if(do_radio(GEN_ID, CRT_W/2 - 200, element_y, 192, 32, g->selected_unique_upgrade == DRILL_GIGA, "Giga Drill", 0.25)) {
                                        g->selected_unique_upgrade = DRILL_GIGA;
                                    }
                                    break;
                                }
                                case DRONE_FIGHTER: {
                                    if(do_radio(GEN_ID, CRT_W/2 - 200, element_y, 192, 32, g->selected_unique_upgrade == WEAPON_GUN, "Gun", 0.25)) {
                                        g->selected_unique_upgrade = WEAPON_GUN;
                                    }
                                    element_y += 31;
                                    if(do_radio(GEN_ID, CRT_W/2 - 200, element_y, 192, 32, g->selected_unique_upgrade == WEAPON_LASER, "Laser", 0.25)) {
                                        g->selected_unique_upgrade = WEAPON_LASER;
                                    }
                                    element_y += 31;
                                    if(do_radio(GEN_ID, CRT_W/2 - 200, element_y, 192, 32, g->selected_unique_upgrade == WEAPON_BHG, "BHG", 0.25)) {
                                        g->selected_unique_upgrade = WEAPON_BHG;
                                    }
                                    break;
                                }
                                default: break;
                            }

                            element_y += 48;
                        }
                        ui_defocus();

                        /* DRAW COST */
                        i32 material_costs[MAX_MATERIAL] = { 0 };
                        {
                            material_costs[MATERIAL_STEEL] = 50;

                            if(g->selected_armor == ARMOR_STEEL) {
                                material_costs[MATERIAL_STEEL] += 50;
                            }
                            else if(g->selected_armor == ARMOR_REACTIVE) {
                                material_costs[MATERIAL_STEEL] += 80;
                                material_costs[MATERIAL_SILICON] += 30;
                                material_costs[MATERIAL_COPPER] += 15;
                            }
                            else if(g->selected_armor == ARMOR_ENERGY) {
                                material_costs[MATERIAL_ENERGINIUM] += 100;
                                material_costs[MATERIAL_SILICON] += 50;
                            }

                            if(g->selected_antenna == ANTENNA_STANDARD) {
                                material_costs[MATERIAL_COPPER] += 5;
                                material_costs[MATERIAL_STEEL] += 5;
                            }
                            else if(g->selected_antenna == ANTENNA_HIGH_BAND) {
                                material_costs[MATERIAL_STEEL] += 100;
                                material_costs[MATERIAL_SILICON] += 30;

                            }
                            else if(g->selected_antenna == ANTENNA_LOW_BAND) {
                                material_costs[MATERIAL_GOLD] += 30;
                                material_costs[MATERIAL_ENERGINIUM] += 10;
                                material_costs[MATERIAL_STEEL] += 100;
                            }

                            switch(g->selected_drone_type) {
                                case DRONE_EXPLORER: {
                                    if(g->selected_unique_upgrade == EXPLORER_VIS) {
                                        material_costs[MATERIAL_STEEL] += 10;
                                    }
                                    else if(g->selected_unique_upgrade == EXPLORER_EM) {
                                        material_costs[MATERIAL_STEEL] += 15;
                                        material_costs[MATERIAL_GOLD] += 10;
                                    }
                                    else if(g->selected_unique_upgrade == EXPLORER_IR) {
                                        material_costs[MATERIAL_STEEL] += 15;
                                        material_costs[MATERIAL_PALLADIUM] += 10;
                                    }
                                    break;
                                }
                                case DRONE_DIGGER: {
                                    if(g->selected_unique_upgrade == DRILL_STEEL) {
                                        material_costs[MATERIAL_STEEL] += 30;
                                        material_costs[MATERIAL_COPPER] += 10;
                                    }
                                    else if(g->selected_unique_upgrade == DRILL_DIAMOND) {
                                        material_costs[MATERIAL_STEEL] += 30;
                                        material_costs[MATERIAL_COPPER] += 10;
                                        material_costs[MATERIAL_DIAMOND] += 3;
                                    }
                                    else if(g->selected_unique_upgrade == DRILL_GIGA) {
                                        material_costs[MATERIAL_UNOBTAINIUM] += 100;
                                        material_costs[MATERIAL_COPPER] += 10;
                                        material_costs[MATERIAL_ENERGINIUM] += 75;
                                        material_costs[MATERIAL_SILICON] += 50;
                                    }
                                    break;
                                }
                                case DRONE_FIGHTER: {
                                    if(g->selected_unique_upgrade == WEAPON_GUN) {
                                        material_costs[MATERIAL_STEEL] += 30;
                                    }
                                    else if(g->selected_unique_upgrade == WEAPON_LASER) {
                                        material_costs[MATERIAL_STEEL] += 20;
                                        material_costs[MATERIAL_SILICON] += 10;
                                        material_costs[MATERIAL_ENERGINIUM] += 10;
                                    }
                                    else if(g->selected_unique_upgrade == WEAPON_BHG) {
                                        material_costs[MATERIAL_SULFUR] += 50;
                                        material_costs[MATERIAL_ENERGINIUM] += 100;
                                    }
                                    break;
                                }
                                default: break;
                            }

                            draw_ui_rect(CRT_W/2 + 8, last_element_y, 192, 95);
                            r32 icon_x = 0, icon_y = 0;
                            for(i16 i = 0; i < MAX_MATERIAL; i++) {
                                if(material_costs[i]) {
                                    draw_texture_region(&textures[TEX_SPRITES], 0, 24+i*12, 72, 12, 12, CRT_W/2 + 16 + icon_x, last_element_y + 8 + icon_y, 0);
                                    char price_str[32] = { 0 };
                                    sprintf(price_str, "x%i", material_costs[i]);
                                    draw_text(&fonts[FONT_BASE], 0, 1, 1, 1, 1, CRT_W/2 + 28 + icon_x, last_element_y + 8 + icon_y, 0.25, price_str);
                                    icon_y += 16;
                                    if(icon_y >= 95) {
                                        icon_y = 0;
                                        icon_x += 32;
                                    }
                                }
                            }
                        }

                        ui_focus(1);
                        {
                            if(do_button(GEN_ID, CRT_W/2 - 136, element_y, 128, 32, "Cancel", 0.3)) {
                                g->menu_state = 0;
                            }
                        }
                        ui_defocus();

                        ui_focus(2);
                        {
                            if(do_button(GEN_ID, CRT_W/2 + 8, element_y, 128, 32, "Build", 0.3)) {
                                g->menu_state = 0;
                                i8 can_build = 1;
                                for(i16 i = 0; i < MAX_MATERIAL; i++) {
                                    if(material_costs[i] > g->game_state.materials[i]) {
                                        can_build = 0;
                                        break;
                                    }
                                }

                                if(can_build) {
                                    for(i16 i = 0; i < MAX_MATERIAL; i++) {
                                        g->game_state.materials[i] -= material_costs[i];
                                    }

                                    Entity ent;
                                    switch(g->selected_drone_type) {
                                        case DRONE_EXPLORER: {
                                            ent = init_explorer_drone(-1, spawn_x, spawn_y, g->selected_armor, g->selected_antenna, g->selected_unique_upgrade);
                                            break;
                                        }
                                        case DRONE_DIGGER: {
                                            ent = init_digger_drone(-1, spawn_x, spawn_y, g->selected_armor, g->selected_antenna, g->selected_unique_upgrade);
                                            break;
                                        }
                                        case DRONE_FIGHTER: {
                                            ent = init_fighter_drone(-1, spawn_x, spawn_y, g->selected_armor, g->selected_antenna, g->selected_unique_upgrade);
                                            break;
                                        }
                                        default: break;
                                    }

                                    i16 new_id = add_entity(&g->map, ent);

                                    if(g->target_entity_id < 0) {
                                        g->target_entity_id = new_id;
                                    }

                                    for(i8 i = 0; i < g->game_state.drone_capacity; i++) {
                                        if(g->drone_ids[i] < 0) {
                                            g->drone_ids[i] = new_id;
                                            break;
                                        }
                                    }
                                }
                                else {
                                    game_error(g, "ERROR: Insufficient resources");
                                }
                            }
                        }
                        ui_defocus();
                    }
                    else {
                        r32 block_height = MAX_DRONE*32,
                            element_y = CRT_H/2 - block_height/2;

                        ui.main_title = "Spawn Drone";
                        ui.main_title_y = element_y - 48;

                        ui_focus(0);
                        {
                            for(i8 i = 0; i < MAX_DRONE; i++) {
                                if(do_button(GEN_ID + i/100.f, CRT_W/2 - 96, element_y, 192, 32, drone_data[i].name, 0.3)) {
                                    for(i8 j = 0; j < g->game_state.drone_capacity; j++) {
                                        if(g->drone_ids[j] < 0) {
                                            g->selected_drone_type = i;
                                            g->selected_antenna = 0;
                                            g->selected_armor = 0;
                                            g->selected_unique_upgrade = 0;
                                            break;
                                        }
                                        else if(j == g->game_state.drone_capacity - 1) {
                                            game_error(g, "ERROR: System can not support more drones");
                                            g->menu_state = 0;
                                            break;
                                        }
                                    }
                                }
                                element_y += 31;
                            }

                            element_y += 16;
                            if(do_button(GEN_ID, CRT_W/2 - 64, element_y, 128, 32, "Cancel", 0.3)) {
                                g->menu_state = 0;
                            }
                        }
                        ui_defocus();
                    }

                    for(i8 i = 0; i < MAX_MATERIAL; i++) {
                        draw_texture_region(&textures[TEX_SPRITES], 0, 24 + i*12, 72, 12, 12, CRT_W/2 - MAX_MATERIAL*25 + i*50 + 14, CRT_H - 24, 0);
                        char number_text[16] = { 0 };
                        sprintf(number_text, "x%i", g->game_state.materials[i]);
                        draw_text(&fonts[FONT_BASE], ALIGN_CENTER_X, 1, 1, 1, 1, CRT_W/2  - MAX_MATERIAL*25 + i*50 + 16, CRT_H - 36, 0.25, number_text);
                    }

                    break;
                }
                case MENU_PAUSE: {
                    if(g->settings_state >= 0) {
                        do_settings_menu(&g->settings_state, &g->selected_control);
                    }
                    else {
                        ui_focus(0);
                        {
                            r32 block_height = 32*3,
                                element_y = CRT_H/2 - block_height/2;

                            if(do_button(GEN_ID, CRT_W/2 - 64, element_y, 128, 32, "Resume", 0.3)) {
                                g->menu_state = 0;
                            }
                            element_y += 31;
                            if(do_button(GEN_ID, CRT_W/2 - 64, element_y, 128, 32, "Settings", 0.3)) {
                                g->settings_state = 0;
                                reset_ui_current_focus();
                            }
                            element_y += 31;
                            if(do_button(GEN_ID, CRT_W/2 - 64, element_y, 128, 32, "Quit", 0.3)) {
                                next_state = init_title();
                                reset_ui_current_focus();
                            }
                        }
                        ui_defocus();
                    }

                    break;
                }
                default: break;
            }
        }
        else {
            g->error_t *= 0.992;
            if(g->error_t >= 0.001 && g->error_msg) {
                draw_text(&fonts[FONT_BASE], ALIGN_CENTER_X, 1*g->error_t, 0.5*g->error_t, 0.5*g->error_t, 1*g->error_t, CRT_W/2, CRT_H - 52, 0.3, g->error_msg);
            }

            i8 has_drones = 0;
            for(i8 i = 0; i < g->game_state.drone_capacity; i++) {
                if(g->drone_ids[i] >= 0) {
                    has_drones = 1;
                    break;
                }
            }

            if(!has_drones) {
                char spawn_prompt_msg[32] = { 0 };
                sprintf(spawn_prompt_msg, "Press %s to spawn a drone", key_name(key_control_maps[KEY_CONTROL_SPAWN]));
                draw_text(&fonts[FONT_BASE], ALIGN_CENTER_X | ALIGN_CENTER_Y, 1, 1, 1, 1, CRT_W/2, CRT_H/2, 0.3, spawn_prompt_msg);
            }
        }
    }
    bind_fbo(NULL);

    if(g->game_state.drone_capacity < MAX_DRONES &&
       g->game_state.materials[MATERIAL_STEEL] >= g->game_state.capacity_increase_requirement) {
        char msg[64] = { 0 };
        sprintf(msg, "DRONE CAPACITY INCREASED");
        game_notification(g, msg);
        ++g->game_state.drone_capacity;
        g->game_state.capacity_increase_requirement *= 1.5;
    }
}

#undef UI_SRC_ID
