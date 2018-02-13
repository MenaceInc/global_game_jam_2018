#include <GL/glew.h>

#include "draw.h"

#define draw_texture(tex, flags, x, y, angle) draw_scaled_texture_region(tex, flags, 0, 0, (tex)->w, (tex)->h, x, y, (tex)->w, (tex)->h, angle)
#define draw_scaled_texture(tex, flags, x, y, wi, he, angle) draw_scaled_texture_region(tex, flags, 0, 0, (tex)->w, (tex)->h, x, y, wi, he, angle)
#define draw_texture_region(tex, flags, tx, ty, tw, th, x, y, angle) draw_scaled_texture_region(tex, flags, tx, ty, tw, th, x, y, tw, th, angle)
#define draw_fbo(fbo, flags, x, y) { Texture t = { (fbo)->w, -(fbo)->h, (fbo)->texture }; draw_texture(&t, flags, x, y-(t.h), 0); }
#define draw_scaled_fbo(fbo, flags, x, y, wi, he) { Texture t = { (fbo)->w, (fbo)->h, (fbo)->texture }; draw_scaled_texture(&t, flags, x, y+he, wi, -he, 0); }

#define reset_model() { model = HMM_Mat4d(1); }

#define translate(x, y) { model = HMM_Multiply(model, HMM_Translate(HMM_Vec3(x, y, 0))); }
#define scale(x, y) { model = HMM_Multiply(model, HMM_Scale(HMM_Vec3(x, y, 1))); }
#define rotate(angle) { model = HMM_Multiply(model, HMM_Rotate(angle, HMM_Vec3(0, 0, 1))); }

global GLfloat quad_vertices[12] = {
    0.0, 0.0, 0.0,
    0.0, 1.0, 0.0,
    1.0, 0.0, 0.0,
    1.0, 1.0, 0.0
};

global GLfloat quad_uvs[8] = {
    0.0, 0.0,
    0.0, 1.0,
    1.0, 0.0,
    1.0, 1.0
};

global GLfloat line_vertices[6] = {
    0.0, 0.0, 0.0,
    1.0, 0.0, 0.0
};

global GLuint quad_vertex_buffer = 0,
              quad_uv_buffer = 0,
              line_vertex_buffer = 0,

              active_shader = 0,
              min_filter = GL_LINEAR,
              mag_filter = GL_LINEAR;

global FBO *active_fbo = NULL;

global hmm_vec4 clip = HMM_Vec4(0, 0, 0, 0),
                tint = HMM_Vec4(1, 1, 1, 1);

global hmm_m4 model, view, projection;

global Shader rect_shader,
              filled_rect_shader,
              line_shader,
              texture_shader,
              text_shader;

FBO init_fbo(i16 w, i16 h) {
    FBO f;
    f.w = w;
    f.h = h;

    glGenFramebuffers(1, &f.id);
    glBindFramebuffer(GL_FRAMEBUFFER, f.id);

    glGenTextures(1, &f.texture);
    glBindTexture(GL_TEXTURE_2D, f.texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, f.texture, 0);
    GLenum draw_buffers[1] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, draw_buffers);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return f;
}

void clean_up_fbo(FBO *f) {
    glDeleteTextures(1, &f->texture);
    f->texture = 0;
    glDeleteFramebuffers(1, &f->id);
    f->id = 0;
}

InstancedList init_instanced_list(i8 instance_data_length, i16 max_instances, Texture *texture) {
    InstancedList i;
    i.instance_data_length = instance_data_length;
    i.max_instances = max_instances;
    i.texture = texture;

    i.instance_data = (GLfloat *)calloc(instance_data_length * max_instances, sizeof(GLfloat));
    i.instance_render_data = NULL;
    i.vertex_attrib_arrays = NULL;

    glGenVertexArrays(1, &i.vao);
    glBindVertexArray(i.vao);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, NULL);

    glGenBuffers(1, &i.instance_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, i.instance_buffer);
    glBufferData(GL_ARRAY_BUFFER, instance_data_length * max_instances * sizeof(GLfloat), i.instance_data, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return i;
}

