#version 330 compatibility

struct Light {
	vec4 attributes; // x, y, radius, intensity
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

uniform Light lights[16];
uniform int light_count;
uniform vec2 camera_pos;
uniform float default_light;

void main() {
    if(gl_FragCoord.x >= clip.x &&
	   gl_FragCoord.x <= clip.z &&
	   gl_FragCoord.y >= clip.y &&
	   gl_FragCoord.y <= clip.w) {
        color = texture(tex, uv);
		
		if(color.a > 0) {
			float brightness = (0.2126 * color.r + 0.7152 * color.g + 0.0722 * color.b);
			vec3 color_change = vec3(0, 0, 0);
				
			float distance = 0;
			float factor = 0;
			
			for(int i = 0; i < light_count && i < 16; i++) {
				distance = length((position.xy + camera_pos) - lights[i].attributes.xy);
				factor = (1.0 - (distance / lights[i].attributes.z));
				if(factor < 0) {
					factor = 0;
				}
				color_change += (factor * lights[i].color) * lights[i].attributes.w;
			}

			if(color_change.r < default_light) { color_change.r = default_light; }
			if(color_change.g < default_light) { color_change.g = default_light; }
			if(color_change.b < default_light) { color_change.b = default_light; }

			color *= vec4(color_change, 1) * brightness;
			
			color *= tint;
		}
    }
    else {
        discard;
    }
}
