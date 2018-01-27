#ifndef PLAYER_CONTROLLER_H
#define PLAYER_CONTROLLER_H

enum {
    CONTROL_MOVE_UP,
    CONTROL_MOVE_LEFT,
    CONTROL_MOVE_DOWN,
    CONTROL_MOVE_RIGHT,
    MAX_CONTROL
};

struct PlayerController {
    i8 controls[MAX_CONTROL];
};

#endif
