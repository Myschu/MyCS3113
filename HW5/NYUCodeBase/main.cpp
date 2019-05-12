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
#include <vector>
#include <string>
#include <iostream>
#include <cstdlib>
#include <fstream>
#include <sstream>

#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

SDL_Window* displayWindow;

float elapsed;
bool running = false;

ShaderProgram program;
GLuint fontTexture;
GLuint spriteTexture;


class SheetSprite {
public:
	SheetSprite() {}
	SheetSprite(unsigned int textureID, float u, float v, float width, float height, float size) : textureID(textureID), u(u), v(v), width(width), height(height), size(size) {}

	void Draw(ShaderProgram &program);

	float size;
	unsigned int textureID;
	float u;
	float v;
	float width;
	float height;
};

void SheetSprite::Draw(ShaderProgram &program) {
	glBindTexture(GL_TEXTURE_2D, textureID);

	GLfloat texCoords[] = {
		u, v + height,
		u + width, v,
		u, v,
		u + width, v,
		u, v + height,
		u + width, v + height
	};
	float aspect = width / height;
	float vertices[] = {
		-0.5f * size * aspect, -0.5f * size,
		0.5f * size * aspect, 0.5f * size,
		-0.5f * size * aspect, 0.5f * size,
		0.5f * size * aspect, 0.5f * size,
		-0.5f * size * aspect, -0.5f * size,
		0.5f * size * aspect, -0.5f * size };

	glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
	glEnableVertexAttribArray(program.positionAttribute);


	glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
	glEnableVertexAttribArray(program.texCoordAttribute);

	glDrawArrays(GL_TRIANGLES, 0, 6);
	glDisableVertexAttribArray(program.positionAttribute);
	glDisableVertexAttribArray(program.texCoordAttribute);

}

class Entity {
public:
	Entity() {}
	Entity(float x, float y, float dx, float dy) {
		position.x = x;
		position.y = y;
		d_x = dx;
		d_y = dy;

	}

	GLuint textureID;
	SheetSprite sprite;
	std::string type;
	float timeAlive;
	float d_x;
	float d_y;
	glm::vec3 position;
	glm::vec3 size;

	void Draw(ShaderProgram& program) {
		glm::mat4 modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, glm::vec3(position.x, position.y, 0.0f));
		program.SetModelMatrix(modelMatrix);
		sprite.Draw(program);
	}
	void remove() {
		position.x = 2000.0f;
		position.y = 3000.0f;
	}
};

void DrawText(ShaderProgram program, int fontTexture, std::string text, float size, float spacing) {
	float character_size = 1.0 / 16.0;
	std::vector<float> vertexData;
	std::vector<float> texCoordData;

	for (int i = 0; i < text.size(); i++) {
		int spriteIndex = (int)text[i];
		float texture_x = (float)(spriteIndex % 16) / 16.0f;
		float texture_y = (float)(spriteIndex / 16) / 16.0f;
		vertexData.insert(vertexData.end(), {
			((size + spacing) * i) + (-0.5f * size), 0.5f * size,
			((size + spacing) * i) + (-0.5f * size), -0.5f * size,
			((size + spacing) * i) + (0.5f * size), 0.5f * size,
			((size + spacing) * i) + (0.5f * size), -0.5f * size,
			((size + spacing) * i) + (0.5f * size), 0.5f * size,
			((size + spacing) * i) + (-0.5f * size), -0.5f * size,
		});
		texCoordData.insert(texCoordData.end(), {
			texture_x, texture_y,
			texture_x, texture_y + character_size,
			texture_x + character_size, texture_y,
			texture_x + character_size, texture_y + character_size,
			texture_x + character_size, texture_y,
			texture_x, texture_y + character_size,
		});

	}
	glBindTexture(GL_TEXTURE_2D, fontTexture);
	glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
	glEnableVertexAttribArray(program.positionAttribute);


	glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
	glEnableVertexAttribArray(program.texCoordAttribute);

	glDrawArrays(GL_TRIANGLES, 0, text.length() * 6);
	glDisableVertexAttribArray(program.positionAttribute);
	glDisableVertexAttribArray(program.texCoordAttribute);
}

