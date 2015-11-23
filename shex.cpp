#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <SDL2/SDL_opengl.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define WIN_WIDTH 480
#define WIN_HEIGHT 1040

#define OPENGL_MAJOR_VERSION 2
#define OPENGL_MINOR_VERSION 0

// Shader sources
const GLchar *vertexSource =
	"#version 150 core\n"
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
	Shex() {};
	int init();
	void loop();
private:
	int winw, winh; // window width and height in pixels
	SDL_Window *win;
	SDL_GLContext glcontext;
	void init_gl();
	void set_viewport();
	void draw();
};

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

/*	// Create Vertex Array Object:
	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	// Create a Vertex Buffer Object and copy the vertex data to it:
	GLuint vbo;
	glGenBuffers(1, &vbo);
	float vertices[] = {
		// Position   Color		Texcoords
		-0.5f,  0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, // Top-left
		0.5f,  0.5f,  0.0f, 1.0f, 0.0f, 1.0f, 0.0f, // Top-right
		0.5f, -0.5f,  0.0f, 0.0f, 1.0f, 1.0f, 1.0f, // Bottom-right
		-0.5f, -0.5f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f  // Bottom-left
	};

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices,
		GL_STATIC_DRAW);

	// Create an Element Buffer Object:
	GLuint ebo;
	glGenBuffers(1, &ebo);
	// Element buffer
	GLuint elements[] = {
		0, 1, 2,
		2, 3, 0
	};
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,
		sizeof(elements), elements, GL_STATIC_DRAW);

	// Create a texture:
	GLuint tex[8];
	glGenTextures(sizeof(tex), tex);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex[0]);
	// Black/white checkerboard:
	float pixels[] = {
		0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f
	};
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 2, 2, 0, GL_RGB, GL_FLOAT,
									pixels);
	// Set wrap parameters:
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	// Set borter color to red in case we use GL_CLAMP_TO_BORDER above:
	float color[] = { 1.0f, 0.0f, 0.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, color);
	// Specify filtering method:
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
						GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
						GL_LINEAR_MIPMAP_LINEAR);
	// Create a mipmap:
	glGenerateMipmap(GL_TEXTURE_2D);

	// FONT:
	{
		int x, y, n;
		unsigned char *data = stbi_load("font.png", &x, &y, &n, 1);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, tex[1]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, x, y, 0,
			GL_LUMINANCE, GL_UNSIGNED_BYTE, data);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
						GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
						GL_NEAREST);
	}

	// Create and compile the vertex shader:
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexSource, NULL);
	glCompileShader(vertexShader);
	GLint status;
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &status);
	if(status != GL_TRUE) {
		char buffer[512];
		glGetShaderInfoLog(vertexShader, 512, NULL, buffer);
		std::cout << "Error compiling vertex shader: " << buffer
			<< std::endl;
	}
	// Create and compile the fragment shader:
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
	glCompileShader(fragmentShader);
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &status);
	if(status != GL_TRUE) {
		char buffer[512];
		glGetShaderInfoLog(fragmentShader, 512, NULL, buffer);
		std::cout << "Error compiling fragment shader" << buffer
			<< std::endl;
	}
	// Link the vertex and fragment shader into a shader program
	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glBindFragDataLocation(shaderProgram, 0, "outColor");
	glLinkProgram(shaderProgram);
	glUseProgram(shaderProgram);

	// load the linen pixel shader:
	{
		GLuint linen_shader = glCreateProgram();
		// load the fragment shader:
		std::ifstream t("linen.glsl");
		std::stringstream buffer;
		buffer << t.rdbuf();
		const GLchar *text_str = buffer.str().c_str();
		// compile the fragment shader:
		GLuint linen_fragment = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(linen_fragment, 1, &text_str, NULL);
		glCompileShader(linen_fragment);
		glGetShaderiv(linen_fragment, GL_COMPILE_STATUS, &status);
		if(status != GL_TRUE) {
			char errbuf[512];
			glGetShaderInfoLog(linen_fragment, 512, NULL, errbuf);
			std::cout << "Error compiling linen.glsl " << errbuf <<
				std::endl;
		}
		// link
		glAttachShader(linen_shader, vertexShader);
		glAttachShader(linen_shader, linen_fragment);
		glLinkProgram(linen_shader);
		glUseProgram(linen_shader);
	}


	// Specify the layout of the vertex data:
	GLint posAttrib = glGetAttribLocation(shaderProgram, "position");
	glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE,
			7 * sizeof(float), 0);
	glEnableVertexAttribArray(posAttrib);
	GLint colAttrib = glGetAttribLocation(shaderProgram, "color");
	glVertexAttribPointer(colAttrib, 3, GL_FLOAT, GL_FALSE,
			7 * sizeof(float), (void *)(2 * sizeof(float)));
	glEnableVertexAttribArray(colAttrib);
	GLint texAttrib = glGetAttribLocation(shaderProgram, "texcoord");
	glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE,
			7 * sizeof(float), (void *)(5 * sizeof(float)));
	glEnableVertexAttribArray(texAttrib);

//	Display_Render(displayWindow);
//	SDL_Delay(2000);

*/


/*	glDeleteTextures(sizeof(tex), tex);

	glDeleteProgram(shaderProgram);
	glDeleteShader(fragmentShader);
	glDeleteShader(vertexShader);

	glDeleteBuffers(1, &ebo);
	glDeleteBuffers(1, &vbo);
	glDeleteVertexArrays(1, &vao);
*/
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
		// Clear the screen to black:
		glClearColor(0.0f, 0.5f, 0.0f, 1.0f);
//		glClear(GL_COLOR_BUFFER_BIT);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		// Draw a triangle from 3 vertices:
/*		GLint uniColor = glGetUniformLocation(
					shaderProgram, "triangleColor");
		glUniform3f(uniColor, 1.0f, 0.0f, 0.0f);
		// Specify texture:
		GLint tex_id = glGetUniformLocation(shaderProgram, "tex");
		glUniform1i(tex_id, 1);
		// 2nd parameter: number of indices to draw, type, offset
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
*/

		// draw a square:
		glBegin(GL_QUADS);
			glVertex3f(0, 0, 0);
			glVertex3f(0.5, 0, 0);
			glVertex3f(0.5, 0.5, 0);
			glVertex3f(0, 0.5, 0);
		glEnd();
		SDL_GL_SwapWindow(win);
		SDL_Delay(1000); // 1fps
	}
	SDL_GL_DeleteContext(glcontext);
	SDL_Quit();
}

void Shex::init_gl() {
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
}

int main(int argc, char *argv[]) {
	Shex shex;
	int err = shex.init();
	if(!err) {
		return err;
	}
	shex.loop();
	return 0;
}

