#version 330 compatibility

struct Light {
	vec4 attributes; // x, y, rad, intensity
	vec3 color;
};

in vec3 position;
in vec2 uv;

out vec4 color;

uniform sampler2D tex;
uniform vec2 uv_offset;
uniform vec2 uv_range;
uniform vec4 clip;
uniform vec4 tint;
uniform float sin_pos;
uniform Light lights[16];
uniform int light_count;

void main() {
	float sin_offset_1 = sin(sin_pos + uv.y*50)*0.0005,
		  sin_offset_2 = sin(sin_pos + uv.y*30)*0.0009,
		  round_offset = sin(uv.x*3.14159)*0.03;
    if(gl_FragCoord.x >= clip.x &&
	   gl_FragCoord.x <= clip.z &&
	   gl_FragCoord.y >= clip.y &&
	   gl_FragCoord.y <= clip.w) {
		
		color = texture(tex, uv+vec2(sin_offset_1, -round_offset)) * 0.2;
		
		float distance = 0;
		float factor = 0;
		float default_light = 0.2;
		vec3 color_change = vec3(0, 0, 0);
		
		for(int i = 0; i < light_count && i < 16; i++) {
			distance = length(position.xy - lights[i].attributes.xy);
			factor = (1.0 - (distance / lights[i].attributes.z));
			if(factor < 0) {
				factor = 0;
			}
			color_change += (factor * lights[i].color) * lights[i].attributes.w;
		}

        if(color_change.r < default_light) { color_change.r = default_light; }
        if(color_change.g < default_light) { color_change.g = default_light; }
        if(color_change.b < default_light) { color_change.b = default_light; }

        color *= vec4(color_change, 1);
	   
		color.r = texture(tex, uv+vec2(sin_offset_2, -round_offset)).r;
		color *= tint;
		color.b += sin(uv.y*400)*sin(uv.y*400);
	}
    else {
        discard;
    }
}
