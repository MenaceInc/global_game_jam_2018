#include <SOIL/SOIL.h>

#define DR_WAV_IMPLEMENTATION
#include "ext/dr_wav.h"

#include "ext/rf_resource_loading.h"
#include "resources.h"

#include "ext/stb_vorbis.c"

#define MAX_RESOURCE (MAX_SHADER+MAX_TEX+MAX_FONT+MAX_SOUND)
#define MAX_RESOURCE_FILE (MAX_SHADER*resource_type_file_count(SHADER) +\
                           MAX_TEX*resource_type_file_count(TEXTURE) +\
                           MAX_FONT*resource_type_file_count(FONT) +\
                           MAX_SOUND*resource_type_file_count(SOUND))


#define resource_type_file_count(i) (i == SHADER ? 3 :\
                                     i == TEXTURE ? 1 :\
                                     i == FONT ? 2 :\
                                     i == SOUND ? 1 :\
                                     1)

enum {
    SHADER,
    TEXTURE,
    FONT,
    SOUND,
    MAX_RESOURCE_TYPE
};

global mtr_ResourceMaster *resource_master = NULL;

global void       *resources[MAX_RESOURCE];
global Shader      shaders[MAX_SHADER];
global Texture     textures[MAX_TEX];
global Font        fonts[MAX_FONT];
global Sound       sounds[MAX_SOUND];

/* RESOURCE TYPE INFORMATION */

global const i16 resource_type_count[MAX_RESOURCE_TYPE] = {
    MAX_SHADER,
    MAX_TEX,
    MAX_FONT,
    MAX_SOUND
};

global const char *resource_type_directory[MAX_RESOURCE_TYPE] = {
    "resource/shader/",
    "resource/img/",
    "resource/font/",
    "resource/snd/"
};

global const char *shader_file_extensions[resource_type_file_count(SHADER)] = {
    "vert",
    "frag",
    "info"
};

global const char *texture_extension = "png";

global const char *font_file_extensions[resource_type_file_count(FONT)] = {
    "png",
    "fnt"
};

global const char *sound_extension = "wav";

/* RESOURCE FILENAMES */

global char *resource_filenames[MAX_RESOURCE_FILE] = { 0 };

global const char *shader_filenames[MAX_SHADER] = {
    "crt",
    "particle",
    "lighting",
    "tiles",
};

global const char *texture_filenames[MAX_TEX] = {
    "sprites",
};

global const char *font_filenames[MAX_FONT] = {
    "hack",
    "title"
};

global const char *sound_filenames[MAX_SOUND] = {
    "button",
    "explode1",
    "explode2",
    "hurt",

    "theme",
    "unknown_world",
    "stone",
    "magma",
    "danger_zone",
};

i8 resource_type_by_file_id(i16 id) {
    i16 offset = 0;
    for(i8 i = 0; i < MAX_RESOURCE_TYPE; i++) {
        offset += resource_type_count[i] * resource_type_file_count(i);
        if(id < offset) {
            return i;
        }
    }
    return -1;
}

i8 resource_type_by_id(i16 id) {
    i16 offset = 0;
    for(i8 i = 0; i < MAX_RESOURCE_TYPE; i++) {
        offset += resource_type_count[i];
        if(id < offset) {
            return i;
        }
    }
    return -1;
}

i16 resource_index_by_file_id(i16 id) {
    i8 type = 0;
    i16 offset = 0;

    for(i8 i = 0; i < MAX_RESOURCE_TYPE; i++) {
        offset = resource_type_count[i] * resource_type_file_count(i);
        if(id < offset) {
            type = i;
            break;
        }
        else {
            id -= offset;
        }
    }
    id /= resource_type_file_count(type);

    return id;
}

i16 get_resource_type_offset(i8 resource_type) {
    i16 offset = 0;
    for(i8 i = 0; i < resource_type; i++) {
        offset += resource_type_count[i] * resource_type_file_count(i);
    }
    return offset;
}

