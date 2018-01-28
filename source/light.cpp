#include "light.h"

LightState init_light_state() {
    LightState l;
    l.light_count = 0;
    l.default_light = 1;
    return l;
}

void do_light(LightState *l, r32 x, r32 y, r32 radius, r32 intensity, r32 r, r32 g, r32 b) {
    Light light = { x, y, radius, intensity, r, g, b };
    l->lights[l->light_count++] = light;
}

void update_light_state(LightState *l, Camera *c, r32 bound_x, r32 bound_y) {
    active_shader = shaders[SHADER_LIGHTING].id;
    glUseProgram(active_shader);

    i16 current_light = 0;
    for(i16 i = 0; i < l->light_count && current_light < 16; i++) {
        if(l->lights[i].x + l->lights[i].radius >= c->x &&
           l->lights[i].x - l->lights[i].radius <= c->x + bound_x &&
           l->lights[i].y + l->lights[i].radius >= c->y &&
           l->lights[i].y - l->lights[i].radius <= c->y + bound_y) {
            char light_attributes_name[64] = { 0 },
                 light_color_name[64] = { 0 };
            sprintf(light_attributes_name, "lights[%i].attributes", current_light);
            sprintf(light_color_name, "lights[%i].color", current_light);

            glUniform4f(glGetUniformLocation(active_shader, light_attributes_name), l->lights[i].x-c->x, l->lights[i].y-c->y, l->lights[i].radius, l->lights[i].intensity);
            glUniform3f(glGetUniformLocation(active_shader, light_color_name), l->lights[i].r, l->lights[i].g, l->lights[i].b);

            ++current_light;
        }
    }

    glUniform1i(glGetUniformLocation(active_shader, "light_count"), current_light);

    active_shader = 0;
    glUseProgram(0);
    l->light_count = 0;
}