void clean_up_instanced_list(InstancedList *il) {
    free(il->instance_data);
    da_free(il->vertex_attrib_arrays);
    glDeleteBuffers(1, &il->instance_buffer);
    glDeleteVertexArrays(1, &il->vao);
}

void add_instance_attribute(InstancedList *il, u32 index, i8 a_size, u8 offset) {
    glBindVertexArray(il->vao);
    glBindBuffer(GL_ARRAY_BUFFER, il->instance_buffer);
    {
        glVertexAttribPointer(index, a_size, GL_FLOAT, GL_FALSE, il->instance_data_length * sizeof(GLfloat), (void *)(offset * sizeof(GLfloat)));
        glVertexAttribDivisor(index, 1);
        glEnableVertexAttribArray(index);
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    da_push(il->vertex_attrib_arrays, index);
}

void update_instance_vbo(InstancedList *il) {
    for(u32 i = 0;
        i < da_size(il->instance_render_data) &&
        i < (u32)(il->max_instances * il->instance_data_length);
        i++) {
        il->instance_data[i] = il->instance_render_data[i];
    }
    glBindBuffer(GL_ARRAY_BUFFER, il->instance_buffer);
    glBufferData(GL_ARRAY_BUFFER, il->max_instances * il->instance_data_length * sizeof(GLfloat), il->instance_data, GL_STREAM_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void enable_instance_attributes(InstancedList *il) {
    for(u32 i = 0; i < da_size(il->vertex_attrib_arrays); i++) {
        glEnableVertexAttribArray(il->vertex_attrib_arrays[i]);
    }
}

void disable_instance_attributes(InstancedList *il) {
    for(u32 i = 0; i < da_size(il->vertex_attrib_arrays); i++) {
        glDisableVertexAttribArray(il->vertex_attrib_arrays[i]);
    }
}

r32 text_width(Font *font, const char *text) {
    r32 w = 0;

    if(font) {
        for(u32 i = 0; i < strlen(text); i++) {
            if(text[i] >= 32 && text[i] < 127) {
                w += (r32)font->char_x_advance[text[i] - 32];
            }
        }
    }

    return w;
}

r32 text_width_n(Font *font, const char *text, u32 n) {
    r32 w = 0;

    if(font) {
        for(u32 i = 0; i < n && i < strlen(text); i++) {
            if(text[i] >= 32 && text[i] < 127) {
                w += (r32)font->char_x_advance[text[i] - 32];
            }
        }
    }

    return w;
}

void set_render_clip(r32 x1, r32 y1, r32 x2, r32 y2) {
    y1 = active_fbo ? active_fbo->h - y1 : window_h - y1;
    y2 = active_fbo ? active_fbo->h - y2 : window_h - y2;
    clip = HMM_Vec4(x1, y2, x2, y1);
}

void reset_render_clip() {
    clip = active_fbo ? HMM_Vec4(0, 0, active_fbo->w, active_fbo->h) : HMM_Vec4(0, 0, window_w, window_h);
}

void bind_fbo(FBO *fbo) {
    if(fbo) {
        glBindFramebuffer(GL_FRAMEBUFFER, fbo->id);
        glViewport(0, 0, fbo->w, fbo->h);
        projection = HMM_Orthographic(0.f, (r32)fbo->w, (r32)fbo->h, 0.f, -1.f, 1.f);
    }
    else {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, window_w, window_h);
        projection = HMM_Orthographic(0.f, (r32)window_w, (r32)window_h, 0.f, -1.f, 1.f);
    }

    active_fbo = fbo;
    reset_render_clip();
}

void clear_fbo(FBO *fbo) {
    bind_fbo(fbo);
    {
        glClearColor(0, 0, 0, 0);
        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    }
    bind_fbo(NULL);
}

void draw_rect(r32 r, r32 g, r32 b, r32 a, r32 x, r32 y, r32 w, r32 h, r32 thickness) {
    if(w < 0) {
        x += w;
        w *= -1;
    }
    if(h < 0) {
        y += h;
        h *= -1;
    }

    i8 default_shader = 0;
    if(!active_shader) {
        default_shader = 1;
        active_shader = rect_shader.id;
    }

    glUseProgram(active_shader);

    model = HMM_Translate(HMM_Vec3(x, y, 0));
    model = HMM_Multiply(model, HMM_Scale(HMM_Vec3(w, h, 1)));

    glUniformMatrix4fv(glGetUniformLocation(active_shader, "projection"), 1, GL_FALSE, &projection.Elements[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(active_shader, "view"), 1, GL_FALSE, &view.Elements[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(active_shader, "model"), 1, GL_FALSE, &model.Elements[0][0]);

    glUniform4f(glGetUniformLocation(active_shader, "in_color"), r, g, b, a);
    glUniform4f(glGetUniformLocation(active_shader, "clip"), clip.X, clip.Y, clip.Z, clip.W);
    glUniform4f(glGetUniformLocation(active_shader, "tint"), tint.R, tint.G, tint.B, tint.A);
    glUniform1f(glGetUniformLocation(active_shader, "thickness"), thickness / w);
    glUniform2f(glGetUniformLocation(active_shader, "rect_size"), w, h);

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, quad_vertex_buffer);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);

    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, quad_uv_buffer);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, NULL);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glUseProgram(0);

    if(default_shader) {
        active_shader = 0;
    }
}

