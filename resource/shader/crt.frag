#version 330 compatibility

in vec3 position;
in vec2 uv;

out vec4 color;

uniform sampler2D tex;
uniform vec2 uv_offset;
uniform vec2 uv_range;
uniform vec4 clip;
uniform vec4 tint;
uniform float sin_pos;

void main() {
	float sin_offset_1 = sin(sin_pos + uv.y*50)*0.0005,
		  sin_offset_2 = sin(sin_pos + uv.y*30)*0.0009,
		  round_offset = sin(uv.x*3.14159)*0.03;
    if(gl_FragCoord.x >= clip.x &&
	   gl_FragCoord.x <= clip.z &&
	   gl_FragCoord.y >= clip.y &&
	   gl_FragCoord.y <= clip.w) {

		color = texture(tex, uv+vec2(sin_offset_1, -round_offset));
		color.r = texture(tex, uv+vec2(sin_offset_2, -round_offset)).r;
		color *= tint;
		color.b += (sin(uv.y*400)*sin(uv.y*400))*((color.a+0.1)/4);
	}
    else {
        discard;
    }
}
