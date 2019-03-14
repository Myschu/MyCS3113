#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#include <SDL.h>
#define GL_GLEXT_PROTOTYPES 1
#include <SDL_opengl.h>
#include <SDL_image.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "ShaderProgram.h"
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include <iostream>
#include <cstdlib>

#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

SDL_Window* displayWindow;

class Paddle {
public:
	Paddle(float posX, float posY) : pos_x(posX), pos_y(posY) {}
	Paddle() {}

	float pos_x;
	float pos_y;


};

class Ball {
public:
	Ball(float posX, float posY,  float spd, float dirX, float dirY) : pos_x(posX), pos_y(posY), speed(spd),  dx(dirX), dy(dirY) {}
	Ball(float posX, float posY, float spd) : pos_x(posX), pos_y(posY), speed(spd) {}
	float pos_x = 0.0f;
	float pos_y = 0.0f;
	float speed = 0.1f;
	float dx = (float)(rand() % 6 + 2);
	float dy = (float)(rand() % 4 - 3);

	void reset() {
		pos_x = 0.0f;
		pos_y = 0.0f;
		speed = 0.1f;
		dx = (float)(rand() % 6 + 2);
		dy = (float)(rand() % 7 - 3);
	}

};

GLuint LoadTexture(const char *filepath) {
	int w, h, comp;
	unsigned char* image = stbi_load(filepath, &w, &h, &comp, STBI_rgb_alpha);

	if (image == NULL) {
		std::cout << "Unable to load image. Make sure the file path is correct\n";
		assert(false);
	}

	GLuint retTexture;
	glGenTextures(1, &retTexture);
	glBindTexture(GL_TEXTURE_2D, retTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	stbi_image_free(image);
	return retTexture;
}

int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("It's Pong Y'all.", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);

#ifdef _WINDOWS
	glewInit();
#endif

	glViewport(0, 0, 640, 360);

	ShaderProgram program;
	program.Load(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");

	glm::mat4 projectionMatrix = glm::mat4(1.0f);
	glm::mat4 viewMatrix = glm::mat4(1.0f);

	projectionMatrix = glm::ortho(-1.777f, 1.777f, -1.0f, 1.0f, -1.0f, 1.0f);

	Paddle leftpaddle = Paddle(-1.65f, 0);
	Paddle rightpaddle = Paddle(1.65f, 0);
	Ball ball = Ball(0, 0, 0.1f);

	GLuint ballTexture = LoadTexture(RESOURCE_FOLDER"white.png");
	GLuint paddleTexture = LoadTexture(RESOURCE_FOLDER"white.png");

	float lastFrameTicks = 0.0f;

	glUseProgram(program.programID);

	SDL_Event event;
	bool done = false;
	bool start = false;
	bool right_win = false;
	while (!done) {

		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
			else if (event.type == SDL_KEYDOWN) {
				if (event.key.keysym.scancode == SDL_SCANCODE_SPACE)
					start = true;
			}
		}

		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		glm::mat4 leftpaddleMatrix = glm::mat4(1.0f);
		glm::mat4 rightpaddleMatrix = glm::mat4(1.0f);
		glm::mat4 ballMatrix = glm::mat4(1.0f);
		leftpaddleMatrix = glm::translate(leftpaddleMatrix, glm::vec3(leftpaddle.pos_x, leftpaddle.pos_y, 0.0f));
		rightpaddleMatrix = glm::translate(rightpaddleMatrix, glm::vec3(rightpaddle.pos_x, rightpaddle.pos_y, 0.0f));
		ballMatrix = glm::translate(ballMatrix, glm::vec3(ball.pos_x, ball.pos_y, 0.0f));

		program.SetModelMatrix(ballMatrix);
		program.SetProjectionMatrix(projectionMatrix);
		program.SetViewMatrix(viewMatrix);

		float ballVert[] = { -0.05f, -0.05f, 0.05f, -0.05f, 0.05f, 0.05f, 0.05f, 0.05f, -0.05f, 0.05f, -0.05f, -0.05f };

		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, ballVert);
		glEnableVertexAttribArray(program.positionAttribute);
		

		float texCoords[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f };

		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
		glEnableVertexAttribArray(program.texCoordAttribute);
		glBindTexture(GL_TEXTURE_2D, ballTexture);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);

		program.SetModelMatrix(leftpaddleMatrix);

		float leftpaddleVert[] = { -0.05f, -0.25f, 0.05f, -0.25f, 0.05f, 0.25f, 0.05f, 0.25f, -0.05f, 0.25f, -0.05f, -0.25f };

		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, leftpaddleVert);
		glEnableVertexAttribArray(program.positionAttribute);

		float texCoords2[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f };

		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords2);
		glEnableVertexAttribArray(program.texCoordAttribute);
		glBindTexture(GL_TEXTURE_2D, paddleTexture);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);

		program.SetModelMatrix(rightpaddleMatrix);
		float rightpaddleVert[] = { -0.05f, -0.25f, 0.05f, -0.25f, 0.05f, 0.25f, 0.05f, 0.25f, -0.05f, 0.25f, -0.05f, -0.25f };

		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, rightpaddleVert);
		glEnableVertexAttribArray(program.positionAttribute);

		float texCoords3[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f };

		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords3);
		glEnableVertexAttribArray(program.texCoordAttribute);
		glBindTexture(GL_TEXTURE_2D, paddleTexture);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);

		float ticks = (float)SDL_GetTicks() / 1000.0f;
		float elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;

		const Uint8 *keys = SDL_GetKeyboardState(NULL);

		if (keys[SDL_SCANCODE_W] && leftpaddle.pos_y < 0.7f) {
			leftpaddle.pos_y += elapsed * 0.4f;
		}
		else if (keys[SDL_SCANCODE_S] && leftpaddle.pos_y > -0.75f) {
			leftpaddle.pos_y += elapsed * -0.4f;
		}

		if (keys[SDL_SCANCODE_UP] && rightpaddle.pos_y < 0.7f) {
			rightpaddle.pos_y += elapsed * 0.4f;
		}
		else if (keys[SDL_SCANCODE_DOWN] && rightpaddle.pos_y > -0.75f) {
			rightpaddle.pos_y += elapsed * -0.4f;
		}

		if (start) {
			float p1 = (abs(leftpaddle.pos_x - ball.pos_x)) - 0.1;
			float p2 = (abs(leftpaddle.pos_y - ball.pos_y)) - 0.3;
			float p3 = (abs(rightpaddle.pos_x - ball.pos_x)) - 0.1;
			float p4 = (abs(rightpaddle.pos_y - ball.pos_y)) - 0.3;
			if ((p1 < 0 && p2 < 0) || (p3 < 0 && p4 < 0)) {
				ball.dx *= -1;
				ball.pos_x += (ball.speed * ball.dx * elapsed);
				ball.pos_y += (ball.speed * ball.dy * elapsed);
			}
			else if (ball.pos_x >= 1.777) {
				start = false;
				ball.reset();
				ball.dx *= -abs(ball.dx);
			}
			else if (ball.pos_x <= -1.777) {
				start = false;
				ball.reset();
				ball.dx = abs(ball.dx);
			}
			else if (ball.pos_y <-0.9 || ball.pos_y > 0.9) {
				ball.dy *= -1;
				ball.pos_x += (ball.speed * ball.dx * elapsed);
				ball.pos_y += (ball.speed * ball.dy * elapsed);
			}
			else {
				ball.pos_x += (ball.speed * ball.dx * elapsed);
				ball.pos_y += (ball.speed * ball.dy * elapsed);
			}
		}

		SDL_GL_SwapWindow(displayWindow);
	}

	SDL_Quit();
	return 0;
}