void draw_filled_rect(r32 r, r32 g, r32 b, r32 a, r32 x, r32 y, r32 w, r32 h) {
    if(w < 0) {
        x += w;
        w *= -1;
    }
    if(h < 0) {
        y += h;
        h *= -1;
    }

    i8 default_shader = 0;
    if(!active_shader) {
        default_shader = 1;
        active_shader = filled_rect_shader.id;
    }

    glUseProgram(active_shader);

    model = HMM_Translate(HMM_Vec3(x, y, 0));
    model = HMM_Multiply(model, HMM_Scale(HMM_Vec3(w, h, 1)));

    glUniformMatrix4fv(glGetUniformLocation(active_shader, "projection"), 1, GL_FALSE, &projection.Elements[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(active_shader, "view"), 1, GL_FALSE, &view.Elements[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(active_shader, "model"), 1, GL_FALSE, &model.Elements[0][0]);

    glUniform4f(glGetUniformLocation(active_shader, "in_color"), r, g, b, a);
    glUniform4f(glGetUniformLocation(active_shader, "clip"), clip.X, clip.Y, clip.Z, clip.W);
    glUniform4f(glGetUniformLocation(active_shader, "tint"), tint.R, tint.G, tint.B, tint.A);

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, quad_vertex_buffer);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);

    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, quad_uv_buffer);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, NULL);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glUseProgram(0);

    if(default_shader) {
        active_shader = 0;
    }
}

