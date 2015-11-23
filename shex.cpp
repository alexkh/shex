#include <iostream>
#include <cmath>
#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <SDL2/SDL_opengl.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 1000

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

void Display_InitGL() {
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

// function to reset our viewport after a window resize
int Display_SetViewport(int width, int height) {
	std::cout << "Set Veiweport\n";
	GLfloat ratio;
	if(height < 1) {
		height = 1;
	}
	ratio = (GLfloat)width / (GLfloat)height;
	// setup our viewport
	glViewport(0, 0, (GLsizei)width, (GLsizei)height);
	// change to the projection matrix and set our viewing volume
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	// set our orthogonal projection:
	glOrtho(0.0, 1.0, 0.0, 1.0, -1.0, 1.0);
	// make sure we are changing the model view and not the projection
	glMatrixMode(GL_MODELVIEW);
	// reset the view
	glLoadIdentity();
	return 1;
}

void Display_Render(SDL_Window *displayWindow) {
	// clear the screen and the depth buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glColor4f(0, 0, 0, 1.0);
	glLoadIdentity();
	glTranslatef(-1.5f, 0.0f, -6.0f);
	glBegin(GL_TRIANGLES);
	glVertex3f(0.0f, 1.0f, 0.0f);
	glVertex3f(-1.0f, -1.0f, 0.0f);
	glVertex3f(1.0f, -1.0f, 0.0f);
	glEnd();

	SDL_GL_SwapWindow(displayWindow);
}

int main(int argc, char *argv[]) {
	if(SDL_Init(SDL_INIT_VIDEO) < 0) {
		std::cerr << "There was an error initializing SDL2: " <<
			SDL_GetError() << std::endl;
		return 1;
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
						SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, OPENGL_MAJOR_VERSION);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, OPENGL_MINOR_VERSION);

	SDL_Window *displayWindow = SDL_CreateWindow("shex",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_OPENGL);

	if(displayWindow == NULL) {
		std::cerr << "There was an error creating the window: " <<
			SDL_GetError() << std::endl;
		return 1;
	}

	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);

	if(context == NULL) {
		std::cerr << "There was an error creating OpenGL context: " <<
			SDL_GetError() << std::endl;
		return 1;
	}

	const unsigned char *version = glGetString(GL_VERSION);
	if(version == NULL) {
		std::cerr << "There was an error with OpenGL configuration." <<
			std::endl;
		return 1;
	}


	glewExperimental = GL_TRUE;
	glewInit();

	SDL_GL_MakeCurrent(displayWindow, context);
	Display_InitGL();
	Display_SetViewport(SCREEN_WIDTH, SCREEN_HEIGHT);
//	Display_Render(displayWindow);
//	SDL_Delay(5000);

	// Create Vertex Array Object:
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
				Display_SetViewport(e.window.data1,
					e.window.data2);
			}
		}
		if(quit) break;
		// Clear the screen to black:
		glClearColor(0.0f, 0.5f, 0.0f, 1.0f);
//		glClear(GL_COLOR_BUFFER_BIT);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		// Draw a triangle from 3 vertices:
		GLint uniColor = glGetUniformLocation(
					shaderProgram, "triangleColor");
		glUniform3f(uniColor, 1.0f, 0.0f, 0.0f);
		// Specify texture:
		GLint tex_id = glGetUniformLocation(shaderProgram, "tex");
		glUniform1i(tex_id, 1);
		// 2nd parameter: number of indices to draw, type, offset
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		SDL_GL_SwapWindow(displayWindow);
		SDL_Delay(1000); // 1fps
	}

	glDeleteTextures(sizeof(tex), tex);

	glDeleteProgram(shaderProgram);
	glDeleteShader(fragmentShader);
	glDeleteShader(vertexShader);

	glDeleteBuffers(1, &ebo);
	glDeleteBuffers(1, &vbo);
	glDeleteVertexArrays(1, &vao);

	SDL_GL_DeleteContext(context);
	SDL_Quit();
	return 0;
}


