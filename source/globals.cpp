#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <AL/al.h>
#include <AL/alc.h>
#include <AL/efx.h>
#include <AL/efx-creative.h>
#include <AL/EFX-Util.h>
#include "globals.h"
#include "ext/rf_darray.h"
#include "audio.h"

#define HANDMADE_MATH_IMPLEMENTATION
#define HANDMADE_MATH_STATIC

#include "ext/HandmadeMath.h"

#define CONFIG_PATH     "./resource/cfg/"

global GLFWwindow *window = NULL;
global i32 window_w = 0, window_h = 0;
global r64 mouse_x = 0, mouse_y = 0;
global i8 state_fade_out = 0;
// mouse_state bytes:
// left mouse down
// left mouse pressed
// right mouse down
// right mouse pressed
global i8 mouse_state = 0;
global i8 key_down[GLFW_KEY_LAST] = { 0 },
          key_pressed[GLFW_KEY_LAST] = { 0 };
global u8 last_char = 0;
global i16 last_key = 0, last_char_mods = 0;
global i8 mouse_buttons_used = 0,
          mouse_position_used = 0,
          keyboard_used = 0;
global i32 gamepad_axis_count = 0, gamepad_button_count = 0;
global const r32 *gamepad_axes = NULL;
global const u8 *gamepad_button_states = NULL;
global u8 last_gamepad_button_states[16];

global ALCdevice *audio_device;
global ALCcontext *audio_context;

#define left_mouse_down     (mouse_state & (1 << 7))
#define left_mouse_pressed  (mouse_state & (1 << 6))
#define right_mouse_down    (mouse_state & (1 << 5))
#define right_mouse_pressed (mouse_state & (1 << 4))

#define joystick_1_x        (gamepad_axes ? gamepad_axes[0] : 0)
#define joystick_1_y        (gamepad_axes && gamepad_axis_count > 1 ? gamepad_axes[1] : 0)
#define joystick_2_x        (gamepad_axes && gamepad_axis_count > 2 ? gamepad_axes[2] : 0)
#define joystick_2_y        (gamepad_axes && gamepad_axis_count > 3 ? gamepad_axes[3] : 0)

#define gamepad_button_down(i)      (gamepad_button_count > i && gamepad_button_states ? gamepad_button_states[i] == GLFW_PRESS : 0)
#define gamepad_button_pressed(i)   (gamepad_button_count > i && gamepad_button_states ? \
                                     gamepad_button_states[i] == GLFW_PRESS && last_gamepad_button_states[i] != GLFW_PRESS : 0)

#define PI 3.1415926535897
#define SNAP_SIZE 16

#define audio_type_volume(i) (audio_type_volumes[i] * audio_type_volumes[AUDIO_MASTER])
#define key_control_down(i)  (key_down[key_control_maps[i]])
#define key_control_pressed(i) (key_pressed[key_control_maps[i]])

#define percent_difference(v1, v2) (fabs(v1 - v2) / v1)

global
const char *key_control_names[MAX_KEY_CONTROL] = {
    "Move Up",
    "Move Left",
    "Move Down",
    "Move Right",
    "Suicide",
    "Spawn",
};

global
i16 key_control_maps[MAX_KEY_CONTROL] = {
    KEY_W,
    KEY_A,
    KEY_S,
    KEY_D,
    KEY_DELETE,
    KEY_E,
};

global
const char *gamepad_control_names[MAX_GP_CONTROL] = {

};

global
i16 gamepad_control_maps[MAX_GP_CONTROL] = {

};

global
const char *audio_type_names[MAX_AUDIO] = {
    "Master",
    "Music",
    "User Interface",
    "Entities",
};

global
r32 audio_type_volumes[MAX_AUDIO] = {
    1,
    1,
    1,
    1,
};

global
i8 fullscreen = 0;

global FILE *log_file;