void draw_scaled_texture_region(Texture *texture, i8 flags, r32 tx, r32 ty, r32 tw, r32 th, r32 x, r32 y, r32 w, r32 h, r32 angle) {
    i8 default_shader = 0;
    if(!active_shader) {
        default_shader = 1;
        active_shader = texture_shader.id;
    }

    glUseProgram(active_shader);

    if(flags & FLIP_HORIZONTAL) {
        x += w;
        w *= -1;
    }
    if(flags & FLIP_VERTICAL) {
        y += h;
        h *= -1;
    }

    reset_model();
    translate(x, y);
    translate(w/2, h/2);
    rotate(angle);
    translate(-w/2, -h/2);
    scale(w, h);

    glUniformMatrix4fv(glGetUniformLocation(active_shader, "projection"), 1, GL_FALSE, &projection.Elements[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(active_shader, "view"), 1, GL_FALSE, &view.Elements[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(active_shader, "model"), 1, GL_FALSE, &model.Elements[0][0]);

    glUniform1i(glGetUniformLocation(active_shader, "tex"), 0);
    glUniform2f(glGetUniformLocation(active_shader, "uv_offset"), tx/texture->w, ty/texture->h);
    glUniform2f(glGetUniformLocation(active_shader, "uv_range"), tw/texture->w, th/texture->h);
    glUniform4f(glGetUniformLocation(active_shader, "clip"), clip.X, clip.Y, clip.Z, clip.W);
    glUniform4f(glGetUniformLocation(active_shader, "tint"), tint.R, tint.G, tint.B, tint.A);

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, quad_vertex_buffer);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);

    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, quad_uv_buffer);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, NULL);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture->id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glUseProgram(0);

    if(default_shader) {
        active_shader = 0;
    }
}

void draw_textn(Font *font, i8 flags, r32 r, r32 g, r32 b, r32 a, r32 x, r32 y, r32 font_scale, r32 boldness, r32 softness, const char *text, u32 n) {
    i8 default_shader = 0;
    if(!active_shader) {
        default_shader = 1;
        active_shader = text_shader.id;
    }

    glUseProgram(active_shader);

    if(flags & ALIGN_CENTER_X) {
        x -= (text_width(font, text) * font_scale) / 2;
    }
    else if(flags & ALIGN_RIGHT) {
        x -= (text_width(font, text) * font_scale);
    }

    if(flags & ALIGN_CENTER_Y) {
        y -= (font->line_height * font_scale) / 2;
    }

    glBindTexture(GL_TEXTURE_2D, font->texture.id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, quad_vertex_buffer);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);

    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, quad_uv_buffer);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void *)0);

    glUniform4f(glGetUniformLocation(active_shader, "clip"), clip.X, clip.Y, clip.Z, clip.W);
    glUniform4f(glGetUniformLocation(active_shader, "tint"), tint.R, tint.G, tint.B, tint.A);
    glUniform4f(glGetUniformLocation(active_shader, "text_color"), r, g, b, a);
    glUniform1f(glGetUniformLocation(active_shader, "boldness"), boldness);
    glUniform1f(glGetUniformLocation(active_shader, "softness"), softness);

    glUniformMatrix4fv(glGetUniformLocation(active_shader, "projection"), 1, GL_FALSE, &projection.Elements[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(active_shader, "view"), 1, GL_FALSE, &view.Elements[0][0]);

    r32 x_addition = 0;

    for(u32 i = 0; i < strlen(text) && i < n; i++) {
        if(text[i] >= 32 && text[i] < 127) {
            r32 x_advance = (r32)font->char_x_advance[text[i] - 32];

            if(text[i] > 32) {
                r32 char_x = (r32)font->char_x[text[i] - 32];
                r32 char_y = (r32)font->char_y[text[i] - 32];
                r32 char_w = (r32)font->char_w[text[i] - 32];
                r32 char_h = (r32)font->char_h[text[i] - 32];

                model = HMM_Translate(HMM_Vec3(x + x_addition + ((r32)font->char_x_offset[text[i] - 32] * font_scale),
                                               y + ((r32)font->char_y_offset[text[i] - 32] * font_scale), 0));
                model = HMM_Multiply(model, HMM_Scale(HMM_Vec3(char_w * font_scale, char_h * font_scale, 1.f)));

                glUniformMatrix4fv(glGetUniformLocation(active_shader, "model"), 1, GL_FALSE, &model.Elements[0][0]);

                glUniform2f(glGetUniformLocation(active_shader, "uv_offset"), char_x / font->texture.w, char_y / font->texture.h);
                glUniform2f(glGetUniformLocation(active_shader, "uv_range"), char_w / font->texture.w, char_h / font->texture.h);

                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            }

            x_addition += x_advance * font_scale;
        }
    }

    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(0);

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glUseProgram(0);

    if(default_shader) {
        active_shader = 0;
    }
}

