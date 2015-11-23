in vec3 Color;
in vec2 Texcoord;
in vec2 bbox; // bounding box
uniform sampler2D tex;
void main() {
	gl_FragColor = texture(tex, Texcoord) * vec4(Color, 1.0);
}