i16 file_start_index_by_id(i16 id) {
    i16 file_id = 0;
    i8 type = 0;
    for(i16 i = 0; i < id; i++) {
        type = resource_type_by_id(i);
        file_id += resource_type_file_count(type);
    }
    return file_id;
}

static i16 resource_requests[MAX_RESOURCE] = { 0 },
           waiting_resources[MAX_RESOURCE] = { 0 },
           waiting_resource_count = 0;

Shader init_shader_from_data(void *vert, i64 vert_len,
                             void *frag, i64 frag_len,
                             void *info, i64 info_len) {

    Shader s;

    GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);
    GLint result = GL_FALSE, code_len = 0;
    i32 info_log_length = 0;
    char *code = NULL;

    {
        code_len = vert_len;
        code = (char *)vert;

        fprintf(log_file, "compiling vertex shader\n");
        glShaderSource(vertex_shader_id, 1, &code, &code_len);
        glCompileShader(vertex_shader_id);
    }

    glGetShaderiv(vertex_shader_id, GL_COMPILE_STATUS, &result);
    glGetShaderiv(vertex_shader_id, GL_INFO_LOG_LENGTH, &info_log_length);
    if(info_log_length) {
        char *vertex_shader_error = (char *)malloc(info_log_length * sizeof(char));
        glGetShaderInfoLog(vertex_shader_id, info_log_length, NULL, vertex_shader_error);
        fprintf(log_file, "%s\n", vertex_shader_error);
        free(vertex_shader_error);
    }

    {
        code_len = frag_len;
        code = (char *)frag;
        fprintf(log_file, "compiling fragment shader\n");
        glShaderSource(fragment_shader_id, 1, &code, &code_len);
        glCompileShader(fragment_shader_id);
    }

    glGetShaderiv(fragment_shader_id, GL_COMPILE_STATUS, &result);
    glGetShaderiv(fragment_shader_id, GL_INFO_LOG_LENGTH, &info_log_length);
    if(info_log_length) {
        char *fragment_shader_error = (char *)malloc(info_log_length * sizeof(char));
        glGetShaderInfoLog(fragment_shader_id, info_log_length, NULL, fragment_shader_error);
        fprintf(log_file, "%s\n", fragment_shader_error);
        free(fragment_shader_error);
    }

    GLuint program_id = glCreateProgram();
    glAttachShader(program_id, vertex_shader_id);
    glAttachShader(program_id, fragment_shader_id);

    enum {
        READ_DIRECTION,
        READ_NAME,
        READ_INDEX
    };

    i8 read_mode = READ_DIRECTION,
       in = 0;
    u32 read_start = 0;
    char *name = NULL, *index_str = NULL;

    code = (char *)info;

    for(u64 i = 0; i < (u64)info_len; i++) {
        switch(read_mode) {
            case READ_DIRECTION: {
                if(!strncmp(code + i, "in", 2)) {
                    in = 1;
                    read_mode = READ_NAME;
                    read_start = i + 3;
                    i += 3;
                }
                else if(!strncmp(code + i, "out", 3)) {
                    in = 0;
                    read_mode = READ_NAME;
                    read_start = i + 4;
                    i += 4;
                }
                break;
            }
            case READ_NAME: {
                if(code[i] == ' ') {
                    name = (char *)malloc(sizeof(char) * (i - read_start + 1));
                    name = strncpy(name, code + read_start, (i - read_start));
                    name[i - read_start] = '\0';
                    read_start = i + 1;
                    read_mode = READ_INDEX;
                }
                break;
            }
            case READ_INDEX: {
                if(i == strlen(code) - 1 ||
                   code[i] == ' ' ||
                   code[i] == '\n') {

                    index_str = (char *)malloc(sizeof(char) * (i - read_start + 1));
                    index_str = strncpy(index_str, code + read_start, (i - read_start));
                    index_str[i - read_start] = '\0';

                    if(in) {
                        fprintf(log_file, "binding \"%s\" for input at index %i\n", name, atoi(index_str));
                        glBindAttribLocation(program_id, atoi(index_str), name);
                    }
                    else {
                        fprintf(log_file, "binding \"%s\" for output at index %i\n", name, atoi(index_str));
                        glBindFragDataLocation(program_id, atoi(index_str), name);
                    }

                    free(name);
                    free(index_str);
                    name = NULL;
                    index_str = NULL;

                    read_mode = READ_DIRECTION;
                }
                break;
            }
            default: break;
        }
    }

    fprintf(log_file, "linking program\n");
    glLinkProgram(program_id);

    glGetProgramiv(program_id, GL_LINK_STATUS, &result);
    glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &info_log_length);
    if(info_log_length) {
        char *link_shader_error = (char *)malloc(info_log_length);
        glGetProgramInfoLog(program_id, info_log_length, NULL, link_shader_error);
        fprintf(log_file, "%s\n", link_shader_error);
        free(link_shader_error);
    }

    glValidateProgram(program_id);

    glDeleteShader(vertex_shader_id);
    glDeleteShader(fragment_shader_id);

    s.id = program_id;

    fprintf(log_file, "shader successfully compiled\n\n");

    return s;
}

