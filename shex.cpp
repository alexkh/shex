#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <SDL2/SDL_opengl.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define WIN_WIDTH 360
#define WIN_HEIGHT 1053

#define OPENGL_MAJOR_VERSION 2
#define OPENGL_MINOR_VERSION 0

// Shader sources
const GLchar *vertexSource =
	"in vec2 position;"
	"in vec3 color;"
	"in vec2 texcoord;"
	"out vec3 Color;"
	"out vec2 Texcoord;"
	"void main() {"
	"	Color = color;"
	"	Texcoord = texcoord;"
	"	gl_Position = vec4(position, 0.0, 1.0);"
	"}";
const GLchar *fragmentSource =
	"#version 150 core\n"
	"in vec3 Color;"
	"in vec2 Texcoord;"
	"out vec4 outColor;"
	"uniform sampler2D tex;"
	"void main() {"
	"	outColor = texture(tex, Texcoord) * vec4(Color, 1.0);"
	"}";

class Shex {
public:
	const char *datafname;
	Shex() {};
	int init();
	void loop();
private:
	int winw, winh; // window width and height in pixels
	SDL_Window *win;
	SDL_GLContext glcontext;
	GLuint tex[8]; // storage for texture ids
	size_t datalen; // length of data actually stored in data texture
	GLuint sp_linen; // line number shader program
	int init_gl(); // OpenGL-specific initializations
	void set_viewport(); // called after window resize
	void draw(); // draw window
	// compile shader given filename and shader type:
	GLuint compile_shader(const char *shfname, GLenum shtype);
	// build shader program given file names for vertex and fragment shader:
	GLuint build_sprogram(GLuint vshader, GLuint fshader);
};

GLuint Shex::compile_shader(const char *shfname, GLenum shtype) {
	// load the shader:
	std::ifstream t(shfname);
	std::stringstream txtbuf;
	txtbuf << t.rdbuf();
	std::string temp_str = txtbuf.str();
	const GLchar *text_str = temp_str.c_str();
	// compile the shader:
	GLuint shader = glCreateShader(shtype);
	glShaderSource(shader, 1, &text_str, NULL);
	glCompileShader(shader);
	GLint status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if(status != GL_TRUE) {
		char errbuf[512];
		glGetShaderInfoLog(shader, 512, NULL, errbuf);
		std::cerr << "Error compiling " << shfname << errbuf <<
			std::endl;
		return 0;
	}
	return shader;
}

GLuint Shex::build_sprogram(GLuint vshader, GLuint fshader) {
	GLuint sprogram = glCreateProgram();
	// link
	glAttachShader(sprogram, vshader);
	glAttachShader(sprogram, fshader);
	glLinkProgram(sprogram);
	//glUseProgram(linen_shader);
	return sprogram;
}

int Shex::init_gl() {
	// enable smooth shading
	glShadeModel(GL_SMOOTH);
	// set the background red
	glClearColor(1.0f, 0.0f, 0.0f, 0.0f);
	// depth buffer setup
	glClearDepth(1.0f);
	// enable depth testing
	glEnable(GL_DEPTH_TEST);
	// the type of depth test to do
	glDepthFunc(GL_LEQUAL);
	// really nice perspective calculations
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

	glGenTextures(8, tex);
	// load font:
	{
		int x, y, n;
		unsigned char *data = stbi_load(
			"/usr/share/shex/font.png", &x, &y, &n, 1);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, tex[1]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, x, y, 0,
			GL_LUMINANCE, GL_UNSIGNED_BYTE, data);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
						GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
						GL_NEAREST);
	}
	// load shaders:
	GLuint vshader = compile_shader(
		"/usr/share/shex/texbox.glsl", GL_VERTEX_SHADER);
	GLuint linen_shader = compile_shader(
		"/usr/share/shex/linen.glsl", GL_FRAGMENT_SHADER);
	sp_linen = build_sprogram(vshader, linen_shader);

	// load data file:
	{
		char buffer[4096]; // 4096 = 64x64 1byte-per-pixel tex
		std::ifstream t(datafname, std::ios::in | std::ios::binary);
		t.read(buffer, 4096);
		datalen = t.gcount();
//		std::cout << "DATA: " << datalen << buffer << std::endl;
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, tex[2]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, 64, 64, 0,
			GL_LUMINANCE, GL_UNSIGNED_BYTE, buffer);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
						GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
						GL_NEAREST);
	}


	return 0;
}

