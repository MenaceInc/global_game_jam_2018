#version 330 compatibility

in vec2 uv;
uniform vec4 in_color;
uniform vec4 clip;
uniform vec4 tint;
uniform float thickness;
uniform vec2 rect_size;
out vec4 color;

void main() {
    if(gl_FragCoord.x >= clip.x &&
	   gl_FragCoord.x <= clip.z &&
	   gl_FragCoord.y >= clip.y &&
	   gl_FragCoord.y <= clip.w &&
		((uv.x <= thickness || uv.x >= 1.0 - thickness) ||
		(uv.y <= thickness * (rect_size.x / rect_size.y) || uv.y >= 1.0 - thickness * (rect_size.x / rect_size.y)))) {
        color = in_color;
		color *= tint;
    }
    else {
        discard;
    }
}
