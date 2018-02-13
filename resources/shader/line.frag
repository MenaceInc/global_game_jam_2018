#version 330 compatibility

uniform vec4 in_color;
uniform vec4 clip;
uniform vec4 tint;

out vec4 color;

void main() {
	if(gl_FragCoord.x >= clip.x && 
	   gl_FragCoord.x <= clip.z &&
	   gl_FragCoord.y >= clip.y && 
	   gl_FragCoord.y <= clip.w) {
		color = in_color;
		color *= tint;
	}
	else {
		discard;
	}
}
