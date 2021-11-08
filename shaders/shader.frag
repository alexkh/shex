#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject {
	mat4 model;
	mat4 view;
	mat4 proj;
	uvec4 fontdata[256];
	uvec4 chunkmeta;
	uvec4 datachunk[128];
	vec4 param; // parameters: scalex, scaley
} ubo;

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

const int charw = 6; // character width
const int charh = 13; // character height
const int lineh = 14; // line height
const int scrollerminsize = 40;
const ivec4 koibbox = ivec4(263, 15, 359, 1063);
const ivec4 hexbbox = ivec4(44, 15, 254, 1063);
const ivec4 addrbbox = ivec4(14, 15, 37, 1063);
const ivec4 scrollbbox = ivec4(3, 15, 10, 1063);
const ivec4 topbbox = ivec4(2, 2, 359, 14);

void print_koi(uint letter, uint col, uint row, vec4 color) {
	uint word = ubo.fontdata[letter][row / 4];
	uint bytebit = 1 << (7 - col);
	uint wordbit = bytebit << (8 * (row % 4));
	uint val = word & wordbit;

	if(val != 0) {
		outColor = color;
	}
}

vec4 lettercolor(uint letter) {
	vec4 color = vec4(0.8, 0.8, 0.0, 1.0);
	if(letter == 0) {
		color = vec4(0.0, 0.0, 0.9, 1.0);
	} else if(letter < 32) {
		color = vec4(0.0, 0.6, 0.6, 1.0);
	} else if(letter < 128) {
		color = vec4(0.8, 0.8, 0.8, 1.0);
	} else if(letter < 192) {
		color = vec4(1.0, 0.5, 0.0, 1.0);
	}
	return color;
}