void draw_text(Font *font, i8 flags, r32 r, r32 g, r32 b, r32 a, r32 x, r32 y, r32 font_scale, r32 boldness, r32 softness, const char *text) {
    draw_textn(font, flags, r, g, b, a, x, y, font_scale, boldness, softness, text, strlen(text));
}

void draw_text(Font *font, i8 flags, r32 r, r32 g, r32 b, r32 a, r32 x, r32 y, r32 font_scale, const char *text) {
    draw_text(font, flags, r, g, b, a, x, y, font_scale,
              0.75 + (0.15 * (1.0 - font_scale)),
              0.18 + (0.17 * (1.0 - font_scale)),
              text);
}

void draw_wrapped_textn(Font *font, i8 flags, r32 r, r32 g, r32 b, r32 a, r32 x, r32 y, r32 max_width, r32 font_scale, r32 boldness, r32 softness, const char *text,
                        u32 n) {
    i8 default_shader = 0;
    if(!active_shader) {
        default_shader = 1;
        active_shader = text_shader.id;
    }

    glUseProgram(active_shader);

    if(flags & ALIGN_CENTER_X) {
        x -= (text_width(font, text) * font_scale) / 2;
    }
    else if(flags & ALIGN_RIGHT) {
        x -= (text_width(font, text) * font_scale);
    }

    if(flags & ALIGN_CENTER_Y) {
        y -= (font->line_height * font_scale) / 2;
    }

    glBindTexture(GL_TEXTURE_2D, font->texture.id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, quad_vertex_buffer);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);

    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, quad_uv_buffer);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void *)0);

    glUniform4f(glGetUniformLocation(active_shader, "clip"), clip.X, clip.Y, clip.Z, clip.W);
    glUniform4f(glGetUniformLocation(active_shader, "tint"), tint.R, tint.G, tint.B, tint.A);
    glUniform4f(glGetUniformLocation(active_shader, "text_color"), r, g, b, a);
    glUniform1f(glGetUniformLocation(active_shader, "boldness"), boldness);
    glUniform1f(glGetUniformLocation(active_shader, "softness"), softness);

    glUniformMatrix4fv(glGetUniformLocation(active_shader, "projection"), 1, GL_FALSE, &projection.Elements[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(active_shader, "view"), 1, GL_FALSE, &view.Elements[0][0]);

    r32 x_addition = 0,
        y_addition = 0;

    i8 start_of_word = 0;

    for(u32 i = 0; i < n; i++) {
        if(text[i] >= 32 && text[i] < 127) {
            r32 x_advance = (r32)font->char_x_advance[text[i] - 32];

            if(text[i] > 32) {
                r32 char_x = (r32)font->char_x[text[i] - 32];
                r32 char_y = (r32)font->char_y[text[i] - 32];
                r32 char_w = (r32)font->char_w[text[i] - 32];
                r32 char_h = (r32)font->char_h[text[i] - 32];

                if(start_of_word) {
                    r32 necessary_space = 0;
                    for(u32 j = i; j < strlen(text); j++) {
                        if(text[j] == 32) {
                            break;
                        }
                        else {
                            necessary_space += x_advance * font_scale;
                        }
                    }

                    if(x_addition + necessary_space >= max_width) {
                        x_addition = 0;
                        y_addition += font->line_height*font_scale;
                    }
                    start_of_word = 0;
                }
                else {
                    if(x_addition + x_advance*font_scale >= max_width) {
                        x_addition = 0;
                        y_addition += font->line_height*font_scale;
                    }
                }

                model = HMM_Translate(HMM_Vec3(x + x_addition + ((r32)font->char_x_offset[text[i] - 32] * font_scale),
                                               y + y_addition + ((r32)font->char_y_offset[text[i] - 32] * font_scale), 0));
                model = HMM_Multiply(model, HMM_Scale(HMM_Vec3(char_w * font_scale, char_h * font_scale, 1.f)));

                glUniformMatrix4fv(glGetUniformLocation(active_shader, "model"), 1, GL_FALSE, &model.Elements[0][0]);

                glUniform2f(glGetUniformLocation(active_shader, "uv_offset"), char_x / font->texture.w, char_y / font->texture.h);
                glUniform2f(glGetUniformLocation(active_shader, "uv_range"), char_w / font->texture.w, char_h / font->texture.h);

                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            }
            else {
                start_of_word = 1;
            }

            x_addition += x_advance * font_scale;
        }
    }

    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(0);

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glUseProgram(0);

    if(default_shader) {
        active_shader = 0;
    }
}

