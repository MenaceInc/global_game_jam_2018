/*
           Code-Base Convention Notes
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * this project is compiled as a unity-build.
   that means there is only ONE compilation
   unit; eventually, everything will be
   included into THIS file at some point.
   the makefile takes care of this

 * types are stylized as UpperCamelCase

 * objects/variables are lower_case

 * global constants are CAPS_CASE

 * in general, <name>_t implies some sort of
   floating point transition variable. these
   are used to smoothly transition between
   different states of something

 * there are some standard typedefs and
   definitions that are used. for example,
   "global" is #defined as "static", just
   because it's a nicer word to work with.
   additionally, convention is to use
   fixed-size variables. there are some typedefs
   to help with this: e.g. u16 instead of uint16_t,
   r32 instead of float, r64 instead of double,
   etc.
*/

#include <time.h>

#include "globals.cpp"
#include "audio.cpp"
#include "resources.cpp"
#include "draw.cpp"

global FBO crt_render;

#include "ui.cpp"
#include "state.cpp"

int main() {
    // open log stream. currently, just setting it to stdout (standard output),
    // so we see all log output in the console.

    log_file = stdout;//fopen("run.log", "w");

    // initialize GLFW. glfwInit() returns nonzero on success
    if(glfwInit()) {
        fprintf(log_file, "GLFW successfully initialized\n");

        // load settings from config file. this is loaded before window creation
        // so we can load settings like fullscreen/resolution/etc.
        load_settings();

        // create window that:
        //     * is resizable
        //     * has resolution of 1024x600
        glfwWindowHint(GLFW_RESIZABLE, 1);
        window_w = 1024;
        window_h = 600;
        clip = HMM_Vec4(0, 0, window_w, window_h);
        window = glfwCreateWindow(window_w, window_h, "Global Game Jam 2018",
                                  fullscreen ? glfwGetPrimaryMonitor() : NULL, NULL);

        // window is a pointer, will be nonzero if successfully created
        if(window) {
            fprintf(log_file, "GLFW window successfully created\n");
            // make OpenGL context of created window current
            glfwMakeContextCurrent(window);
            // no vsync
            glfwSwapInterval(0);
            // set GLFW key callback function- will be called when a key is pressed
            glfwSetKeyCallback(window, key_callback);
            // set GLFW char callback function- will be called when a character is typed
            glfwSetCharModsCallback(window, charmods_callback);
            // get starting cursor position, storing it in mouse_x and mouse_y
            glfwGetCursorPos(window, &mouse_x, &mouse_y);

            // center the window
            {
                const GLFWvidmode *mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
                glfwSetWindowPos(window, (mode->width - window_w)/2, (mode->height - window_h)/2);
            }

            // glewInit() returns 0 on success
            if(!glewInit()) {
                fprintf(log_file, "GLEW successfully initialized\n\n");
                // initialize resource management data (multithreaded resource loading)
                init_resources();
                fprintf(log_file, "resource loading data initialized\n\n");

                model = HMM_Mat4d(1);
                view = HMM_Mat4d(1);
                projection = HMM_Orthographic(0.f, (r32)window_w, (r32)window_h, 0.f, -1.f, 1.f);

                // set default OpenGL options
                glEnable(GL_TEXTURE_2D);
                glDisable(GL_CULL_FACE);
                glAlphaFunc(GL_GREATER, 1);
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

                // generate quad vertex buffer for the graphics hardware
                glGenBuffers(1, &quad_vertex_buffer);
                glBindBuffer(GL_ARRAY_BUFFER, quad_vertex_buffer);
                glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(GLfloat), quad_vertices, GL_STATIC_DRAW);

                // generate uv vertex buffer for the graphics hardware
                glGenBuffers(1, &quad_uv_buffer);
                glBindBuffer(GL_ARRAY_BUFFER, quad_uv_buffer);
                glBufferData(GL_ARRAY_BUFFER, 8 * sizeof(GLfloat), quad_uvs, GL_STATIC_DRAW);

                // generate line vertex buffer for the graphics hardware
                glGenBuffers(1, &line_vertex_buffer);
                glBindBuffer(GL_ARRAY_BUFFER, line_vertex_buffer);
                glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(GLfloat), line_vertices, GL_STATIC_DRAW);

                // reset bound OpenGL buffer
                glBindBuffer(GL_ARRAY_BUFFER, 0);

                // load in default shaders.
                // NOTE(ryan): these are not requested to be loaded by the multithreaded
                //             system because they're common enough that they should
                //             pretty much always be loaded. also, they're used by
                //             drawing functions as fallback shaders.

                rect_shader = load_shader("resource/shader/rectangle.vert",
                                          "resource/shader/rectangle.frag",
                                          "resource/shader/rectangle.info");

                filled_rect_shader = load_shader("resource/shader/filled_rectangle.vert",
                                                 "resource/shader/filled_rectangle.frag",
                                                 "resource/shader/filled_rectangle.info");

                line_shader = load_shader("resource/shader/line.vert",
                                          "resource/shader/line.frag",
                                          "resource/shader/line.info");

                texture_shader = load_shader("resource/shader/texture.vert",
                                             "resource/shader/texture.frag",
                                             "resource/shader/texture.info");

                text_shader = load_shader("resource/shader/text.vert",
                                          "resource/shader/text.frag",
                                          "resource/shader/text.info");

                fprintf(log_file, "rendering data initialized\n");

                // initialize UI data
                init_ui();
                fprintf(log_file, "ui data initialized\n\n");

                // NOTE(ryan): last_time is used to regulate the FPS of the program.
                //             last_time will store the result of glfwGetTime(), then
                //             the program can wait until glfwGetTime() returns a result
                //             that is 1/60 of a second higher than last_time (thereby
                //             preventing each game loop from taking less than 1/60 of a
                //             second)
                r64 last_time;

                r32 crt_sin_pos = 0;
                crt_render = init_fbo(640, 480);
                i8 last_fullscreen = fullscreen;

                // seed RNG with time
                // NOTE(ryan): this might need to be replaced if we see bad results.
                //             srand/rand aren't that great and are pretty inefficient
                //             but they should get the job done for now.
                srand((unsigned int)time(NULL));

                // request some resources that probably always need to be loaded
                // (UI texture, base/title fonts)
                request_shader(SHADER_CRT);
                request_texture(TEX_UI);
                request_font(FONT_BASE);
                request_font(FONT_TITLE);

                // first state should be the splash screen
                state = init_splash();

                // next_state.type should not be nonzero until we want to change
                // the program's state. STATE_NULL == 0
                next_state.type = STATE_NULL;

                // we're looping until glfwWindowShouldClose(window) returns true,
                // or when the X button is clicked, Alt+F4, etc.
                while(!glfwWindowShouldClose(window)) {
                    // initialize audio device/context if they're not already.
                    // NOTE(ryan): we do this inside the loop because it might
                    //             fail initially, but that just means we should
                    //             probably silently continue. maybe the user
                    //             doesn't have their headphones plugged in yet
                    //             which is why there is no device able to be
                    //             accessed yet, so we'll just keep trying until
                    //             there is a device that can be accessed.

                    if(!audio_device && !audio_context) {
                        audio_device = alcOpenDevice(alcGetString(NULL, ALC_DEFAULT_DEVICE_SPECIFIER));
                        if(audio_device) {
                            fprintf(log_file, "audio device opened\n");
                            const ALint attribute_list[] = {
                                ALC_FREQUENCY, 44100,
                                0
                            };
                            audio_context = alcCreateContext(audio_device, attribute_list);
                            if(alcMakeContextCurrent(audio_context)) {
                                fprintf(log_file, "OpenAL context successfully established\n");
                                set_listener_position(0, 0, 0);
                                init_audio();
                            }
                            else {
                                fprintf(log_file, "ERROR [openal]: audio context establishment failed (%i)\n", alGetError());
                            }
                        }
                        else {
                            fprintf(log_file, "ERROR [openal]: audio device initialization failed (%i)\n", alGetError());
                        }
                    }

                    // we'll get last_time at the /beginning/ of the frame, so the frame's time can be
                    // taken up by the actual logic of the game
                    last_time = glfwGetTime();
                    // reset input stuff
                    last_key = 0;
                    last_char = 0;
                    mouse_buttons_used = 0;
                    mouse_position_used = 0;
                    keyboard_used = 0;
                    // glfwPollEvents() updates the window and records input
                    glfwPollEvents();
                    // we'll actively update the window size, as we allow resizing
                    glfwGetWindowSize(window, &window_w, &window_h);
                    clip = HMM_Vec4(0, 0, window_w, window_h);

                    // update the mouse position
                    glfwGetCursorPos(window, &mouse_x, &mouse_y);

                    // get the mouse button states for both right/left click
                    i32 left_mouse_button_state = glfwGetMouseButton(window, 0),
                        right_mouse_button_state = glfwGetMouseButton(window, 1);

                    // update the bits of mouse_state accordingly. comments on this can be found in globals.cpp
                    (left_mouse_button_state && !(mouse_state & (1 << 7))) ? mouse_state |= (1 << 6) : mouse_state &= ~(1 << 6);
                    left_mouse_button_state ? mouse_state |= (1 << 7) : mouse_state &= ~(1 << 7);

                    (right_mouse_button_state && !(mouse_state & (1 << 5))) ? mouse_state |= (1 << 4) : mouse_state &= ~(1 << 4);
                    right_mouse_button_state ? mouse_state |= (1 << 5) : mouse_state &= ~(1 << 5);

                    // update key states
                    for(i16 i = 0; i < GLFW_KEY_LAST; i++) {
                        i32 key_state = glfwGetKey(window, i);
                        if(key_state == GLFW_PRESS) {
                            key_pressed[i] = !key_down[i];
                            key_down[i] = 1;
                        }
                        else {
                            key_down[i] = 0;
                            key_pressed[i] = 0;
                        }
                    }

                    // update gamepad input
                    for(i8 i = 0; i < 16 && i < gamepad_button_count; i++) {
                        last_gamepad_button_states[i] = gamepad_button_states[i];
                    }
                    gamepad_axes = glfwGetJoystickAxes(0, &gamepad_axis_count);
                    gamepad_button_states = glfwGetJoystickButtons(0, &gamepad_button_count);

                    update_audio();

                    // clear the screen before each frame
                    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
                    glClearColor(0, 0, 0, 1);
                    glViewport(0, 0, window_w, window_h);
                    {
                        // update the state
                        clear_fbo(&crt_render);

                        bind_fbo(&crt_render);
                        {
                            draw_filled_rect(0, 0, 0, 1, 0, 0, CRT_W, CRT_H);
                        }
                        bind_fbo(NULL);

                        ui_begin();
                        {
                            update_state();
                        }
                        bind_fbo(&crt_render);
                        ui_end();

                        // this rectangle is just used for fading in/out between state changes. the
                        // transparency is state_t (or, when state_t is pretty close to 1, it is just
                        // 1, to ensure that the screen is totally black between state changes)
                        draw_filled_rect(0, 0, 0, state_t < 0.99 ? state_t : 1, 0, 0, window_w, window_h);
                        bind_fbo(NULL);

                        {
                            r32 w = 4 * (window_h / 3.f),
                                h = window_h;
                            crt_sin_pos += 0.7;
                            active_shader = shaders[SHADER_CRT].id;
                            glUseProgram(active_shader);
                            glUniform1f(glGetUniformLocation(active_shader, "sin_pos"), crt_sin_pos);
                            draw_scaled_fbo(&crt_render, 0, window_w/2 - w/2, 0, w, h);
                            active_shader = 0;
                            glUseProgram(active_shader);
                        }
                    }
                    // swap buffers (update screen)
                    glfwSwapBuffers(window);

                    // check for state change
                    if(next_state.type || state_fade_out) {
                        state_t += (1 - state_t) * 0.3;
                        if(next_state.type) {
                            if(state_t >= 0.99) {
                                clean_up_state();
                                state.type = next_state.type;
                                state.memory = next_state.memory;
                                init_state_heavy();
                                next_state.type = STATE_NULL;
                                next_state.memory = NULL;
                            }
                        }
                    }
                    else {
                        update_resources();
                        if(!waiting_resource_count) {
                            state_t *= 0.9;
                        }
                    }

                    // check for any opengl errors
                    {
                        GLenum error = glGetError();
                        if(error) {
                            fprintf(log_file, "ERROR [opengl]: %i\n", error);
                        }
                    }

                    // check for any openal errors
                    {
                        ALenum error = alGetError();
                        if(error) {
                            fprintf(log_file, "ERROR [openal]: %i\n", error);

                        }
                    }

                    // check for fullscreen state change
                    if(key_pressed[KEY_F11] || fullscreen != last_fullscreen) {
                        if(key_pressed[KEY_F11]) {
                            fullscreen = !fullscreen;
                        }
                        // make appropriate window changes
                        GLFWmonitor *monitor = glfwGetWindowMonitor(window) ? NULL : glfwGetPrimaryMonitor();
                        glfwWindowHint(GLFW_RESIZABLE, 1);
                        glfwSetWindowMonitor(window, monitor, 0, 0, window_w, window_h, GLFW_DONT_CARE);
                        last_fullscreen = fullscreen;
                    }

                    // wait until this frame's time is done
                    while(glfwGetTime() < last_time + 1/FPS);
                }

                // the game is done, save all the current settings
                save_settings();

                // clean up the state
                clean_up_state();

                clean_up_fbo(&crt_render);

                // unrequest the resources we requested
                unrequest_font(FONT_TITLE);
                unrequest_font(FONT_BASE);
                unrequest_texture(TEX_UI);
                unrequest_shader(SHADER_CRT);

                // clean up default shaders
                clean_up_shader(&text_shader);
                clean_up_shader(&texture_shader);
                clean_up_shader(&line_shader);
                clean_up_shader(&filled_rect_shader);
                clean_up_shader(&rect_shader);

                // delete the graphics hardware buffers that were allocated before
                glDeleteBuffers(1, &quad_vertex_buffer);
                glDeleteBuffers(1, &quad_uv_buffer);
                glDeleteBuffers(1, &line_vertex_buffer);

                // clean up all the resources data
                clean_up_resources();
            }
            else {
                fprintf(log_file, "ERROR [glew]: GLEW initialization failed\n");
            }
            glfwDestroyWindow(window);
        }
        else {
            fprintf(log_file, "ERROR [glfw]: GLFW window creation failed\n");
        }
    }
    else {
        fprintf(log_file, "ERROR [glfw]: GLFW initialization failed (%i)\n", glGetError());
    }

    // if audio was ever initialized, now is the time when we'll clean it up :)
    if(audio_device && audio_context) {
        clean_up_audio();
        alcMakeContextCurrent(NULL);
        alcDestroyContext(audio_context);
        alcCloseDevice(audio_device);
    }

    return 0;
}
