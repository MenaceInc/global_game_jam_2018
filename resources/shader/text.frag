#version 330 core

in vec3 position;
in vec2 uv;

out vec4 color;

uniform sampler2D tex;
uniform vec2 uv_offset;
uniform vec2 uv_range;
uniform vec4 clip;
uniform vec4 tint;
uniform vec4 text_color;
uniform float boldness;
uniform float softness;

void main() {
    if(gl_FragCoord.x >= clip.x &&
	   gl_FragCoord.x <= clip.z &&
	   gl_FragCoord.y >= clip.y &&
	   gl_FragCoord.y <= clip.w) {
		float distance = texture(tex, (uv * uv_range) + uv_offset).a;
		float smooth_step = smoothstep(1.0 - boldness, (1.0 - boldness) + softness, distance);
		color = text_color * tint * smooth_step;
    }
    else {
        discard;
    }
}