void draw_line(r32 r, r32 g, r32 b, r32 a, r32 x1, r32 y1, r32 x2, r32 y2) {
    i8 default_shader = 0;
    if(!active_shader) {
        active_shader = line_shader.id;
        default_shader = 1;
    }

    glUseProgram(active_shader);

    r32 distance = distance_32(x1, y1, x2, y2);
    r32 rotation = atan2(y2 - y1, x2 - x1) + (PI / 2);
    model = HMM_Translate(HMM_Vec3(x1 + ((x2 - x1) / 2), y1 + ((y2 - y1) / 2), 0));
    model = HMM_Multiply(model, HMM_Rotate(rotation * (180 / PI), HMM_Vec3(0, 0, 1)));
    model = HMM_Multiply(model, HMM_Translate(HMM_Vec3(0, -(distance / 2), 0)));
    model = HMM_Multiply(model, HMM_Scale(HMM_Vec3(1.f, distance, 1.f)));

    glUniformMatrix4fv(glGetUniformLocation(active_shader, "projection"), 1, GL_FALSE, &(projection.Elements[0][0]));
    glUniformMatrix4fv(glGetUniformLocation(active_shader, "view"), 1, GL_FALSE, &(view.Elements[0][0]));
    glUniformMatrix4fv(glGetUniformLocation(active_shader, "model"), 1, GL_FALSE, &(model.Elements[0][0]));

    glUniform4f(glGetUniformLocation(active_shader, "in_color"), r, g, b, a);
    glUniform4f(glGetUniformLocation(active_shader, "clip"), clip.X, clip.Y, clip.Z, clip.W);
    glUniform4f(glGetUniformLocation(active_shader, "tint"), tint.R, tint.G, tint.B, tint.A);

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, quad_vertex_buffer);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);

    glDrawArrays(GL_LINES, 0, 2);

    glDisableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glUseProgram(0);

    if(default_shader) {
        active_shader = 0;
    }
}

void draw_instanced_list(InstancedList *il) {
    update_instance_vbo(il);

    glUseProgram(active_shader);

    glUniformMatrix4fv(glGetUniformLocation(active_shader, "projection"), 1, GL_FALSE, &(projection.Elements[0][0]));
    glUniformMatrix4fv(glGetUniformLocation(active_shader, "view"), 1, GL_FALSE, &(view.Elements[0][0]));
    glUniform4f(glGetUniformLocation(active_shader, "clip"), clip.X, clip.Y, clip.Z, clip.W);
    glUniform4f(glGetUniformLocation(active_shader, "tint"), tint.R, tint.G, tint.B, tint.A);
    glUniform1i(glGetUniformLocation(active_shader, "tex"), 0);

    glActiveTexture(GL_TEXTURE0);
    if(il->texture) {
        glBindTexture(GL_TEXTURE_2D, il->texture->id);
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glBindVertexArray(il->vao);

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, quad_vertex_buffer);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);

    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, quad_uv_buffer);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void *)0);

    enable_instance_attributes(il);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, (int)da_size(il->instance_render_data) / il->instance_data_length);
    disable_instance_attributes(il);

    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(0);

    glBindVertexArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glUseProgram(0);
}
