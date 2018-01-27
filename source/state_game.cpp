#include "state.h"

#include "map.cpp"
#include "camera.cpp"

struct GameData {
    Map map;
    Camera camera;
    FBO map_render;
};

State init_game() {
    State s;
    s.type = STATE_GAME;
    s.memory = malloc(sizeof(GameData));

    GameData *g = (GameData *)s.memory;
    g->map = generate_map();
    g->camera = init_camera(0, 0);
    g->map_render = init_fbo(480, 360);

    request_texture(TEX_SPRITES);

    return s;
}

void clean_up_game(State *s) {
    GameData *g = (GameData *)s->memory;

    clean_up_fbo(&g->map_render);

    unrequest_texture(TEX_SPRITES);

    free(s->memory);
    s->memory = NULL;
    s->type = 0;
}

void update_game() {
    GameData *g = (GameData *)state.memory;

    update_camera(&g->camera);

    if(key_down[KEY_W]) {
        g->camera.target_y -= 10;
    }
    if(key_down[KEY_S]) {
        g->camera.target_y += 10;
    }
    if(key_down[KEY_A]) {
        g->camera.target_x -= 10;
    }
    if(key_down[KEY_D]) {
        g->camera.target_x += 10;
    }

    clear_fbo(&g->map_render);
    bind_fbo(&g->map_render);
    {
        for(i16 i = (i16)g->camera.x/8; i < (i16)g->camera.x/8 + g->map_render.w/8 + 1; i++) {
            for(i16 j = (i16)g->camera.y/8; j < (i16)g->camera.y/8 + g->map_render.h/8 + 1; j++) {
                if(i >= 0 && i < MAP_WIDTH && j >= 0 && j < MAP_HEIGHT && g->map.tiles[i][j]) {
                    draw_texture_region(&textures[TEX_SPRITES], 0,
                                        tile_data[g->map.tiles[i][j]].tx, tile_data[g->map.tiles[i][j]].ty, 8, 8,
                                        i*8 - g->camera.x, j*8 - g->camera.y, 0);
                }
            }
        }
    }
    bind_fbo(NULL);
    draw_scaled_fbo(&g->map_render, 0, 0, 0, window_w, window_w*0.75);
}

