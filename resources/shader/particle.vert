#version 330 core

in vec3 in_position;
in vec2 in_uv;
in mat4 model;
in float particle_progress;

out vec2 uv;

out float progress;

uniform mat4 projection;
uniform mat4 view;

void main() {
    gl_Position = projection * view * model * vec4(in_position, 1.0);
	progress = particle_progress;
	uv = in_uv;
}
