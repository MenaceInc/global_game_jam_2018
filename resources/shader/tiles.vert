#version 330 core

in vec3 in_position;
in vec2 in_uv;
in vec4 tile_data;

out vec2 uv;
out vec2 uv_offset;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main() {
    gl_Position = projection * view * model * vec4(in_position + vec3(tile_data.x/8, tile_data.y/8, 0), 1.0);
	uv = in_uv;
	uv_offset = tile_data.zw;
}
