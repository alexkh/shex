varying vec2 texcoord;
in vec2 bbox; // bounding box
uniform sampler2D tex;
void main() {
	gl_FragColor = texture(tex, texcoord); // * vec4(Color, 1.0);
//	gl_FragColor = vec4(1.0, 0.5, texcoord, 1.0);
}