const char *key_name(u16 key) {
    switch(key) {
        case KEY_SPACE: { return "Space"; }
        case KEY_APOSTROPHE: { return "'"; }
        case KEY_COMMA: { return ","; }
        case KEY_MINUS: { return "-"; }
        case KEY_PERIOD: { return "."; }
        case KEY_SLASH: { return "/"; }
        case KEY_0: { return "0"; }
        case KEY_1: { return "1"; }
        case KEY_2: { return "2"; }
        case KEY_3: { return "3"; }
        case KEY_4: { return "4"; }
        case KEY_5: { return "5"; }
        case KEY_6: { return "6"; }
        case KEY_7: { return "7"; }
        case KEY_8: { return "8"; }
        case KEY_9: { return "9"; }
        case KEY_SEMICOLON: { return ";"; }
        case KEY_EQUAL: { return "="; }
        case KEY_A: { return "A"; }
        case KEY_B: { return "B"; }
        case KEY_C: { return "C"; }
        case KEY_D: { return "D"; }
        case KEY_E: { return "E"; }
        case KEY_F: { return "F"; }
        case KEY_G: { return "G"; }
        case KEY_H: { return "H"; }
        case KEY_I: { return "I"; }
        case KEY_J: { return "J"; }
        case KEY_K: { return "K"; }
        case KEY_L: { return "L"; }
        case KEY_M: { return "M"; }
        case KEY_N: { return "N"; }
        case KEY_O: { return "O"; }
        case KEY_P: { return "P"; }
        case KEY_Q: { return "Q"; }
        case KEY_R: { return "R"; }
        case KEY_S: { return "S"; }
        case KEY_T: { return "T"; }
        case KEY_U: { return "U"; }
        case KEY_V: { return "V"; }
        case KEY_W: { return "W"; }
        case KEY_X: { return "X"; }
        case KEY_Y: { return "Y"; }
        case KEY_Z: { return "Z"; }
        case KEY_LEFT_BRACKET: { return "["; }
        case KEY_BACKSLASH: { return "\\"; }
        case KEY_RIGHT_BRACKET: { return "]"; }
        case KEY_GRAVE_ACCENT: { return "`"; }
        case KEY_WORLD_1: { return "World 1"; }
        case KEY_WORLD_2: { return "World 2"; }
        case KEY_ESCAPE: { return "Escape"; }
        case KEY_ENTER: { return "Enter"; }
        case KEY_TAB: { return "Tab"; }
        case KEY_BACKSPACE: { return "Backspace"; }
        case KEY_INSERT: { return "Insert"; }
        case KEY_DELETE: { return "Delete"; }
        case KEY_RIGHT: { return "Right"; }
        case KEY_LEFT: { return "Left"; }
        case KEY_UP: { return "Up"; }
        case KEY_DOWN: { return "Down"; }
        case KEY_PAGE_UP: { return "Page Up"; }
        case KEY_PAGE_DOWN: { return "Page Down"; }
        case KEY_HOME: { return "Home"; }
        case KEY_END: { return "End"; }
        case KEY_CAPS_LOCK: { return "Caps Lock"; }
        case KEY_SCROLL_LOCK: { return "Scroll Lock"; }
        case KEY_NUM_LOCK: { return "Num Lock"; }
        case KEY_PRINT_SCREEN: { return "Print Screen"; }
        case KEY_PAUSE: { return "Pause"; }
        case KEY_F1: { return "F1"; }
        case KEY_F2: { return "F2"; }
        case KEY_F3: { return "F3"; }
        case KEY_F4: { return "F4"; }
        case KEY_F5: { return "F5"; }
        case KEY_F6: { return "F6"; }
        case KEY_F7: { return "F7"; }
        case KEY_F8: { return "F8"; }
        case KEY_F9: { return "F9"; }
        case KEY_F10: { return "F10"; }
        case KEY_F11: { return "F11"; }
        case KEY_F12: { return "F12"; }
        case KEY_F13: { return "F13"; }
        case KEY_F14: { return "F14"; }
        case KEY_F15: { return "F15"; }
        case KEY_F16: { return "F16"; }
        case KEY_F17: { return "F17"; }
        case KEY_F18: { return "F18"; }
        case KEY_F19: { return "F19"; }
        case KEY_F20: { return "F20"; }
        case KEY_F21: { return "F21"; }
        case KEY_F22: { return "F22"; }
        case KEY_F23: { return "F23"; }
        case KEY_F24: { return "F24"; }
        case KEY_F25: { return "F25"; }
        case KEY_KP_0: { return "0"; }
        case KEY_KP_1: { return "1"; }
        case KEY_KP_2: { return "2"; }
        case KEY_KP_3: { return "3"; }
        case KEY_KP_4: { return "4"; }
        case KEY_KP_5: { return "5"; }
        case KEY_KP_6: { return "6"; }
        case KEY_KP_7: { return "7"; }
        case KEY_KP_8: { return "8"; }
        case KEY_KP_9: { return "9"; }
        case KEY_KP_DECIMAL: { return "Decimal"; }
        case KEY_KP_DIVIDE: { return "Divide"; }
        case KEY_KP_MULTIPLY: { return "Multiply"; }
        case KEY_KP_SUBTRACT: { return "Subtract"; }
        case KEY_KP_ADD: { return "Add"; }
        case KEY_LEFT_SHIFT: { return "Left Shift"; }
        case KEY_LEFT_CONTROL: { return "Left Control"; }
        case KEY_LEFT_ALT: { return "Left Alt"; }
        case KEY_LEFT_SUPER: { return "Left Super"; }
        case KEY_RIGHT_SHIFT: { return "Right Shift"; }
        case KEY_RIGHT_CONTROL: { return "Right Control"; }
        case KEY_RIGHT_ALT: { return "Right Alt"; }
        case KEY_RIGHT_SUPER: { return "Right Super"; }
        case KEY_MENU: { return "Menu"; }
        default: break;
    }
    return "";
}

