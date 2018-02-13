#include "particle.h"

enum {
    PARTICLE_SMOKE,
    PARTICLE_FIRE,
    PARTICLE_DEBRIS,
    MAX_PARTICLE
};

struct {
    r32 life_decay;
    i16 tx, ty, w, h;
} particle_data[MAX_PARTICLE] = {
    { 0.05, 32, 24, 8, 8 },
    { 0.025, 24, 24, 8, 8 },
    { 0.07, 0, 0, 8, 8 },
};

ParticleGroup init_particle_group(i8 type) {
    ParticleGroup g = {
        type,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        init_instanced_list(17, 1500, &textures[TEX_SPRITES])
    };

    add_instance_attribute(&g.il, 2, 4, 0);
    add_instance_attribute(&g.il, 3, 4, 4);
    add_instance_attribute(&g.il, 4, 4, 8);
    add_instance_attribute(&g.il, 5, 4, 12);
    add_instance_attribute(&g.il, 6, 1, 16);

    return g;
}

void clean_up_particle_group(ParticleGroup *g) {
    da_free(g->x_pos);
    da_free(g->y_pos);
    da_free(g->x_vel);
    da_free(g->y_vel);
    da_free(g->life);
    da_free(g->il.instance_render_data);
    clean_up_instanced_list(&g->il);
}

void request_particle_group_resources(ParticleGroup *g) {
    request_shader(SHADER_PARTICLE);
}

void unrequest_particle_group_resources(ParticleGroup *g) {
    unrequest_shader(SHADER_PARTICLE);
}

void do_particle(ParticleGroup *g, r32 x, r32 y, r32 x_vel, r32 y_vel) {
    da_push(g->x_pos, x);
    da_push(g->y_pos, y);
    da_push(g->x_vel, x_vel);
    da_push(g->y_vel, y_vel);
    r32 life = 1;
    da_push(g->life, life);
}

void update_particle_group(ParticleGroup *g, Camera *c, r32 x, r32 y, Map *m) {
    da_clear(g->il.instance_render_data);

    Camera cam = init_camera(0, 0);
    if(!c) { c = &cam; }

    i8 particle_dead = 0;
    for(u32 i = 0; i < da_size(g->x_pos);) {
        particle_dead = 0;
        g->x_pos[i] += g->x_vel[i];
        g->y_pos[i] += g->y_vel[i];
        g->y_vel[i] += 0.003;
        g->life[i] -= particle_data[g->type].life_decay;

        if(g->life[i] <= 0) {
            particle_dead = 1;
        }

        if(particle_dead) {
            da_erase(g->x_pos, i);
            da_erase(g->y_pos, i);
            da_erase(g->x_vel, i);
            da_erase(g->y_vel, i);
            da_erase(g->life, i);
        }
        else {
            hmm_mat4 model = HMM_Translate(HMM_Vec3(g->x_pos[i] + x - c->x, g->y_pos[i] + y - c->y, 0));
            model = HMM_Multiply(model, HMM_Scale(HMM_Vec3(particle_data[g->type].w, particle_data[g->type].h, 1)));
            for(int x = 0; x < 4; x++) {
                for(int y = 0; y < 4; y++) {
                    da_push(g->il.instance_render_data, model.Elements[x][y]);
                }
            }
            r32 progress = 1 - g->life[i];
            da_push(g->il.instance_render_data, progress);

            ++i;
        }
    }

    glDepthMask(GL_FALSE);

    i8 additive = g->type == PARTICLE_FIRE;

    if(additive) {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    }

    active_shader = shaders[SHADER_PARTICLE].id;
    glUseProgram(active_shader);

    Texture *texture = &textures[TEX_SPRITES];
    glUniform2f(glGetUniformLocation(active_shader, "uv_offset"), (r32)particle_data[g->type].tx/texture->w, (r32)particle_data[g->type].ty/texture->h);
    glUniform2f(glGetUniformLocation(active_shader, "uv_range"), (r32)particle_data[g->type].w/texture->w, (r32)particle_data[g->type].h/texture->h);

    draw_instanced_list(&g->il);
    active_shader = 0;

    if(additive) {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    glDepthMask(GL_TRUE);
}
