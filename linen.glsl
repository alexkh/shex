varying vec2 texcoord;
uniform vec2 bbox; // bounding box
uniform sampler2D tex; // font texture
uniform sampler2D data; // data values texture

// output a hexadecimal digit's pixel dxo, dyo, colored by color and
// blend factor blend. if blend is -1.0, then texture's red at that point
// will determine its blend factor (transparent background)
void print_digit(float digit, float dxo, float dyo, vec4 color, float blend) {
	if(digit > 9) {
		digit -= 9;
		dyo += 39.0;
	}
	vec4 tcol = texture(tex, vec2((digit * 6.0 + dxo) / 128.0, (13.0 + dyo)
		/ 128.0));
	blend = blend == -1.0? tcol.r: blend;
	gl_FragColor = mix(gl_FragColor, tcol * color, blend);
}

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

	// offset values on the left:
	if(co.x < 24.0) {
		float base = pow(16.0, (3.0 - floor(co.x / 6)));
		float digit = mod(floor(linen / base), 16);
		float dxo = mod(co.x, 6); // offset within digit: dig x offset
		print_digit(digit, dxo, py, vec4(0.3, 0.3, 0.3, 1.0), -1.0);
	}

	// frame around the window:
	if(co.x < 1.0 || co.x > (bbox.x - 1) ||
		co.y < 1.0 || co.y > (bbox.y - 1)) {
		gl_FragColor = vec4(0.1, 0.1, 0.1, 1.0);
	}



//	gl_FragColor = vec4(0.0, 0.1 * linen_mod10, 0.0, 1.0);

	if(mod(linen, 2) == 0) {
		gl_FragColor = mix(vec4(0.1, 0.2, 0.1, 1.0), gl_FragColor,
			0.8);
	}


}
