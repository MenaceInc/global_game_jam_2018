#version 330 compatibility

in vec3 position;
in vec2 uv;

out vec4 color;

uniform sampler2D tex;
uniform vec2 uv_offset;
uniform vec2 uv_range;
uniform vec4 clip;
uniform vec4 tint;

void main() {
    if(gl_FragCoord.x >= clip.x &&
	   gl_FragCoord.x <= clip.z &&
	   gl_FragCoord.y >= clip.y &&
	   gl_FragCoord.y <= clip.w) {
        color = texture(tex, uv);
		color *= tint;
    }
    else {
        discard;
    }
}
