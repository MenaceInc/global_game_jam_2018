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
		  sin_offset_3 = sin(sin_pos + uv.x*60)*0.0001,
		  round_offset = sin(uv.x*3.14159)*0.03;
    if(gl_FragCoord.x >= clip.x &&
	   gl_FragCoord.x <= clip.z &&
	   gl_FragCoord.y >= clip.y &&
	   gl_FragCoord.y <= clip.w) {

		color = texture(tex, uv+vec2(sin_offset_1, -round_offset));
		color.r = texture(tex, uv+vec2(sin_offset_2, -round_offset)).r;
		color *= tint;
		
		float crt_wave_val = sin((uv.y + sin_offset_3)*400);
		crt_wave_val *= crt_wave_val;
		color.r += crt_wave_val*((color.a+0.1)/12);
		color.g += crt_wave_val*((color.a+0.1)/8);
		color.b += crt_wave_val*((color.a+0.1)/4);
	}
    else {
        discard;
    }
}
