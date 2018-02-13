#include "player_controller.h"

void begin_player_controller_update(PlayerController *c) {
    for(i8 i = 0; i < MAX_CONTROL; i++) {
        c->controls[i] = 0;
    }
}

void update_player_controller_keyboard(PlayerController *c) {
    c->controls[CONTROL_MOVE_UP] = key_control_down(KEY_CONTROL_MOVE_UP);
    c->controls[CONTROL_MOVE_LEFT] = key_control_down(KEY_CONTROL_MOVE_LEFT);
    c->controls[CONTROL_MOVE_DOWN] = key_control_down(KEY_CONTROL_MOVE_DOWN);
    c->controls[CONTROL_MOVE_RIGHT] = key_control_down(KEY_CONTROL_MOVE_RIGHT);
    c->controls[CONTROL_SUICIDE] = key_control_pressed(KEY_CONTROL_SUICIDE);
}