r32 distance_32(r32 x1, r32 y1, r32 x2, r32 y2) {
    return sqrt((x2-x1)*(x2-x1) + (y2-y1)*(y2-y1));
}

r32 distance2_32(r32 x1, r32 y1, r32 x2, r32 y2) {
    return ((x2-x1)*(x2-x1) + (y2-y1)*(y2-y1));
}

r32 random(r32 low, r32 high) {
    r32 val = low + ((r32)(((int)rand() % 1000) / 1000.f) * (high-low));
    return val;
}

i8 file_exists(const char *filename) {
    FILE *file;
    if((file = fopen(filename, "r"))) {
        fclose(file);
        return 1;
    }
    return 0;
}

i32 num_of_char(const char *text, char c) {
    i32 count = 0;
    for(u32 i = 0; i < strlen(text); ++i) {
        if(text[i] == c) { ++count; }
    }
    return count;
}

i8 filename_extension_cmp(const char *filename, const char *extension) {
    u32 cmp_pos = 0;
    for(u32 i = 0; i < strlen(filename); i++) {
        if(filename[i] == '.') {
            cmp_pos = i+1;
        }
    }

    return(!strcmp(filename + cmp_pos, extension));
}

void load_settings() {
    char settings_filename[64] = { 0 };
    snprintf(settings_filename, 63, "%s%s", CONFIG_PATH, "settings.cfg");

    FILE *file = fopen(settings_filename, "r");

    if(file) {
        i32 file_size = 0;
        fseek(file, 0, SEEK_END);
        file_size = ftell(file);
        rewind(file);

        char *buffer = (char *)calloc(file_size, 1);
        fread(buffer, 1, file_size, file);

        enum {
            READ_NONE,
            READ_NEED_NEWLINE,
            READ_NAME,
            READ_WAIT_VAL,
            READ_VAL,
        };

        i8 read_mode = READ_NONE;
        i32 read_pos = 0;
        char name[32] = { 0 },
             value[32] = { 0 };

        for(i32 i = 0; i < file_size;) {
            switch(read_mode) {
                case READ_NONE: {
                    if(buffer[i] == '#') {
                        read_mode = READ_NEED_NEWLINE;
                        ++i;
                    }
                    else if(isalnum(buffer[i]) || buffer[i] == '[') {
                        read_mode = READ_NAME;
                        read_pos = i;
                    }
                    else {
                        ++i;
                    }
                    break;
                }
                case READ_NEED_NEWLINE: {
                    if(buffer[i] == '\n') {
                        read_mode = READ_NONE;
                    }
                    ++i;
                    break;
                }
                case READ_NAME: {
                    if(buffer[i] == ':') {
                        memset(name, 0, 32);
                        strncpy(name, buffer + read_pos, i-read_pos);
                        name[31] = 0;
                        read_mode = READ_WAIT_VAL;
                    }
                    ++i;
                    break;
                }
                case READ_WAIT_VAL: {
                    if(isalnum(buffer[i])) {
                        read_mode = READ_VAL;
                        read_pos = i;
                    }
                    else {
                        ++i;
                    }
                    break;
                }
                case READ_VAL: {
                    if(!isalnum(buffer[i]) && buffer[i] != '.') {
                        memset(value, 0, 32);
                        strncpy(value, buffer + read_pos, i-read_pos);
                        value[31] = 0;

                        i8 set = 0;

                        for(i16 j = 0; j < MAX_KEY_CONTROL; j++) {
                            char read_name[64] = { 0 };
                            sprintf(read_name, "[Keyboard] %s", key_control_names[j]);
                            if(!strcmp(read_name, name)) {
                                key_control_maps[j] = atoi(value);
                                set = 1;
                                break;
                            }
                        }

                        if(!set) {
                            for(i16 j = 0; j < MAX_GP_CONTROL; j++) {
                                char read_name[64] = { 0 };
                                sprintf(read_name, "[Gamepad] %s", gamepad_control_names[j]);
                                if(!strcmp(read_name, name)) {
                                    gamepad_control_maps[j] = atoi(value);
                                    set = 1;
                                    break;
                                }
                            }
                            if(!set) {
                                for(i16 j = 0; j < MAX_AUDIO; j++) {
                                    if(!strcmp(audio_type_names[j], name)) {
                                        audio_type_volumes[j] = atof(value);
                                        set = 1;
                                        break;
                                    }
                                }

                                if(!set) {
                                    if(!strcmp(name, "Fullscreen")) {
                                        fullscreen = atoi(value);
                                        set = 1;
                                    }
                                }
                            }
                        }

                        read_mode = READ_NONE;
                    }
                    else {
                        ++i;
                    }
                    break;
                }
                default: {
                    ++i;
                    break;
                }
            }
        }

        free(buffer);

        fclose(file);
    }
    else {
        fprintf(log_file, "ERROR: could not open \"%s\"\n", settings_filename);
    }
}