int Shex::init() {
	if(SDL_Init(SDL_INIT_VIDEO) < 0) {
		std::cerr << "There was an error initializing SDL2: " <<
			SDL_GetError() << std::endl;
		return 1;
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
						SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, OPENGL_MAJOR_VERSION);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, OPENGL_MINOR_VERSION);

	winw = WIN_WIDTH;
	winh = WIN_HEIGHT;
	win = SDL_CreateWindow("shex",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		winw, winh, SDL_WINDOW_OPENGL);
	if(win == NULL) {
		std::cerr << "There was an error creating the window: " <<
			SDL_GetError() << std::endl;
		return 1;
	}

	glcontext = SDL_GL_CreateContext(win);
	if(glcontext == NULL) {
		std::cerr << "There was an error creating OpenGL context: " <<
			SDL_GetError() << std::endl;
		return 1;
	}

	glewExperimental = GL_TRUE;
	glewInit();
	SDL_GL_MakeCurrent(win, glcontext);
	init_gl();
	set_viewport();
}

void Shex::loop() {
	SDL_Event e;
	bool quit = false;
	while(1) {
		// process events:
		while(SDL_PollEvent(&e) != 0) {
			if(e.type == SDL_QUIT) {
				quit = true;
			}
			if(e.type == SDL_WINDOWEVENT &&
			(e.window.event ==SDL_WINDOWEVENT_RESIZED ||
			e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)) {
				winw = e.window.data1;
				winh = e.window.data2;
				set_viewport();
			}
		}
		if(quit) break;
		draw();
	}
	SDL_GL_DeleteContext(glcontext);
	SDL_Quit();
}

// reset viewport after a window resize:
void Shex::set_viewport() {
	winh = (winh < 1)? 1: winh;
//	GLfloat ratio = GLfloat(winw) / GLfloat(winh);
	glViewport(0, 0, (GLsizei)winw, (GLsizei)winh);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	// set an orthogonal projection:
	glOrtho(0.0, 1.0, 0.0, 1.0, -1.0, 1.0);
	// make sure we are changing the model view matrix and not projection:
	glMatrixMode(GL_MODELVIEW);
	// reset the view:
	glLoadIdentity();
}

void Shex::draw() {
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	static const GLfloat vertices[][3] = {
		{ 0.0,  0.0,  0.0},
		{ 1.0,  0.0,  0.0},
		{ 1.0,  1.0,  0.0},
		{ 0.0,  1.0,  0.0}
	};
	static const GLfloat texCoords[] = {
		0.0, 1.0,
		1.0, 1.0,
		1.0, 0.0,
		0.0, 0.0
	};

	glLoadIdentity();
	//glTranslatef(0.0, 0.0, -3.0);

	glClearColor(0.0f, 0.5f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

//	glBindTexture(GL_TEXTURE_2D, tex[1]);
	glVertexPointer(3, GL_FLOAT, 0, vertices);
	glTexCoordPointer(2, GL_FLOAT, 0, texCoords);
	// pass font texture to the fragment shader:
	GLint tex_id = glGetUniformLocation(sp_linen, "tex");
	glUniform1i(tex_id, 1);
	// pass data texture to the fragment shader:
	GLint datatex_id = glGetUniformLocation(sp_linen, "datatex");
	glUniform1i(datatex_id, 2);
	GLint datalen_id = glGetUniformLocation(sp_linen, "datalen");
	glUniform1i(datalen_id, datalen);
	// pass window size to the fragment shader:
	GLint bbox_param = glGetUniformLocation(sp_linen, "bbox");
	glUniform2f(bbox_param, winw, winh);
	glUseProgram(sp_linen);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	SDL_GL_SwapWindow(win);
	return;
}

int main(int argc, char *argv[]) {
	if(argc < 2) {
		std::cerr << "Hex viewer. Usage: " << std::endl <<
			argv[0] << " filename" << std::endl;
		return 1;
	}
	Shex shex;
	shex.datafname = argv[1];
	int err = shex.init();
	if(!err) {
		return err;
	}
	shex.loop();
	return 0;
}

