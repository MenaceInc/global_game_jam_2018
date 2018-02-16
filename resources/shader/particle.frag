#version 330 core

in vec2 uv;
in float progress;
out vec4 color;

uniform sampler2D tex;
uniform vec2 uv_offset;
uniform vec2 uv_range;
uniform vec4 clip;
uniform vec4 tint;

void main() {
    if(gl_FragCoord.x >= clip.x && gl_FragCoord.x <= clip.z &&
       gl_FragCoord.y >= clip.y && gl_FragCoord.y <= clip.w) {
        float alpha = 1-progress;

		color = alpha*texture(tex, (uv * uv_range) + uv_offset);
		color *= tint;
    }
    else {
        discard;
    }
}