struct GameState {
	Entity player;
	std::vector<Entity> enemies;
	std::vector<Entity> bullets;
	int score;
};

enum GameMode { STATE_MAIN_MENU, STATE_GAME_LEVEL };
GameMode mode = STATE_MAIN_MENU;
GameState state;

void RenderMainMenu() {

	glm::mat4 textMatrix = glm::mat4(1.0f);

	textMatrix = glm::translate(textMatrix, glm::vec3(-1.2f, 0.2f, 0.0f));
	program.SetModelMatrix(textMatrix);
	DrawText(program, fontTexture, "Invaders in Space!", 0.25f, -0.12f);

	textMatrix = glm::mat4(1.0f);
	textMatrix = glm::translate(textMatrix, glm::vec3(-1.4f, -0.2f, 0.0f));
	program.SetModelMatrix(textMatrix);
	DrawText(program, fontTexture, "Press SPACE to start!", 0.2f, -0.06f);

	textMatrix = glm::mat4(1.0f);
	textMatrix = glm::translate(textMatrix, glm::vec3(-1.4f, -0.4f, 0.0f));
	program.SetModelMatrix(textMatrix);
	DrawText(program, fontTexture, "Left and Right arrow keys to move.", 0.1f, -0.02f);

	textMatrix = glm::mat4(1.0f);
	textMatrix = glm::translate(textMatrix, glm::vec3(-0.7f, -0.6f, 0.0f));
	program.SetModelMatrix(textMatrix);
	DrawText(program, fontTexture, "Spacebar to shoot.", 0.1f, -0.02f);

}

Entity player;
std::vector<Entity> enemies;
SheetSprite mySprite3;
std::vector<Entity> bullets;

void UpdateMainMenu(float elapsed) {}

void RenderGameLevel() {
	state.player.Draw(program);
	for (size_t i = 0; i < state.enemies.size(); i++) {
		state.enemies[i].Draw(program);
	}
	for (size_t i = 0; i < state.bullets.size(); i++) {
		state.bullets[i].Draw(program);
	}
}

void UpdateGameLevel(float elapsed) {
	const Uint8 *keys = SDL_GetKeyboardState(NULL);
	if (keys[SDL_SCANCODE_LEFT] && state.player.position.x - state.player.size.x / 2.0f > -1.777f) {
		state.player.position.x -= state.player.d_x*elapsed;
	}
	else if (keys[SDL_SCANCODE_RIGHT] && state.player.position.x + state.player.size.x/2.0f < 1.777f) {
		state.player.position.x += state.player.d_x * elapsed;
	}

	for (size_t i = 0; i < state.enemies.size(); i++) {
		state.enemies[i].position.x += state.enemies[i].d_x * elapsed;

		if ((state.enemies[i].position.x + state.enemies[i].size.x/2 > 1.7f) || (state.enemies[i].position.x - state.enemies[i].size.x / 2 < -1.7f )) {
			for (size_t i = 0; i < state.enemies.size(); i++) {
				state.enemies[i].d_x = -state.enemies[i].d_x;
			}
		}

	}
	for (size_t i = 0; i < state.bullets.size(); i++) {
		state.bullets[i].position.y += state.bullets[i].d_y * elapsed;

		for (size_t j = 0; j < state.enemies.size(); j++) {
			float p1 = (abs(state.enemies[j].position.x - state.bullets[i].position.x) - ((state.enemies[j].size.x) + (state.bullets[i].size.x)) / 2.0f);
			float p2 = (abs(state.enemies[j].position.y - state.bullets[i].position.y) - ((state.enemies[j].size.y) + (state.bullets[i].size.y)) / 2.0f);
			if (p1 < 0 && p2 < 0) {
				state.bullets[i].remove();
				state.enemies[j].remove();
			}
		}
	}
	
}


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

