#include "entity.h"
#include "light.h"

struct BrainAlien {
    i16 target_entity;
};

Entity init_brain_alien(i16 id, r32 x, r32 y) {
    Entity e;
    e.direction = 0;
    e.id = id;
    e.type = ENTITY_BRAIN_ALIEN;
    e.x = x;
    e.y = y;
    e.w = 36;
    e.h = 36;
    e.x_vel = 0;
    e.y_vel = 0;
    e.health = 1;
    e.defense = 0.8;
    e.data = malloc(sizeof(BrainAlien));
    e.brain->target_entity = -1;
    return e;
}

void clean_up_brain_alien(Entity *e) {
    free(e->data);
}

void update_brain_alien(Entity *e, Map *m, LightState lighting[MAX_EXPLORER]) {
    BrainAlien *b = (BrainAlien *)e->data;

    do_light(lighting + EXPLORER_IR, e->x + e->w/2, e->y + e->h/2, 256, 1, 1, 0, 0);
    do_light(lighting + EXPLORER_EM, e->x + e->w/2, e->y + e->h/2, 320, 4, 1, 0, 1);

    if(b->target_entity < 0) {
        for(i16 i = 0; i < m->entity_count; i++) {
            Entity *ent = m->entities + m->entity_ids[i];
            if(ent->type != e->type && ent->id >= 0 && distance2_32(ent->x, ent->y, e->x, e->y) < 160*160) {
                b->target_entity = ent->id;
                break;
            }
        }
    }
    else {
        Entity *ent = m->entities + b->target_entity;
        if(ent->id >= 0 && distance2_32(e->x, e->y, ent->x, ent->y) < 160*160) {
            r32 angle = atan2(ent->y + ent->h/2 - e->y - e->h/2, ent->x + ent->w/2 - e->x - e->w/2);
            e->x_vel += (5*cos(angle) - e->x_vel) * 0.15;
            e->y_vel += (5*sin(angle) - e->y_vel) * 0.15;
        }
        else {
            b->target_entity = -1;
        }
    }
}

void draw_brain_alien(Entity *e, Camera *c) {
    BrainAlien *b = (BrainAlien *)e->data;
    draw_texture_region(&textures[TEX_SPRITES], e->x_vel < 0 ? FLIP_HORIZONTAL : 0,
                        0, 208, 40, 40,
                        e->x + e->w/2 - 20 - c->x, e->y + e->h/2 - 20 - c->y,
                        e->x_vel * 4);
}
