varying vec2 texcoord;
uniform vec2 bbox; // bounding box
uniform sampler2D tex; // font texture
uniform sampler2D datatex; // data values texture
uniform int datalen; // length of data inside data texture

// output a hexadecimal digit's pixel dxo, dyo, colored by color and
// blend factor blend. if blend is -1.0, then texture's red at that point
// will determine its blend factor (transparent background)
void print_digit(int digit, int dxo, int dyo, vec4 color, float blend) {
	if(digit > 9) {
		digit -= 9;
		dyo += 39;
	}
	vec4 tcol = texture(tex, vec2((float(digit) * 6.0 + float(dxo)) / 128.0,
			(39.0 + float(dyo)) / 128.0));
	blend = blend == -1.0? tcol.r: blend;
	gl_FragColor = mix(gl_FragColor, tcol * color, blend);
}

void print_ascii(int val, int dxo, int dyo, vec4 color, float blend) {
//	if(val > -1 && val < 128) {
//	}
	vec4 tcol = texture(tex, vec2((float(val % 16) * 6.0 + float(dxo))
		/ 128.0, (13.0 * float(val / 16) + float(dyo)) / 128.0));
	blend = blend == -1.0? tcol.r: blend;
	gl_FragColor = mix(gl_FragColor, tcol * color, blend);
}


// mainbox with hex values of data:
// bxo = x inside the box, dyo = y inside the line, dataoffset = data offset
void mainbox(int bxo, int dyo, int dataoffset) {
	vec4 texval = texture(datatex,
			vec2(mod(dataoffset, 64.0) / 64.0,
			floor(dataoffset / 64.0) / 64.0));
	float hexval = texval.r * 256.0;
	int digit = int((mod(floor(bxo / 6.0), 2.0) == 0.0)?
		floor(hexval / 16.0): mod(hexval, 16.0));
	vec4 col = vec4(0.8, 0.8, 0.0, 1.0);
	if(hexval == 0) {
		col = vec4(0.0, 0.0, 0.9, 1.0);
	} else if(hexval < 32) {
		col = vec4(0.0, 0.6, 0.6, 1.0);
	} else if(hexval < 128) {
		col = vec4(0.8, 0.8, 0.8, 1.0);
	} else if(hexval < 192) {
		col = vec4(1.0, 0.5, 0.0, 1.0);
	}
	print_digit(digit, int(mod(bxo, 6)), dyo, col, 1.0);
}

void asciibox(int bxo, int dyo, int dataoffset) {
	vec4 texval = texture(datatex,
			vec2(mod(dataoffset, 64.0) / 64.0,
			floor(dataoffset / 64.0) / 64.0));
	int val = int(texval.r * 256.0);
	vec4 col = vec4(0.8, 0.8, 0.8, 1.0);
	if(val < 32) {
		col = vec4(0.0, 0.6, 0.6, 1.0);
	}
	if(val == 0) {
		col = vec4(0.0, 0.0, 0.9, 1.0);
	}
	if(val > 127 && val < 192) {
		val = 127;
		col = vec4(1.0, 0.5, 0.0, 1.0);
	}
	if(val > 191) {
		val = 127;
		col = vec4(0.8, 0.8, 0.0, 1.0);
	}
	if(val > -1 && val < 256) {
		print_ascii(val, int(mod(bxo, 6)), dyo, col, 1.0);
	}
}

void main() {
	vec2 co = floor(texcoord * bbox); // coordinates within the window
	co.y -= 13.0;
	float lineh = 13; //bbox.y * 0.01234567; // line height in pixels
	// line number:
	int linen = int(co.y / lineh);
	// pixel from top of the line (co.y modulo lineh):
	int py = int(co.y - lineh * float(linen));

//	gl_FragColor = texture(datatex, texcoord); // * vec4(Color, 1.0);

//	if(linen < 0) {
//		gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
//	} else

	// offset values on the left:
	if(co.y >= 0 && co.x < 24.0) {
		float base = pow(16.0, (3.0 - floor(co.x / 6)));
		int digit = int(mod(floor((linen * 16) / base), 16));
		int dxo = int(mod(co.x, 6.0));//offset within digit:dig x offset
		print_digit(digit, dxo, py, vec4(0.0, 0.8, 0.0, 1.0), -1.0);
	} else

	// hex data values in the main box:
	if(co.y >= 0 && co.x > 27.0 && co.x < 244.0) {
		// box x offset == x coord. within box
		int rbxo = int(co.x) - 28; // real box x offset
		// box x offset without gaps:
		int bxo0 = rbxo - (rbxo / 108);
		int bxoz = bxo0 - (bxo0 / 109);
		int bxo1 = bxoz - (bxoz / 27);
		int bxo2 = bxo1 - (bxo1 / 13);
		// next bxo:
		int nbxo1 = (rbxo + 1) - ((rbxo) / 13);
		int nbxo2 = nbxo1 - ((nbxo1 - 1) / 26);
		int dataoffset = linen * 16 + bxo2 / 12;
		if(dataoffset < datalen) {
			mainbox(bxo2, py, dataoffset);
		}
//		if(co.x == 28 || co.x == 40) {
//			gl_FragColor = vec4(0.0, 1.0, 0.0, 1.0);
//		}
	}

	// ascii data values on the right side
	if(co.y >= 0 && co.x > 248.0 && co.x < 346) {
		int bxo2 = int(co.x) - 249;
		int dataoffset = linen * 16 + bxo2 / 6;
		if(dataoffset < datalen) {
			asciibox(bxo2, py, dataoffset);
		}
	}

	// frame around the window:
	if(co.x < 1.0 || co.x > (bbox.x - 2.0) ||
		co.y < -12.0 || co.y > (bbox.y - 15.0)) {
		gl_FragColor = vec4(0.2, 0.0, 0.0, 1.0);
	}



//	gl_FragColor = vec4(0.0, 0.1 * linen_mod10, 0.0, 1.0);

	if(co.y >= 0 && mod(linen, 2) == 0) {
		gl_FragColor = mix(vec4(0.5, 0.6, 0.5, 1.0), gl_FragColor,
			0.8);
	}

	if(co.y >= 0 && (co.x == 249 || co.x == 297.0 || co.x == 346)) {
		gl_FragColor = vec4(0.2, 0.0, 0.2, 1.0);
	}


}
