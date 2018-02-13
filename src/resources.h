#ifndef RESOURCES_H
#define RESOURCES_H

#include <GL/glew.h>
#include <AL/al.h>

#define request_shader(i)   request(i)
#define request_texture(i)  request(MAX_SHADER + i)
#define request_font(i)     request(MAX_SHADER + MAX_TEX + i)
#define request_sound(i)    request(MAX_SHADER + MAX_TEX + MAX_FONT + i)

#define unrequest_shader(i)   unrequest(i)
#define unrequest_texture(i)  unrequest(MAX_SHADER + i)
#define unrequest_font(i)     unrequest(MAX_SHADER + MAX_TEX + i)
#define unrequest_sound(i)    unrequest(MAX_SHADER + MAX_TEX + MAX_FONT + i)

/* Shaders */
enum ShaderType {
    SHADER_CRT,
    SHADER_PARTICLE,
    SHADER_LIGHTING,
    SHADER_TILES,
    MAX_SHADER
};

/* Textures */
enum TextureType {
    TEX_SPRITES,

    MAX_TEX
};

/* Fonts */
enum FontType {
    FONT_BASE,
    FONT_TITLE,
    MAX_FONT
};

/* Sounds */
enum SoundType {
    SOUND_BUTTON,
    SOUND_EXPLODE_1,
    SOUND_EXPLODE_2,
    SOUND_HURT,

    SOUND_THEME,
    SOUND_UNKNOWN_WORLD,
    SOUND_STONE,
    SOUND_MAGMA,
    SOUND_DANGER_ZONE,

    MAX_SOUND
};

struct Shader {
    GLuint id;
};

struct Texture {
    i16 w, h;
    GLuint id;
};

struct Font {
    Texture texture;
    i16 size, line_height,
        char_x[95],
        char_y[95],
        char_w[95],
        char_h[95],
        char_x_offset[95],
        char_y_offset[95],
        char_x_advance[95];
};

struct Sound {
    ALuint id;
    u64 sample_count;
    u32 sample_rate;
};

#endif