Shader load_shader(const char *vert_filename, const char *frag_filename, const char *info_filename) {
    Shader s;
    s.id = 0;

    void *vert = 0, *frag = 0, *info = 0;
    i64 vert_len = 0, frag_len = 0, info_len = 0;

    //vertex shader reading
    FILE *file = fopen(vert_filename, "r");
    if(file) {
        fseek(file, 0, SEEK_END);
        vert_len = ftell(file);
        rewind(file);

        vert = calloc(vert_len + 2, sizeof(char));
        if(!vert) {
            fprintf(log_file, "ERROR: allocation for vertex shader code failed\n");
        }
        fread(vert, 1, vert_len, file);

        ((char *)vert)[vert_len] = '\n';
        ((char *)vert)[vert_len + 1] = '\0';

        fclose(file);

        //fragment shader reading
        file = fopen(frag_filename, "r");
        if(file) {
            fseek(file, 0, SEEK_END);
            frag_len = ftell(file);
            rewind(file);

            frag = calloc(frag_len + 2, sizeof(char));
            if(!frag) {
                fprintf(log_file, "ERROR: allocation for fragment shader code failed\n");
            }
            fread(frag, 1, frag_len, file);

            ((char *)frag)[frag_len] = '\n';
            ((char *)frag)[frag_len + 1] = '\0';

            fclose(file);

            //shader info reading
            file = fopen(info_filename, "r");
            if(file) {
                fseek(file, 0, SEEK_END);
                info_len = ftell(file);
                rewind(file);

                info = calloc(info_len + 2, sizeof(char));
                if(!info) {
                    fprintf(log_file, "ERROR: allocation for shader info code failed\n");
                }
                fread(info, 1, info_len, file);

                ((char *)info)[info_len] = '\n';
                ((char *)info)[info_len + 1] = '\0';

                fclose(file);
            }
            else {
                fprintf(log_file, "ERROR: could not open \"%s\"\n", info_filename);
                return s;
            }
        }
        else {
            fprintf(log_file, "ERROR: could not open \"%s\"\n", frag_filename);
            return s;
        }
    }
    else {
        fprintf(log_file, "ERROR: could not open \"%s\"\n", vert_filename);
        return s;
    }

    s = init_shader_from_data(vert, vert_len,
                              frag, frag_len,
                              info, info_len);

    free(vert);
    free(frag);
    free(info);

    return s;
}

