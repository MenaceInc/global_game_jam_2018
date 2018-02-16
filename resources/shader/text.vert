#version 330 core

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

in vec3 in_position;
in vec2 in_uv;

out vec3 position;
out vec2 uv;

void main() {
	uv = in_uv;
	position = in_position.xyz;
	gl_Position = projection * view * model * vec4(in_position, 1.0);
}