void main() {
	const ivec2 coord = ivec2(uint(gl_FragCoord.x / ubo.param[0]),
			uint(gl_FragCoord.y / ubo.param[1]));

	int linen = (coord.y - addrbbox.y) / lineh;
	uint addr = ubo.chunkmeta[1] + ubo.chunkmeta[2] + linen * 16;

	if(coord.y >= addrbbox.y && coord.y <= addrbbox.w &&
			(addr / 16) % 2 == 0) {
		outColor = vec4(0.0, 0.012, 0.0, 1.0);
	} else {
		outColor = texture(texSampler, vec2(0.0, 0.0));
	}

	if(coord.x == 0 || coord.x == 363 || coord.y == 0 || coord.y == 1064) {
		outColor.r = 0.2;
	}

	// scrollbar
	if(coord.x >= scrollbbox.x && coord.x <= scrollbbox.z
			&& coord.y >= scrollbbox.y && coord.y <= scrollbbox.w) {
		int sbsize = int((1200.0 / float(ubo.chunkmeta[0])) *
				(scrollbbox.w - scrollbbox.y));
		sbsize = sbsize < scrollerminsize? scrollerminsize: sbsize;
		int sbpos = int((float(ubo.chunkmeta[1] + ubo.chunkmeta[2]) /
				float(ubo.chunkmeta[0])) *
				(1048.0 - float(sbsize))) + 15;
		if(coord.y > sbpos && coord.y < sbpos + sbsize) {
			float gradient = float(coord.y - sbpos) / float(sbsize);
			outColor = vec4(gradient * 0.5,
				0.5 - gradient * 0.5, 0.0, 1.0);
		} else {
			outColor = vec4(0.0, 0.0, 0.0, 1.0);
		}
	}

	if(addr > ubo.chunkmeta[0]) {
		return;
	}

	// addrbox
	if(coord.x >= addrbbox.x && coord.y >= addrbbox.y &&
			coord.x <= addrbbox.z && coord.y <= addrbbox.w) {
		int x = coord.x - addrbbox.x;
		int y = coord.y - addrbbox.y;
		int column = x / 6;

		uint letter = addr >> 4 * (4 - column - 1);
		letter &= 0xf;
		if(letter < 10) {
			letter += 48;
		} else {
			letter += 87;
		}

		vec4 color = vec4(0.6, 0.6, 0.3, 1.0);
		if(addr % 256 == 0) {
			color = vec4(0, 0.5, 0.0, 1.0);
		}

		print_koi(letter, x % 6, y % lineh, color);

		return;
	}

	// koibox
	if(coord.x >= koibbox.x && coord.y >= topbbox.y &&
			coord.x <= koibbox.z && coord.y <= koibbox.w) {
		int x = coord.x - koibbox.x;
		int y = coord.y - koibbox.y;

		int xo = x - (x / (charw * 8 + 1)) * 2;

		if(addr + (xo / 6) >= ubo.chunkmeta[0]) {
			return;
		}

		uint word = ubo.datachunk[y / lineh + ubo.chunkmeta[2] / 16]
				[xo / (4 * charw)];
		uint letter = word >> (8 * (xo / 6));
		letter &= 0xff;

		vec4 color = lettercolor(letter);

		// draw the top address offsets
		if(y < 0) {
			int addroffset = xo / 6;
			if(addroffset < 10) {
				letter = 48 + addroffset;
			} else {
				letter = 87 + addroffset;
			}

			y += 13;
			color = vec4(0.0, 0.4, 0.0, 1.0);
		}

		if(x != charw * 8 && x != charw * 8 + 1) {
			print_koi(letter, xo % 6, y % lineh, color);
		}
		return;
	}

	// hexbox
	if(coord.x >= hexbbox.x && coord.y >= topbbox.y &&
			coord.x <= hexbbox.z && coord.y <= hexbbox.w) {
		int x = coord.x - hexbbox.x;
		int x1 = x + 1;
		int y = coord.y - hexbbox.y;
		if(y < 0) {
			x += 3;
		}

		// x offset
		int o1 = (charw * 2 + 1); // 1 byte offset
		int o4 = (o1 * 4 + 1); // 4 byte offset
		int o8 = (o4 * 2 + 1); // 8 byte offset

		int xo = x - x / o8;
		xo -= xo / o4;
		xo -= xo / o1;

		int x1o = x1 - x1 / o8;
		x1o -= x1o / o4;
		x1o -= x1o / o1;

		if(xo == x1o) {
//			outColor.g = 1.0;
			return;
		}

		if(addr + (xo / 12) >= ubo.chunkmeta[0]) {
			return;
		}

		uint word = ubo.datachunk[y / lineh + ubo.chunkmeta[2] / 16]
				[xo / (8 * charw)];
		uint letter = word >> (8 * (xo / int(charw * 2)));
		letter &= 0xff;
		vec4 color = lettercolor(letter);

		if(xo % int(charw * 2) < 6) {
			letter = letter >> 4;
		}
		letter &= 0xf;
		if(letter < 10) {
			letter += 48;
		} else {
			letter += 87;
		}

		// draw the top address offsets
		if(y < 0) {
			if((xo / 6) % 2 == 0) {
				return;
			}
			int addroffset = xo / 12;
			if(addroffset < 10) {
				letter = 48 + addroffset;
			} else {
				letter = 87 + addroffset;
			}

			y += 13;
			color = vec4(0.0, 0.4, 0.0, 1.0);
		}

		print_koi(letter, xo % 6, y % lineh, color);
		return;
	}

	// topbox
	if(coord.x >= topbbox.x && coord.x <= topbbox.z
			&& coord.y >= topbbox.y && coord.y <= topbbox.w) {
		uint chunkoffset = ubo.chunkmeta[1] + ubo.chunkmeta[2];
		int x = coord.x - topbbox.x;
		int y = coord.y - topbbox.y;
		int column = x / 6;

		if(column < 3) {
			chunkoffset = 0; // todo: support 40-bit addresses
		}
		uint letter = chunkoffset >> (36 - column * 4);
		letter &= 0xf;
		if(letter < 10) {
			letter += 48;
		} else {
			letter += 87;
		}

		vec4 color = vec4(0.6, 0.6, 0.3, 1.0);

		if(column < 6) {
			print_koi(letter, x % 6, y % lineh, color);
		}

		return;
	}
}