void save_settings() {
    char settings_filename[64] = { 0 };
    snprintf(settings_filename, 63, "%s%s", CONFIG_PATH, "settings.cfg");

    FILE *file = fopen(settings_filename, "w");

    if(file) {
        fprintf(file, "# *** SETTINGS CONFIGURATION *** #\n\n");

        { // controls
            fprintf(file, "# Keyboard Controls\n");

            for(i16 i = 0; i < MAX_KEY_CONTROL; i++) {
                fprintf(file, "[Keyboard] %s: %i\n", key_control_names[i], key_control_maps[i]);
            }

            fprintf(file, "\n# Gamepad Controls\n");

            for(i16 i = 0; i < MAX_GP_CONTROL; i++) {
                fprintf(file, "[Gamepad] %s: %i\n", gamepad_control_names[i], gamepad_control_maps[i]);
            }
        }

        { // audio
            fprintf(file, "\n# Audio\n");

            for(i16 i = 0; i < MAX_AUDIO; i++) {
                fprintf(file, "%s: %f\n", audio_type_names[i], audio_type_volumes[i]);
            }
        }

        { // graphics
            fprintf(file, "\n# Graphics\n");
        }

        { // screen
            fprintf(file, "\n# Screen\n");

            fprintf(file, "Fullscreen: %i\n", fullscreen);
        }

        fclose(file);
    }
    else {
        fprintf(log_file, "ERROR: could not open \"%s\"\n", settings_filename);
    }
}

static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if((action == GLFW_PRESS && last_key != key) || action == GLFW_REPEAT) {
        last_key = key;
    }
    else {
        last_key = 0;
    }
}

static void charmods_callback(GLFWwindow *window, unsigned int code_point, int mods) {
    last_char = code_point;
    last_char_mods = mods;
}
