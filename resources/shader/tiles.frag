#version 330 compatibility

in vec2 uv;
in vec2 uv_offset;
out vec4 color;

uniform sampler2D tex;
uniform vec2 tex_size;
uniform vec4 clip;
uniform vec4 tint;

void main() {
    if(gl_FragCoord.x >= clip.x && gl_FragCoord.x <= clip.z &&
       gl_FragCoord.y >= clip.y && gl_FragCoord.y <= clip.w) {
		vec2 uv_range = vec2(8 / tex_size.x, 8 / tex_size.y);
		color = texture(tex, (uv * uv_range) + vec2(uv_offset.x / tex_size.x, uv_offset.y / tex_size.y));
		color *= tint;
    }
    else {
        discard;
    }
}