void Render() {
	glClear(GL_COLOR_BUFFER_BIT);
	switch (mode) {
	case STATE_MAIN_MENU:
		RenderMainMenu();
		break;
	case STATE_GAME_LEVEL:
		RenderGameLevel();
		break;
	}
}

void Update(float elapsed) {
	switch (mode) {
	case STATE_MAIN_MENU:
		UpdateMainMenu(elapsed);
		break;
	case STATE_GAME_LEVEL:
		UpdateGameLevel(elapsed);
		break;
	}
}

void shootBullet() {
	Entity newBullet;
	newBullet.position.x = state.player.position.x;
	newBullet.position.y = state.player.position.y;
	newBullet.sprite = mySprite3;
	newBullet.d_y = 2.0f;
	newBullet.timeAlive = 0.0f;
	newBullet.size = glm::vec3(mySprite3.size * mySprite3.width / mySprite3.height, mySprite3.size, 0.0f);
	state.bullets.push_back(newBullet);
}



int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("Wow. In Space.", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);

#ifdef _WINDOWS
	glewInit();
#endif

	glViewport(0, 0, 640, 360);

	program.Load(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");

	glm::mat4 projectionMatrix = glm::mat4(1.0f);
	glm::mat4 viewMatrix = glm::mat4(1.0f);

	projectionMatrix = glm::ortho(-1.777f, 1.777f, -1.0f, 1.0f, -1.0f, 1.0f);

	program.SetViewMatrix(viewMatrix);
	program.SetProjectionMatrix(projectionMatrix);

	fontTexture = LoadTexture(RESOURCE_FOLDER"font1.png");
	GLuint spriteTexture = LoadTexture(RESOURCE_FOLDER"sheet.png");

	SheetSprite mySprite = SheetSprite(spriteTexture, 425.0f / 1024.0f, 468.0f / 1024.0f, 93.0f / 1024.0f, 84.0f / 1024.0f, 0.2f);
	player.sprite = mySprite;
	player.d_x = 2.0f;
	player.position = glm::vec3(0.0f, -0.8f, 0.0f);
	player.size = glm::vec3(mySprite.size * (mySprite.width/mySprite.height), mySprite.size, 0.0f);
	state.player = player;
	float lastFrameTicks = 0.0f;

	SheetSprite mySprite2 = SheetSprite(spriteTexture, 518.0f / 1024.0f, 325.0f / 1024.0f, 82.0f / 1024.0f, 82.0f / 1024.0f, 0.2f);
	for (int i = 0; i < 12; i++) {
		Entity enemy;
		enemy.type = "enemy";
		enemy.position = glm::vec3(-1.6f + (i % 6) * 0.25f, 0.0f + (i / 6) * 0.5f, 0.0f);
		enemy.d_x = 1.0f;
		enemy.sprite = mySprite2;
		enemy.size = glm::vec3(mySprite2.size * mySprite2.width / mySprite2.height, mySprite2.size, 0.0f);
		enemies.push_back(enemy);

	}
	state.enemies = enemies;


	mySprite3 = SheetSprite(spriteTexture, 856.0f / 1024.0f, 775.0f / 1024.0f, 9.0f / 1024.0f, 37.0f / 1024.0f, 0.2f);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glUseProgram(program.programID);

	SDL_Event event;
	bool done = false;

	while (!done) {

		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
			if (event.type == SDL_KEYDOWN) {
				if (event.key.keysym.scancode == SDL_SCANCODE_SPACE)
					if (mode == STATE_MAIN_MENU) {
						mode = STATE_GAME_LEVEL;
					}
					else if (mode == STATE_GAME_LEVEL) {
						shootBullet();
					}
			}
		}

		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

		float ticks = (float)SDL_GetTicks() / 1000.0f;
		elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;
		Update(elapsed);
		Render();

		SDL_GL_SwapWindow(displayWindow);
	}
	
	SDL_Quit();
	return 0;
}
