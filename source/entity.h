#ifndef ENTITY_H
#define ENTITY_H

enum {
    ENTITY_EXPLORER_DRONE,
    ENTITY_DIGGER_DRONE,
    ENTITY_FIGHTER_DRONE,
    MAX_ENTITY
};

struct ExplorerDrone;
struct DiggerDrone;
struct FighterDrone;

struct Entity {
    i8 direction;
    i16 id, type;
    r32 x, y, w, h,
        x_vel, y_vel,

        health,
        defense;

    union {
        ExplorerDrone *explorer;
        DiggerDrone *digger;
        FighterDrone *fighter;
        void *data;
    };
};

struct GameState;
struct Map;
void mine(r32 x, r32 y, r32 radius, Map *m, GameState *g);

#endif
