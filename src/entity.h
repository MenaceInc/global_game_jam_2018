#ifndef ENTITY_H
#define ENTITY_H

enum {
    ENTITY_EXPLORER_DRONE,
    ENTITY_DIGGER_DRONE,
    ENTITY_FIGHTER_DRONE,
    ENTITY_BRAIN_ALIEN,
    MAX_ENTITY
};

struct ExplorerDrone;
struct DiggerDrone;
struct FighterDrone;
struct BrainAlien;

struct Entity {
    i8 direction;
    i16 id, type;
    r32 x, y, w, h,
        x_vel, y_vel,

        health,
        hurt_cooldown,
        defense;

    union {
        ExplorerDrone *explorer;
        DiggerDrone *digger;
        FighterDrone *fighter;
        BrainAlien *brain;
        void *data;
    };
};

struct GameState;
struct Projectiles;
struct Map;
void mine(r32 x, r32 y, r32 radius, Map *m, GameState *g, i32 yield);
void shoot(i16 source_id, r32 x, r32 y, r32 target_x, r32 target_y, r32 strength, i32 tiles_left, Projectile **projectiles);

#endif