Texture init_texture_from_data(void *data, i64 len) {
    Texture t;

    int w, h;
    unsigned char *tex_data = SOIL_load_image_from_memory((unsigned char *)data, len, &w, &h, 0, SOIL_LOAD_RGBA);
    t.w = w;
    t.h = h;

    glGenTextures(1, &t.id);
    glBindTexture(GL_TEXTURE_2D, t.id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, t.w, t.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex_data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    SOIL_free_image_data(tex_data);

    return t;
}

Font init_font_from_data(void *image, i64 image_len,
                         void *font, i64 font_len) {
    Font f;

    f.texture = init_texture_from_data(image, image_len);

    enum {
        READ_NONE = 0,
        READ_NAME,
        READ_VALUE,
        MAX_READ
    };

    char *data = (char *)font,
         name[32] = { 0 },
         value[32] = { 0 };

    i8 read_char = 0,
       read_state = 0,
       commented = 0;

    u32 read_pos = 0;

    for(i64 i = 0; i < font_len;) {
        if(data[i] == '#') {
            commented = 1;
        }

        if(commented) {
            if(data[i] == '\n') {
                commented = 0;
            }
            i++;
        }
        else {
            switch(read_state) {
                case READ_NONE: {
                    if(isalpha(data[i])) {
                        read_state = READ_NAME;
                        read_pos = i;
                    }
                    else {
                        i++;
                    }
                    break;
                }
                case READ_NAME: {
                    if(data[i] == '=') {
                        i16 name_len = i + 1 - read_pos < 32 ? i + 1 - read_pos : 31;
                        strncpy(name, data + read_pos, name_len);
                        name[name_len - 1] = '\0';

                        read_state = READ_VALUE;
                        read_pos = ++i;
                    }
                    else {
                        i++;
                    }
                    break;
                }
                case READ_VALUE: {
                    if(isspace(data[i]) || i == strlen(data) - 1) {
                        i16 value_len = i + 1 - read_pos < 32 ? i + 1 - read_pos : 31;
                        strncpy(value, data + read_pos, value_len);
                        value[value_len - 1] = '\0';

                        if(!strcmp(name, "char id")) {
                            read_char = atoi(value) - 32;
                            if(read_char < 0 || read_char >= 95) {
                                read_char = 0;
                            }
                        }
                        else if(!strcmp(name, "x")) {
                            f.char_x[read_char] = atoi(value);
                        }
                        else if(!strcmp(name, "y")) {
                            f.char_y[read_char] = atoi(value);
                        }
                        else if(!strcmp(name, "width")) {
                            f.char_w[read_char] = atoi(value);
                        }
                        else if(!strcmp(name, "height")) {
                            f.char_h[read_char] = atoi(value);
                        }
                        else if(!strcmp(name, "xoffset")) {
                            f.char_x_offset[read_char] = atoi(value);
                        }
                        else if(!strcmp(name, "yoffset")) {
                            f.char_y_offset[read_char] = atoi(value);
                        }
                        else if(!strcmp(name, "xadvance")) {
                            f.char_x_advance[read_char] = atoi(value) - 16;
                        }
                        else if(!strcmp(name, "size")) {
                            f.size = atoi(value);
                        }
                        else if(!strcmp(name, "lineHeight")) {
                            f.line_height = atoi(value);
                        }

                        read_state = READ_NONE;
                    }
                    else {
                        i++;
                    }
                    break;
                }
                default: {
                    i++;
                    break;
                }
            }
        }
    }

    return f;
}

Sound init_sound_from_wav_data(void *data, i64 len) {
    Sound s;

    ALenum format = 0;

    drwav *file = drwav_open_memory(data, len);
    if(file) {
        i16 *data_i = NULL;

        s.sample_count = file->totalSampleCount;
        s.sample_rate = file->sampleRate;

        data_i = (i16 *)malloc(sizeof(i16) * s.sample_count);
        r32 *sample_data_f = (r32 *)malloc(sizeof(r32) * s.sample_count);
        drwav_read_f32(file, s.sample_count, sample_data_f);

        for(u64 i = 0; i < s.sample_count; i++) {
            data_i[i] = (i16)(sample_data_f[i] * 32767.f);
        }

        if(file->bitsPerSample == 16) {
            if(file->channels == 1) {
                format = AL_FORMAT_MONO16;
            }
            else if(file->channels == 2) {
                format = AL_FORMAT_STEREO16;
            }
        }
        else if(file->bitsPerSample == 8) {
            if(file->channels == 1) {
                format = AL_FORMAT_MONO8;
            }
            else if(file->channels == 2) {
                format = AL_FORMAT_STEREO8;
            }
        }

        alGenBuffers(1, &s.id);
        alBufferData(s.id, format,
                     data_i,
                     s.sample_count * sizeof(i16),
                     s.sample_rate);

        free(data_i);
        free(sample_data_f);
        drwav_close(file);
    }

    return s;
}

Sound init_sound_from_ogg_data(void *data, i64 len) {
    Sound s;

    s.id = 0;
    s.sample_count = 0;
    s.sample_rate = 0;

    i32 channels = 0, sample_rate = 0;
    i16 *data_i = NULL;
    ALenum format = 0;

    s.sample_count = stb_vorbis_decode_memory((u8 *)data, len, &channels, &sample_rate, &data_i);
    if(s.sample_count > 0) {
        s.sample_rate = sample_rate;

        if(channels == 1) {
            format = AL_FORMAT_MONO16;
        }
        else if(channels == 2) {
            format = AL_FORMAT_STEREO16;
        }

        alGenBuffers(1, &s.id);
        alBufferData(s.id, format,
                     data_i,
                     s.sample_count * channels * sizeof(i16),
                     s.sample_rate);

        free(data_i);
    }

    return s;
}

void clean_up_shader(Shader *s) {
    if(s->id) {
        glDeleteProgram(s->id);
        s->id = 0;
    }
}

void clean_up_texture(Texture *t) {
    if(t->id) {
        glDeleteTextures(1, &t->id);
        t->id = 0;
    }
}

void clean_up_font(Font *f) {
    if(f->texture.id) {
        clean_up_texture(&f->texture);
    }
}

void clean_up_sound(Sound *s) {
    if(s->id) {
        alDeleteBuffers(1, &s->id);
        s->id = 0;
    }
}

void clean_up_resource(i16 i) {
    i8 type = resource_type_by_id(i);

    switch(type) {
        case SHADER: {
            clean_up_shader((Shader *)resources[i]);
            break;
        }
        case TEXTURE: {
            clean_up_texture((Texture *)resources[i]);
            break;
        }
        case FONT: {
            clean_up_font((Font *)resources[i]);
            break;
        }
        case SOUND: {
            clean_up_sound((Sound *)resources[i]);
            break;
        }
        default: break;
    }
}

void request(i16 i) {
    if(!resource_requests[i]++) {
        i8 type = resource_type_by_id(i);
        i16 file_start_index = file_start_index_by_id(i);
        for(i8 j = 0; j < resource_type_file_count(type); j++) {
            mtr_request_resource(resource_master, file_start_index + j);
        }

        waiting_resources[waiting_resource_count++] = i;
    }
}

void unrequest(i16 i) {
    if(--resource_requests[i] < 1) {
        clean_up_resource(i);
        resource_requests[i] = 0;
    }
}

void init_resources() {
    {
        for(u32 i = 0; i < MAX_SHADER; i++) {
            resources[i] = &shaders[i];
        }
        for(u32 i = 0; i < MAX_TEX; i++) {
            resources[MAX_SHADER + i] = &textures[i];
        }
        for(u32 i = 0; i < MAX_FONT; i++) {
            resources[MAX_SHADER+MAX_TEX+i] = &fonts[i];
        }
        for(u32 i = 0; i < MAX_SOUND; i++) {
            resources[MAX_SHADER+MAX_TEX+MAX_FONT+i] = &sounds[i];
        }
    }

    i16 file_i = 0;
    for(i16 i = 0; i < MAX_RESOURCE; i++) {
        i8 type = resource_type_by_id(i);
        i16 resource_i = resource_index_by_file_id(file_i);
        u16 filename_length = strlen(resource_type_directory[type]);

        for(i8 j = 0; j < resource_type_file_count(type); j++) {
            char *filename = NULL;
            switch(type) {
                case SHADER: {
                    filename_length += strlen(shader_filenames[i]) + 5;
                    filename = (char *)calloc(filename_length + 1, 1);
                    sprintf(filename, "%s%s.%s",
                            resource_type_directory[type],
                            shader_filenames[resource_i],
                            shader_file_extensions[j]);
                    break;
                }
                case TEXTURE: {
                    filename_length += strlen(texture_filenames[resource_i]) + 4;
                    filename = (char *)calloc(filename_length+1, 1);
                    sprintf(filename, "%s%s.%s",
                            resource_type_directory[type],
                            texture_filenames[resource_i],
                            texture_extension);
                    break;
                }
                case FONT: {
                    filename_length += strlen(font_filenames[resource_i]) + 4;
                    filename = (char *)calloc(filename_length+1, 1);
                    sprintf(filename, "%s%s.%s",
                            resource_type_directory[type],
                            font_filenames[resource_i],
                            font_file_extensions[j]);
                    break;
                }
                case SOUND: {
                    filename_length += strlen(sound_filenames[resource_i]) + 4;
                    filename = (char *)calloc(filename_length+1, 1);
                    sprintf(filename, "%s%s.%s",
                            resource_type_directory[type],
                            sound_filenames[resource_i],
                            sound_extension);

                    if(!file_exists(filename)) {
                        filename[strlen(filename) - 3] = 'o';
                        filename[strlen(filename) - 2] = 'g';
                        filename[strlen(filename) - 1] = 'g';
                    }
                    break;
                }
            }
            resource_filenames[file_i + j] = filename;
        }

        file_i += resource_type_file_count(type);
    }

    resource_master = mtr_new(MAX_RESOURCE_FILE, (const char **)resource_filenames);
}

void clean_up_resources() {
    mtr_clean_up(resource_master);
    resource_master = NULL;
    for(i16 i = 0; i < MAX_RESOURCE; i++) {
        clean_up_resource(i);
    }
    for(i16 i = 0; i < MAX_RESOURCE_FILE; i++) {
        free(resource_filenames[i]);
    }
}

void update_resources() {
    mtr_update(resource_master);

    for(i16 i = 0; i < waiting_resource_count; i++) {
        i8 type = resource_type_by_id(waiting_resources[i]),
           ready = 1;
        i16 file_start_index = file_start_index_by_id(waiting_resources[i]);

        for(i8 j = 0; j < resource_type_file_count(type); j++) {
            if(!mtr_resource_data_ready(resource_master, file_start_index + j)) {
                ready = 0;
                break;
            }
        }

        if(ready) {
            i64 data_len[resource_type_file_count(type)] = { 0 };
            void *data[resource_type_file_count(type)] = { 0 };

            for(i8 j = 0; j < resource_type_file_count(type); j++) {
                mtr_grab_resource_data(resource_master, file_start_index + j, &data[j], &data_len[j]);
            }

            switch(type) {
                case SHADER: {
                    fprintf(log_file, "initializing shader \"%s\"\n", shader_filenames[waiting_resources[i]]);
                    *((Shader *)resources[waiting_resources[i]]) = init_shader_from_data(data[0], data_len[0],
                                                                                         data[1], data_len[1],
                                                                                         data[2], data_len[2]);
                    break;
                }
                case TEXTURE: {
                    *((Texture *)resources[waiting_resources[i]]) = init_texture_from_data(data[0], data_len[0]);
                    break;
                }
                case FONT: {
                    *((Font *)resources[waiting_resources[i]]) = init_font_from_data(data[0], data_len[0],
                                                                                     data[1], data_len[1]);
                    break;
                }
                case SOUND: {
                    const char *sound_filename = resource_filenames[file_start_index_by_id(waiting_resources[i])];
                    if(filename_extension_cmp(sound_filename, "wav")) {
                        *((Sound *)resources[waiting_resources[i]]) = init_sound_from_wav_data(data[0], data_len[0]);
                    }
                    else if(filename_extension_cmp(sound_filename, "ogg")) {
                        *((Sound *)resources[waiting_resources[i]]) = init_sound_from_ogg_data(data[0], data_len[0]);
                    }
                    break;
                }
                default: break;
            }

            for(i8 j = 0; j < resource_type_file_count(type); j++) {
                free(data[j]);
            }

            memmove(waiting_resources + i,
                    waiting_resources + i + 1,
                    (waiting_resource_count - i - 1) * sizeof(i16));

            --waiting_resource_count;
        }
    }
}

#undef R
#undef L
#undef C
