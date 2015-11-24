varying vec2 texcoord;
uniform vec2 bbox; // bounding box
uniform sampler2D tex;
void main() {
	vec2 co = texcoord * bbox; // coordinates within the window
	float lineh = bbox.y * 0.0125; // line height in pixels
	// line number:
	float linen = floor(co.y / lineh);
	// pixel from top of the line (co.y modulo lineh):
	float py = co.y - lineh * linen;
	// linen_mod10
	float linen_mod10 = linen - 10 * floor(linen * 0.1);

	gl_FragColor = texture(tex, texcoord); // * vec4(Color, 1.0);

	if(co.x < 6.0) {
		gl_FragColor = texture(tex,
			vec2( (linen_mod10 * 6.0 + co.x) / 128.0, (13.0 + py) /128.0));
	}

	// frame around the window:
	if(co.x < 1.0 || co.x > (bbox.x - 1) ||
		co.y < 1.0 || co.y > (bbox.y - 1)) {
		gl_FragColor = vec4(0.1, 0.1, 0.1, 1.0);
	}



//	gl_FragColor = vec4(0.0, 0.1 * linen_mod10, 0.0, 1.0);

//	if(linen == 5.0) {
//		gl_FragColor = vec4(1.0, 0.7, 0.0, 1.0);
//	}


}
