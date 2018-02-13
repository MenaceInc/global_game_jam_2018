#ifndef DRAW_H
#define DRAW_H

#define FLIP_HORIZONTAL  0x01
#define FLIP_VERTICAL    0x02

#define ALIGN_CENTER_X   0x01
#define ALIGN_CENTER_Y   0x02
#define ALIGN_RIGHT      0x04

struct FBO {
    GLuint id, texture;
    i16 w, h;
};

struct InstancedList {
    GLfloat *instance_data, *instance_render_data;
    u32 *vertex_attrib_arrays;
    GLuint vao, instance_buffer;
    Texture *texture;
    i8 instance_data_length;
    i16 max_instances;
};

#endif
