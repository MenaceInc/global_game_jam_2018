#include "state.h"

#include "map.cpp"
#include "camera.cpp"

struct GameData {
    Map map;
    Camera camera;
};

State init_game() {
    State s;
    s.type = STATE_GAME;
    s.memory = malloc(sizeof(GameData));

    GameData *g = (GameData *)s.memory;
    g->map = generate_map();
    g->camera = init_camera(0, 0);

    request_texture(TEX_SPRITES);

    return s;
}

void clean_up_game(State *s) {
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

    prepare_for_2d();
    {
        for(i16 i = (i16)g->camera.x/24; i < (i16)g->camera.x/24 + window_w/24 + 1; i++) {
            for(i16 j = (i16)g->camera.y/24; j < (i16)g->camera.y/24 + window_h/24 + 1; j++) {
                if(i >= 0 && i < MAP_WIDTH && j >= 0 && j < MAP_HEIGHT && g->map.tiles[i][j]) {
                    draw_scaled_texture_region(&textures[TEX_SPRITES], 0,
                                               tile_data[g->map.tiles[i][j]].tx, tile_data[g->map.tiles[i][j]].ty, 8, 8,
                                               i*24 - g->camera.x, j*24 - g->camera.y, 24, 24, 0);
                }
            }
        }
    }
}

