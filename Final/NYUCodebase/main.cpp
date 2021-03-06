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
#include <SDL_mixer.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "ShaderProgram.h"
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "FlareMap.h"
#include <vector>
#include <string>
#include <iostream>
#include <cstdlib>
#include <fstream>
#include <sstream>

using namespace std;

#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

SDL_Window* displayWindow;

#define FIXED_TIMESTEP 0.0166666f
#define MAX_TIMESTEPS 6

float lastFrameTicks = 0.0f;
float elapsed;
FlareMap map;
ShaderProgram program;
float TILE_SIZE = 0.1f;
int SPRITE_COUNT_X = 16;
int SPRITE_COUNT_Y = 8;
float friction_x = 0.5f;
float friction_y = 0.5f;
GLuint spriteTexture;
void DrawSpriteSheetSprite(ShaderProgram &program, int index, int spriteCountX, int spriteCountY, float size);
Mix_Chunk *DefaultGunSound;
Mix_Chunk *HeavyGunSound;
Mix_Chunk *WaveGunSound;
Mix_Chunk *SparkGunSound;
GLuint fontTexture;
class Entity;

float lerp(float v0, float v1, float t) {
	return (1.0 - t)*v0 + t * v1;
}

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

class Particle {
public:
	glm::vec3 position;
	glm::vec3 velocity;
	float lifetime;
};

class ParticleEmitter {
public:
	ParticleEmitter() {}
	ParticleEmitter(unsigned int particleCount) {}
	~ParticleEmitter() {}


	void Update(float elapsed);
	void Render();
	glm::vec3 position;
	glm::vec3 gravity;
	float maxLifetime;

	std::vector<Particle> particles;
};


void Draw(ShaderProgram& program) {
	vector<float> vertexData;
	vector<float> texCoordData;
	for (int y = 0; y < map.mapHeight; y++) {
		for (int x = 0; x < map.mapWidth; x++) {
			if (map.mapData[y][x] != 0) {
				float u = (float)(((int)map.mapData[y][x]) % SPRITE_COUNT_X) / (float)SPRITE_COUNT_X;
				float v = (float)(((int)map.mapData[y][x]) / SPRITE_COUNT_X) / (float)SPRITE_COUNT_Y;

				float spriteWidth = 1.0f / (float)SPRITE_COUNT_X;
				float spriteHeight = 1.0f / (float)SPRITE_COUNT_Y;

				vertexData.insert(vertexData.end(), {

				TILE_SIZE * x, -TILE_SIZE * y,
			   TILE_SIZE * x, (-TILE_SIZE * y) - TILE_SIZE,
				(TILE_SIZE * x) + TILE_SIZE, (-TILE_SIZE * y) - TILE_SIZE,

				TILE_SIZE * x, -TILE_SIZE * y,
				(TILE_SIZE * x) + TILE_SIZE, (-TILE_SIZE * y) - TILE_SIZE,
				(TILE_SIZE * x) + TILE_SIZE, -TILE_SIZE * y
					});

				texCoordData.insert(texCoordData.end(), {
				u, v,
				u, v + (spriteHeight),
				u + spriteWidth, v + (spriteHeight),

				u, v,
				u + spriteWidth, v + (spriteHeight),
				u + spriteWidth, v
					});
			}

		}

	}
	glBindTexture(GL_TEXTURE_2D, spriteTexture);
	glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
	glEnableVertexAttribArray(program.positionAttribute);


	glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
	glEnableVertexAttribArray(program.texCoordAttribute);

	glDrawArrays(GL_TRIANGLES, 0, int(vertexData.size() / 2));
	glDisableVertexAttribArray(program.positionAttribute);
	glDisableVertexAttribArray(program.texCoordAttribute);
}

pair<int, int> worldToTileCoordinates(float worldX, float worldY) {
	return { (int)(worldX / TILE_SIZE), (int)(worldY / -TILE_SIZE) };
}

vector<Entity> bullets1;
vector<Entity> bullets2;
vector<Entity> items;

class Entity {
public:
	Entity() {}
	Entity(float x, float y, int ind) : index(ind) {
		position.x = x;
		position.y = y;
		acceleration = glm::vec3(0.0f, 0.0f, 0.0f);
		velocity = glm::vec3(0.0f, 0.0f, 0.0f);
		size = glm::vec3(TILE_SIZE, TILE_SIZE, 0.0f);
	}

	SheetSprite sprite;
	GLuint textureID;
	int index;
	string type;
	float timeAlive;
	string bullet_type = "default";
	string facing = "left";
	int health = 100;
	float sincelastshot = 0;

	glm::vec3 position;
	glm::vec3 velocity = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 acceleration;
	glm::vec3 size = glm::vec3(TILE_SIZE, TILE_SIZE, 0.0f);

	void Draw(ShaderProgram& program) {
		glm::mat4 modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, glm::vec3(position.x, position.y, 0.0f));

		program.SetModelMatrix(modelMatrix);
		DrawSpriteSheetSprite(program, index, SPRITE_COUNT_X, SPRITE_COUNT_Y, TILE_SIZE);
	}
	void Draw1(ShaderProgram& program) {
		glm::mat4 modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, glm::vec3(position.x+TILE_SIZE/2, position.y + TILE_SIZE / 2, 0.0f));

		program.SetModelMatrix(modelMatrix);
		DrawSpriteSheetSprite(program, index, SPRITE_COUNT_X, SPRITE_COUNT_Y, TILE_SIZE);
	}

	void remove() {
		position.x = 2000.0f;
		position.y = 3000.0f;
	}
	void pickup(string type1) {
		bullet_type = type1;
	}

	void is_hit(int value) {
		health -= value;
	}
};

void DrawSpriteSheetSprite(ShaderProgram &program, int index, int spriteCountX, int spriteCountY, float size) {
	float u = (float)(((int)index) % spriteCountX) / (float)spriteCountX;
	float v = (float)(((int)index) / spriteCountX) / (float)spriteCountY;
	float spriteWidth = 1.0 / (float)spriteCountX;
	float spriteHeight = 1.0 / (float)spriteCountY;

	float texCoords[] = {
	u, v + spriteHeight,
	u + spriteWidth, v,
	u, v,
	u + spriteWidth, v,
	u, v + spriteHeight,
	u + spriteWidth, v + spriteHeight
	};

	float vertices[] = { -0.5f*size, -0.5f*size, 0.5f*size, 0.5f*size, -0.5f*size, 0.5f*size, 0.5f*size, 0.5f*size, -0.5f*size,
	-0.5f*size, 0.5f*size, -0.5f*size };
	// draw this data


	glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
	glEnableVertexAttribArray(program.positionAttribute);


	glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
	glEnableVertexAttribArray(program.texCoordAttribute);

	glDrawArrays(GL_TRIANGLES, 0, 6);
	glDisableVertexAttribArray(program.positionAttribute);
	glDisableVertexAttribArray(program.texCoordAttribute);

}

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
	Entity player1;
	Entity player2;
	vector<Entity> bullets1;
	vector<Entity> bullets2;
	vector<Entity> items;
};

enum GameMode { STATE_MAIN_MENU, STATE_GAME_SELECT, STATE_GAME_LEVEL_1, STATE_GAME_LEVEL_2, STATE_GAME_LEVEL_3, STATE_GAME_OVER1, STATE_GAME_OVER2 };
GameMode mode = STATE_MAIN_MENU;
GameState state;

void RenderMainMenu() {

	glm::mat4 textMatrix = glm::mat4(1.0f);
	textMatrix = glm::translate(textMatrix, glm::vec3(0.2f, -0.5f, 0.0f));
	program.SetModelMatrix(textMatrix);
	DrawText(program, fontTexture, "The Result of Sleepless Nights", 0.25f, -0.15f);

	textMatrix = glm::mat4(1.0f);
	textMatrix = glm::translate(textMatrix, glm::vec3(0.65f, -0.9f, 0.0f));
	program.SetModelMatrix(textMatrix);
	DrawText(program, fontTexture, "Press Space to start!", 0.2f, -0.1f);

	textMatrix = glm::mat4(1.0f);
	textMatrix = glm::translate(textMatrix, glm::vec3(0.7f, -1.1f, 0.0f));
	program.SetModelMatrix(textMatrix);
	DrawText(program, fontTexture, "Arrow keys (P1) and WASD (P2) to move.", 0.1f, -0.05f);

	textMatrix = glm::mat4(1.0f);
	textMatrix = glm::translate(textMatrix, glm::vec3(0.7f, -1.3f, 0.0f));
	program.SetModelMatrix(textMatrix);
	DrawText(program, fontTexture, "] (P1) and TAB (P2) to pick up weapons.", 0.1f, -0.05f);

	textMatrix = glm::mat4(1.0f);
	textMatrix = glm::translate(textMatrix, glm::vec3(0.5f, -1.5f, 0.0f));
	program.SetModelMatrix(textMatrix);
	DrawText(program, fontTexture, "Right shift (P1) and Left shift (P2) to shoot.", 0.1f, -0.05f);

	textMatrix = glm::mat4(1.0f);
	textMatrix = glm::translate(textMatrix, glm::vec3(1.35f, -1.7f, 0.0f));
	program.SetModelMatrix(textMatrix);
	DrawText(program, fontTexture, "Esc to quit.", 0.1f, -0.05f);

}

void RenderGameOver1() {
	glm::mat4 textMatrix = glm::mat4(1.0f);
	textMatrix = glm::translate(textMatrix, glm::vec3(0.8f, -0.6f, 0.0f));
	program.SetModelMatrix(textMatrix);
	DrawText(program, fontTexture, "Player 1 Wins!", 0.25f, -0.12f);

	textMatrix = glm::mat4(1.0f);
	textMatrix = glm::translate(textMatrix, glm::vec3(0.6f, -1.0f, 0.0f));
	program.SetModelMatrix(textMatrix);
	DrawText(program, fontTexture, "Press Esc to quit.", 0.25f, -0.12f);
}

void RenderGameOver2() {
	glm::mat4 textMatrix = glm::mat4(1.0f);
	textMatrix = glm::translate(textMatrix, glm::vec3(0.8f, -0.6f, 0.0f));
	program.SetModelMatrix(textMatrix);
	DrawText(program, fontTexture, "Player 2 Wins!", 0.25f, -0.12f);

	textMatrix = glm::mat4(1.0f);
	textMatrix = glm::translate(textMatrix, glm::vec3(0.6f, -1.0f, 0.0f));
	program.SetModelMatrix(textMatrix);
	DrawText(program, fontTexture, "Press Esc to quit.", 0.25f, -0.12f);
}

void RenderGameSelect() {
	glm::mat4 textMatrix = glm::mat4(1.0f);
	textMatrix = glm::translate(textMatrix, glm::vec3(0.9f, -0.4f, 0.0f));
	program.SetModelMatrix(textMatrix);
	DrawText(program, fontTexture, "Choose a map.", 0.25f, -0.12f);

	textMatrix = glm::mat4(1.0f);
	textMatrix = glm::translate(textMatrix, glm::vec3(0.85f, -2.0f, 0.0f));
	program.SetModelMatrix(textMatrix);
	DrawText(program, fontTexture, "Press 1 2 or 3.", 0.25f, -0.12f);
}

Entity player1;
Entity player2;


void UpdateMainMenu(float elapsed) {}

void RenderGameLevel1() {
	Draw(program);
	state.player1.Draw(program);
	state.player2.Draw(program);
	for (size_t i = 0; i < state.bullets1.size(); i++) {
		state.bullets1[i].Draw(program);
	}
	for (size_t i = 0; i < state.bullets2.size(); i++) {
		state.bullets2[i].Draw(program);
	}
	for (size_t i = 0; i < state.items.size(); i++) {
		state.items[i].Draw1(program);
	}
	glm::mat4 textMatrix = glm::mat4(1.0f);
	textMatrix = glm::translate(textMatrix, glm::vec3(1.25f, -3.15f, 0.0f));
	program.SetModelMatrix(textMatrix);
	string healthstr;
	stringstream health; 
	health << state.player1.health;
	healthstr = health.str();
	string typestr;
	stringstream type;
	type << state.player1.bullet_type;
	typestr = type.str();
	DrawText(program, fontTexture, " Weapon: " + typestr + " Player 1 Health: "+healthstr, 0.1f, -0.05f);

	textMatrix = glm::mat4(1.0f);
	textMatrix = glm::translate(textMatrix, glm::vec3(0.05f, -0.05f, 0.0f));
	program.SetModelMatrix(textMatrix);
	string healthstr2;
	stringstream health2;
	health2 << state.player2.health;
	healthstr2 = health2.str();
	string typestr2;
	stringstream type2;
	type2 << state.player2.bullet_type;
	typestr2 = type2.str();
	DrawText(program, fontTexture, "Player 2 Health: " + healthstr2+ " Weapon: " + typestr2, 0.1f, -0.05f);
}

void RenderGameLevel2() {
	Draw(program);
	state.player1.Draw(program);
	state.player2.Draw(program);
	for (size_t i = 0; i < state.bullets1.size(); i++) {
		state.bullets1[i].Draw(program);
	}
	for (size_t i = 0; i < state.bullets2.size(); i++) {
		state.bullets2[i].Draw(program);
	}
	for (size_t i = 0; i < state.items.size(); i++) {
		state.items[i].Draw1(program);
	}
	glm::mat4 textMatrix = glm::mat4(1.0f);
	textMatrix = glm::translate(textMatrix, glm::vec3(1.25f, -3.15f, 0.0f));
	program.SetModelMatrix(textMatrix);
	string healthstr;
	stringstream health;
	health << state.player1.health;
	healthstr = health.str();
	string typestr;
	stringstream type;
	type << state.player1.bullet_type;
	typestr = type.str();
	DrawText(program, fontTexture, " Weapon: " + typestr + " Player 1 Health: " + healthstr, 0.1f, -0.05f);

	textMatrix = glm::mat4(1.0f);
	textMatrix = glm::translate(textMatrix, glm::vec3(0.05f, -0.05f, 0.0f));
	program.SetModelMatrix(textMatrix);
	string healthstr2;
	stringstream health2;
	health2 << state.player2.health;
	healthstr2 = health2.str();
	string typestr2;
	stringstream type2;
	type2 << state.player2.bullet_type;
	typestr2 = type2.str();
	DrawText(program, fontTexture, "Player 2 Health: " + healthstr2 + " Weapon: " + typestr2, 0.1f, -0.05f);
}

void RenderGameLevel3() {
	Draw(program);
	state.player1.Draw(program);
	state.player2.Draw(program);
	for (size_t i = 0; i < state.bullets1.size(); i++) {
		state.bullets1[i].Draw(program);
	}
	for (size_t i = 0; i < state.bullets2.size(); i++) {
		state.bullets2[i].Draw(program);
	}
	for (size_t i = 0; i < state.items.size(); i++) {
		state.items[i].Draw1(program);
	}
	glm::mat4 textMatrix = glm::mat4(1.0f);
	textMatrix = glm::translate(textMatrix, glm::vec3(1.25f, -3.15f, 0.0f));
	program.SetModelMatrix(textMatrix);
	string healthstr;
	stringstream health;
	health << state.player1.health;
	healthstr = health.str();
	string typestr;
	stringstream type;
	type << state.player1.bullet_type;
	typestr = type.str();
	DrawText(program, fontTexture, " Weapon: " + typestr + " Player 1 Health: " + healthstr, 0.1f, -0.05f);

	textMatrix = glm::mat4(1.0f);
	textMatrix = glm::translate(textMatrix, glm::vec3(0.05f, -0.05f, 0.0f));
	program.SetModelMatrix(textMatrix);
	string healthstr2;
	stringstream health2;
	health2 << state.player2.health;
	healthstr2 = health2.str();
	string typestr2;
	stringstream type2;
	type2 << state.player2.bullet_type;
	typestr2 = type2.str();
	DrawText(program, fontTexture, "Player 2 Health: " + healthstr2 + " Weapon: " + typestr2, 0.1f, -0.05f);	
}

void shootBullet1() {
	if (state.player1.bullet_type == "default") {
		Entity newBullet;
		newBullet.position.x = state.player1.position.x;
		newBullet.position.y = state.player1.position.y;
		DrawSpriteSheetSprite(program, 26, SPRITE_COUNT_X, SPRITE_COUNT_Y, TILE_SIZE);
		if (state.player1.facing == "up") {
			newBullet.velocity.y = 1.5f;
		}
		else if (state.player1.facing == "down") {
			newBullet.velocity.y = -1.5f;
		}
		else if (state.player1.facing == "left") {
			newBullet.velocity.x = -1.5f;
		}
		else if (state.player1.facing == "right") {
			newBullet.velocity.x = 1.5f;
		}
		newBullet.timeAlive = 0.0f;
		if (state.player1.type == "player1") {
			state.bullets1.push_back(newBullet);
		}
		else if (state.player1.type == "player2") {
			state.bullets2.push_back(newBullet);
		}

	}
	if (state.player1.bullet_type == "Heavy") {
		Entity newBullet;
		newBullet.position.x = state.player1.position.x;
		newBullet.position.y = state.player1.position.y;
		DrawSpriteSheetSprite(program, 8, SPRITE_COUNT_X, SPRITE_COUNT_Y, TILE_SIZE);
		if (state.player1.facing == "up") {
			newBullet.velocity.y = 1.0f;
		}
		else if (state.player1.facing == "down") {
			newBullet.velocity.y = -1.0f;
		}
		else if (state.player1.facing == "left") {
			newBullet.velocity.x = -1.0f;
		}
		else if (state.player1.facing == "right") {
			newBullet.velocity.x = 1.0f;
		}
		newBullet.timeAlive = 0.0f;
		if (state.player1.type == "player1") {
			state.bullets1.push_back(newBullet);
		}
		else if (state.player1.type == "player2") {
			state.bullets2.push_back(newBullet);
		}
	}
	if (state.player1.bullet_type == "Wave") {
		Entity newBullet;
		newBullet.position.x = state.player1.position.x;
		newBullet.position.y = state.player1.position.y;
		DrawSpriteSheetSprite(program, 37, SPRITE_COUNT_X, SPRITE_COUNT_Y, TILE_SIZE);
		if (state.player1.facing == "up") {
			newBullet.velocity.y = 2.0f;
		}
		else if (state.player1.facing == "down") {
			newBullet.velocity.y = -2.0f;
		}
		else if (state.player1.facing == "left") {
			newBullet.velocity.x = -2.0f;
		}
		else if (state.player1.facing == "right") {
			newBullet.velocity.x = 2.0f;
		}
		newBullet.timeAlive = 0.0f;
		if (state.player1.type == "player1") {
			state.bullets1.push_back(newBullet);
		}
		else if (state.player1.type == "player2") {
			state.bullets2.push_back(newBullet);
		}
	}
	if (state.player1.bullet_type == "Spark") {
		Entity newBullet;
		newBullet.position.x = state.player1.position.x;
		newBullet.position.y = state.player1.position.y;
		DrawSpriteSheetSprite(program, 248, SPRITE_COUNT_X, SPRITE_COUNT_Y, TILE_SIZE);
		if (state.player1.facing == "up") {
			newBullet.velocity.y = 10.0f;
		}
		else if (state.player1.facing == "down") {
			newBullet.velocity.y = -10.0f;
		}
		else if (state.player1.facing == "left") {
			newBullet.velocity.x = -10.0f;
		}
		else if (state.player1.facing == "right") {
			newBullet.velocity.x = 10.0f;
		}
		newBullet.timeAlive = 0.0f;
		if (state.player1.type == "player1") {
			state.bullets1.push_back(newBullet);
		}
		else if (state.player1.type == "player2") {
			state.bullets2.push_back(newBullet);
		}
	}

}
void shootBullet2() {
	if (state.player2.bullet_type == "default") {
		Entity newBullet;
		newBullet.position.x = state.player2.position.x;
		newBullet.position.y = state.player2.position.y;
		DrawSpriteSheetSprite(program, 26, SPRITE_COUNT_X, SPRITE_COUNT_Y, TILE_SIZE);
		if (state.player2.facing == "up") {
			newBullet.velocity.y = 1.5f;
		}
		else if (state.player2.facing == "down") {
			newBullet.velocity.y = -1.5f;
		}
		else if (state.player2.facing == "left") {
			newBullet.velocity.x = -1.5f;
		}
		else if (state.player2.facing == "right") {
			newBullet.velocity.x = 1.5f;
		}
		newBullet.timeAlive = 0.0f;
		if (state.player2.type == "player1") {
			state.bullets1.push_back(newBullet);
		}
		else if (state.player2.type == "player2") {
			state.bullets2.push_back(newBullet);
		}

	}
	if (state.player2.bullet_type == "Heavy") {
		Entity newBullet;
		newBullet.position.x = state.player2.position.x;
		newBullet.position.y = state.player2.position.y;
		DrawSpriteSheetSprite(program, 8, SPRITE_COUNT_X, SPRITE_COUNT_Y, TILE_SIZE);
		if (state.player2.facing == "up") {
			newBullet.velocity.y = 1.0f;
		}
		else if (state.player2.facing == "down") {
			newBullet.velocity.y = -1.0f;
		}
		else if (state.player2.facing == "left") {
			newBullet.velocity.x = -1.0f;
		}
		else if (state.player2.facing == "right") {
			newBullet.velocity.x = 1.0f;
		}
		newBullet.timeAlive = 0.0f;
		if (state.player2.type == "player1") {
			state.bullets1.push_back(newBullet);
		}
		else if (state.player2.type == "player2") {
			state.bullets2.push_back(newBullet);
		}
	}
	if (state.player2.bullet_type == "Wave") {
		Entity newBullet;
		newBullet.position.x = state.player2.position.x;
		newBullet.position.y = state.player2.position.y;
		DrawSpriteSheetSprite(program, 37, SPRITE_COUNT_X, SPRITE_COUNT_Y, TILE_SIZE);
		if (state.player2.facing == "up") {
			newBullet.velocity.y = 2.0f;
		}
		else if (state.player2.facing == "down") {
			newBullet.velocity.y = -2.0f;
		}
		else if (state.player2.facing == "left") {
			newBullet.velocity.x = -2.0f;
		}
		else if (state.player2.facing == "right") {
			newBullet.velocity.x = 2.0f;
		}
		newBullet.timeAlive = 0.0f;
		if (state.player2.type == "player1") {
			state.bullets1.push_back(newBullet);
		}
		else if (state.player2.type == "player2") {
			state.bullets2.push_back(newBullet);
		}
	}
	if (state.player2.bullet_type == "Spark") {
		Entity newBullet;
		newBullet.position.x = state.player2.position.x;
		newBullet.position.y = state.player2.position.y;
		DrawSpriteSheetSprite(program, 248, SPRITE_COUNT_X, SPRITE_COUNT_Y, TILE_SIZE);
		if (state.player2.facing == "up") {
			newBullet.velocity.y = 10.0f;
		}
		else if (state.player2.facing == "down") {
			newBullet.velocity.y = -10.0f;
		}
		else if (state.player2.facing == "left") {
			newBullet.velocity.x = -10.0f;
		}
		else if (state.player2.facing == "right") {
			newBullet.velocity.x = 10.0f;
		}
		newBullet.timeAlive = 0.0f;
		if (state.player2.type == "player1") {
			state.bullets1.push_back(newBullet);
		}
		else if (state.player2.type == "player2") {
			state.bullets2.push_back(newBullet);
		}
	}

}


void UpdateGameLevel(float elapsed) {
	state.player1.velocity.x = 0.0f;
	state.player1.velocity.y = 0.0f;
	state.player2.velocity.x = 0.0f;
	state.player2.velocity.y = 0.0f;
	const Uint8 *keys = SDL_GetKeyboardState(NULL);
	if (keys[SDL_SCANCODE_LEFT]) {
		state.player1.velocity.x = -0.5f;
		state.player1.facing = "left";
	}
	else if (keys[SDL_SCANCODE_RIGHT]) {
		state.player1.velocity.x = 0.5f;
		state.player1.facing = "right";
	}
	if (keys[SDL_SCANCODE_UP]) {
		state.player1.velocity.y = 0.5f;
		state.player1.facing = "up";
	}
	else if (keys[SDL_SCANCODE_DOWN]) {
		state.player1.velocity.y = -0.5f;
		state.player1.facing = "down";
	}
	if (keys[SDL_SCANCODE_A]) {
		state.player2.velocity.x = -0.5f;
		state.player2.facing = "left";
	}
	else if (keys[SDL_SCANCODE_D]) {
		state.player2.velocity.x = 0.5f;
		state.player2.facing = "right";
	}
	if (keys[SDL_SCANCODE_W]) {
		state.player2.velocity.y = 0.5f;
		state.player2.facing = "up";
	}
	else if (keys[SDL_SCANCODE_S]) {
		state.player2.velocity.y = -0.5f;
		state.player2.facing = "down";
	}


	state.player1.position.y += state.player1.velocity.y * elapsed;
	float playerBottom = (state.player1.position.y - state.player1.size.y / 2.0f);
	float playerTop = (state.player1.position.y + state.player1.size.y / 2.0f);
	pair<int, int> tiledcoord = worldToTileCoordinates(state.player1.position.x, playerBottom);
	int gridY = tiledcoord.second;
	int gridX = tiledcoord.first;
	if (gridY >= 0 && gridX >= 0 && map.mapData[gridY][gridX] != 0) {
		float penetration = fabs((-TILE_SIZE * gridY) - (state.player1.position.y - state.player1.size.y / 2.0f));
		state.player1.position.y += 0.01f;
	}
	pair<int, int> tiledcoord2 = worldToTileCoordinates(state.player1.position.x, playerTop);
	int gridY2 = tiledcoord2.second;
	int gridX2 = tiledcoord2.first;
	if (gridY2 >= 0 && gridX2 >= 0 && map.mapData[gridY2][gridX2] != 0) {
		float penetration = fabs((state.player1.position.y + state.player1.size.y / 2.0f) - (-TILE_SIZE * gridY2) - TILE_SIZE);
		state.player1.position.y -= 0.01f;  
	}

	/*
	float bulletBottom = (state.bullets1[i].position.y - state.bullets[i].size.y / 2.0f);
	float bulletTop = (state.bullets1[i].position.y + state.bullets1[i].size.y / 2.0f);
	float bulletLeft = (state.bullets1[i].position.x - state.bullets1[i].size.x / 2.0f);
	float bulletRight = (state.bullets1[i].position.x + state.bullets1[i].size.x / 2.0f);
	pair<int, int> tiledcoord9 = worldToTileCoordinates(state.bullets1[i].position.x, bulletBottom);
	int gridY9 = tiledcoord9.second;
	int gridX9 = tiledcoord9.first;
	if (gridY9 >= 0 && gridX9 >= 0 && map.mapData[gridY9][gridX9] != 0) {
		state.bullets1.erase(state.bullets1.begin() + i);
	}
	pair<int, int> tiledcoord10 = worldToTileCoordinates(state.bullets1[i].position.x, bulletTop);
	int gridY10 = tiledcoord10.second;
	int gridX10 = tiledcoord10.first;
	if (gridY10 >= 0 && gridX10 >= 0 && map.mapData[gridY10][gridX10] != 0) {
	state.bullets1.erase(state.bullets1.begin() + i);
	}
	*/

	state.player1.position.x += state.player1.velocity.x * elapsed;
	float playerLeft = (state.player1.position.x - state.player1.size.x / 2.0f);
	float playerRight = (state.player1.position.x + state.player1.size.x / 2.0f);
	pair<int, int> tiledcoord5 = worldToTileCoordinates(playerLeft, state.player1.position.y);
	int gridY5 = tiledcoord5.second;
	int gridX5 = tiledcoord5.first;
	if (gridY5 >= 0 && gridX5 >= 0 && map.mapData[gridY5][gridX5] != 0) {
		float penetration = fabs((-TILE_SIZE * gridX5) + TILE_SIZE - (state.player1.position.x - state.player1.size.x / 2.0f));
		state.player1.position.x +=  0.01f;
	}
	pair<int, int> tiledcoord6 = worldToTileCoordinates(playerRight, state.player1.position.y);
	int gridY6 = tiledcoord6.second;
	int gridX6 = tiledcoord6.first;
	if (gridY6 >= 0 && gridX6 >= 0 && map.mapData[gridY6][gridX6] != 0) {
		float penetration = fabs((state.player1.position.x + state.player1.size.x / 2.0f) - (-TILE_SIZE * gridX6));
		state.player1.position.x -= 0.01f;
	}

	state.player2.position.y += state.player2.velocity.y * elapsed;
	float playerBottom2 = (state.player2.position.y - state.player2.size.y / 2.0f);
	float playerTop2 = (state.player2.position.y + state.player2.size.y / 2.0f);
	pair<int, int> tiledcoord3 = worldToTileCoordinates(state.player2.position.x, playerBottom2);
	int gridY3 = tiledcoord3.second;
	int gridX3 = tiledcoord3.first;
	if (gridY3 >= 0 && gridX3 >= 0 && map.mapData[gridY3][gridX3] != 0) {
		float penetration = fabs((-TILE_SIZE * gridY3) - (state.player2.position.y - state.player2.size.y / 2.0f));
		state.player2.position.y +=  0.01f;
	}
	pair<int, int> tiledcoord4 = worldToTileCoordinates(state.player2.position.x, playerTop2);
	int gridY4 = tiledcoord4.second;
	int gridX4 = tiledcoord4.first;
	if (gridY4 >= 0 && gridX4 >= 0 && map.mapData[gridY4][gridX4] != 0) {
		float penetration = fabs((state.player2.position.y + state.player2.size.y / 2.0f) - (-TILE_SIZE * gridY4) - TILE_SIZE);
		state.player2.position.y -=  0.01f;
	}


	state.player2.position.x += state.player2.velocity.x * elapsed;
	float playerLeft2 = (state.player2.position.x - state.player2.size.x / 2.0f);
	float playerRight2 = (state.player2.position.x + state.player2.size.x / 2.0f);
	pair<int, int> tiledcoord7 = worldToTileCoordinates(playerLeft2, state.player2.position.y);
	int gridY7 = tiledcoord7.second;
	int gridX7 = tiledcoord7.first;
	if (gridY7 >= 0 && gridX7 >= 0 && map.mapData[gridY7][gridX7] != 0) {
		float penetration = fabs((-TILE_SIZE * gridX7) + TILE_SIZE - (state.player2.position.x - state.player2.size.x / 2.0f));
		state.player2.position.x +=  0.01f;
	}
	pair<int, int> tiledcoord8 = worldToTileCoordinates(playerRight2, state.player2.position.y);
	int gridY8 = tiledcoord8.second;
	int gridX8 = tiledcoord8.first;
	if (gridY8 >= 0 && gridX8 >= 0 && map.mapData[gridY8][gridX8] != 0) {
		float penetration = fabs((state.player2.position.x + state.player2.size.x / 2.0f) - (-TILE_SIZE * gridX8));
		state.player2.position.x -= 0.01f;
	}

	state.player1.sincelastshot += elapsed;
	state.player2.sincelastshot += elapsed;

	if (keys[SDL_SCANCODE_RSHIFT]) {
		if (mode == STATE_GAME_LEVEL_1 || mode == STATE_GAME_LEVEL_2 || mode == STATE_GAME_LEVEL_3) {
			if (state.player1.sincelastshot >= 0.5f) {
				shootBullet1();
				state.player1.sincelastshot = 0.0f;
			}
		}
	}
	if (keys[SDL_SCANCODE_LSHIFT]) {
		if (mode == STATE_GAME_LEVEL_1 || mode == STATE_GAME_LEVEL_2 || mode == STATE_GAME_LEVEL_3) {
			if (state.player2.sincelastshot >= 0.5f) {
				shootBullet2();
				state.player2.sincelastshot = 0.0f;
			}
		}
	}
	if (keys[SDL_SCANCODE_RIGHTBRACKET]) {
		if (mode == STATE_GAME_LEVEL_1 || mode == STATE_GAME_LEVEL_2 || mode == STATE_GAME_LEVEL_3) {
			for (size_t i = 0; i < state.items.size(); i++) {
				float p1 = (fabs(state.items[i].position.x - state.player1.position.x) - ((state.items[i].size.x) + (state.player1.size.x)) / 2.0f);
				float p2 = (fabs(state.items[i].position.y - state.player1.position.y) - ((state.items[i].size.y) + (state.player1.size.y)) / 2.0f);
				if (p1 < 0 && p2 < 0) {
					state.player1.pickup(state.items[i].type);

				}
			}
		}
	}
	if (keys[SDL_SCANCODE_TAB]) {
		if (mode == STATE_GAME_LEVEL_1 || mode == STATE_GAME_LEVEL_2 || mode == STATE_GAME_LEVEL_3) {
			for (size_t i = 0; i < state.items.size(); i++) {
				float p1 = (fabs(state.items[i].position.x - state.player2.position.x) - ((state.items[i].size.x) + (state.player2.size.x)) / 2.0f);
				float p2 = (fabs(state.items[i].position.y - state.player2.position.y) - ((state.items[i].size.y) + (state.player2.size.y)) / 2.0f);
				if (p1 < 0 && p2 < 0) {
						state.player2.pickup(state.items[i].type);
				}
			}
		}
	}


	for (size_t i = 0; i < state.bullets1.size(); i++) {
		state.bullets1[i].position.x += state.bullets1[i].velocity.x*elapsed;
		state.bullets1[i].position.y += state.bullets1[i].velocity.y*elapsed;
		float p1 = (fabs(state.bullets1[i].position.x - state.player2.position.x) - ((state.bullets1[i].size.x) + (state.player2.size.x)) / 2.0f);
		float p2 = (fabs(state.bullets1[i].position.y - state.player2.position.y) - ((state.bullets1[i].size.y) + (state.player2.size.y)) / 2.0f);

		if (p1 < 0 && p2 < 0) {
			state.bullets1[i].remove();
			if (state.player1.bullet_type == "default") {
				state.player2.is_hit(10);
			}
			if (state.player1.bullet_type == "Heavy") {
				state.player2.is_hit(50);
			}
			if (state.player1.bullet_type == "Wave") {
				state.player2.is_hit(25);
			}
			if (state.player1.bullet_type == "Spark") {
				state.player2.is_hit(5);
			}
		}

	}

	for (size_t i = 0; i < state.bullets2.size(); i++) {
		state.bullets2[i].position.x += state.bullets2[i].velocity.x*elapsed;
		state.bullets2[i].position.y += state.bullets2[i].velocity.y*elapsed;
		float p1 = (fabs(state.bullets2[i].position.x - state.player1.position.x) - ((state.bullets2[i].size.x) + (state.player1.size.x)) / 2.0f);
		float p2 = (fabs(state.bullets2[i].position.y - state.player1.position.y) - ((state.bullets2[i].size.y) + (state.player1.size.y)) / 2.0f);
		if (p1 < 0 && p2 < 0) {
			state.bullets2[i].remove();
			if (state.player2.bullet_type == "default") {
				state.player1.is_hit(10);
			}
			if (state.player2.bullet_type == "Heavy") {
				state.player1.is_hit(50);
			}
			if (state.player2.bullet_type == "Wave") {
				state.player1.is_hit(25);
			}
			if (state.player2.bullet_type == "Spark") {
				state.player1.is_hit(5);
			}
		}

	}

	for (size_t i = 0; i < state.bullets1.size(); i++) {
		state.bullets1[i].timeAlive += elapsed;
		if (state.bullets1[i].timeAlive >= 3.0f) {
			state.bullets1.erase(state.bullets1.begin() + i);
		}
	}

	for (size_t i = 0; i < state.bullets2.size(); i++) {
		state.bullets2[i].timeAlive += elapsed;
		if (state.bullets2[i].timeAlive >= 3.0f) {
			state.bullets2.erase(state.bullets2.begin() + i);
		}
	}

	if (state.player1.health <= 0) {
		mode = STATE_GAME_OVER2;

	}
	if (state.player2.health <= 0) {
		mode = STATE_GAME_OVER1;
	}

	glm::mat4 modelMatrix = glm::mat4(1.0f);
	program.SetModelMatrix(modelMatrix);


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
	case STATE_GAME_SELECT:
		RenderGameSelect();
		break;
	case STATE_GAME_LEVEL_1:
		RenderGameLevel1();
		break;
	case STATE_GAME_LEVEL_2:
		RenderGameLevel2();
		break;
	case STATE_GAME_LEVEL_3:
		RenderGameLevel3();
		break;
	case STATE_GAME_OVER1:
		RenderGameOver1();
		break;
	case STATE_GAME_OVER2:
		RenderGameOver2();
		break;

	}
}

void Update(float elapsed) {
	switch (mode) {
	case STATE_MAIN_MENU:
		UpdateMainMenu(elapsed);
		break;
	case STATE_GAME_LEVEL_1:
		UpdateGameLevel(elapsed);
		break;
	case STATE_GAME_LEVEL_2:
		UpdateGameLevel(elapsed);
		break;
	case STATE_GAME_LEVEL_3:
		UpdateGameLevel(elapsed);
		break;
	}

}


int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("Final Game.", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 640, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);

#ifdef _WINDOWS
	glewInit();
#endif

	glViewport(0, 0, 640, 640);

	program.Load(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");

	glm::mat4 projectionMatrix = glm::mat4(1.0f);
	glm::mat4 viewMatrix = glm::mat4(1.0f);
	
	projectionMatrix = glm::ortho(-1.777f, 1.777f, -1.777f, 1.777f, -1.0f, 1.0f);
	viewMatrix = glm::translate(viewMatrix, glm::vec3(-1.6f, 1.55f, 0.0f));

	program.SetViewMatrix(viewMatrix);
	program.SetProjectionMatrix(projectionMatrix);

	fontTexture = LoadTexture(RESOURCE_FOLDER"font1.png");
	spriteTexture = LoadTexture(RESOURCE_FOLDER"arne_sprites.png");

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	state.player1 = player1;
	state.player2 = player2;
	state.bullets1 = bullets1;
	state.bullets2 = bullets2;
	state.items = items;

	Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096);

	Mix_Chunk *DefaultGunSound;
	DefaultGunSound = Mix_LoadWAV("defaultgunsound.wav");

	Mix_Chunk *HeavyGunSound;
	HeavyGunSound = Mix_LoadWAV("heavygunsound.wav");

	Mix_Chunk *WaveGunSound;
	WaveGunSound = Mix_LoadWAV("wavegunsound.wav");

	Mix_Chunk *SparkGunSound;
	SparkGunSound = Mix_LoadWAV("sparkgunsound.wav");
//	Mix_PlayChannel(-1, SparkGunSound, 5);

	Mix_Chunk *Music;
	Music = Mix_LoadWAV("8-Bit-Mayhem.wav");
	Mix_PlayChannel(-1, Music, -1);
	//Mix_Music *music;
	//music = Mix_LoadMUS("8-Bit-Mayhem.mp3");
	//Mix_PlayMusic(music, -1);

	glUseProgram(program.programID);

	SDL_Event event;
	bool done = false;

	while (!done) {
	
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
			if (event.type == SDL_KEYDOWN) {
				if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
					done = true;
				}
				if (event.key.keysym.scancode == SDL_SCANCODE_SPACE) {
					if (mode == STATE_MAIN_MENU) {
						mode = STATE_GAME_SELECT;
					}
				}
				if (mode == STATE_GAME_SELECT && event.key.keysym.scancode == SDL_SCANCODE_1) {
					mode = STATE_GAME_LEVEL_1;
					map.Load("Level1.txt");
					for (FlareMapEntity a : map.entities) {
						if (a.type == "Player1") {
							Entity newEntity(a.x*TILE_SIZE, a.y*-TILE_SIZE, 75);
							state.player1 = newEntity;
							state.player1.type = "player1";

						}
						if (a.type == "Player2") {
							Entity newEntity(a.x*TILE_SIZE, a.y*-TILE_SIZE, 59);
							state.player2 = newEntity;
							state.player2.type = "player2";
						}
						else if (a.type == "Heavy") {
							Entity newEntity(a.x*TILE_SIZE, a.y*-TILE_SIZE, 8);
							newEntity.type = "Heavy";
							state.items.push_back(newEntity);
						}
						else if (a.type == "Wave") {
							Entity newEntity(a.x*TILE_SIZE, a.y*-TILE_SIZE, 37);
							newEntity.type = "Wave";
							state.items.push_back(newEntity);
						}
						else if (a.type == "Spark") {
							Entity newEntity(a.x*TILE_SIZE, a.y*-TILE_SIZE, 48);
							newEntity.type = "Spark";
							state.items.push_back(newEntity);
						}
					}
				}
				if (mode == STATE_GAME_SELECT && event.key.keysym.scancode == SDL_SCANCODE_2) {
					mode = STATE_GAME_LEVEL_2;
					map.Load("Level2.txt");
					for (FlareMapEntity a : map.entities) {
						if (a.type == "Player1") {
							Entity newEntity(a.x*TILE_SIZE, a.y*-TILE_SIZE, 75);
							state.player1 = newEntity;
							state.player1.type = "player1";
							state.player1.velocity.x = 0.5f;
							state.player1.velocity.y = 0.5f;
						}
						if (a.type == "Player2") {
							Entity newEntity(a.x*TILE_SIZE, a.y*-TILE_SIZE, 59);
							state.player2 = newEntity;
							state.player2.type = "player2";
							state.player2.velocity.x = 0.5f;
							state.player2.velocity.y = 0.5f;
						}
						else if (a.type == "Heavy") {
							Entity newEntity(a.x*TILE_SIZE, a.y*-TILE_SIZE, 8);
							newEntity.type = "Heavy";
							state.items.push_back(newEntity);
						}
						else if (a.type == "Wave") {
							Entity newEntity(a.x*TILE_SIZE, a.y*-TILE_SIZE, 37);
							newEntity.type = "Wave";
							state.items.push_back(newEntity);
						}
						else if (a.type == "Spark") {
							Entity newEntity(a.x*TILE_SIZE, a.y*-TILE_SIZE, 48);
							newEntity.type = "Spark";
							state.items.push_back(newEntity);
						}
					}
				}
				if (mode == STATE_GAME_SELECT && event.key.keysym.scancode == SDL_SCANCODE_3) {
					mode = STATE_GAME_LEVEL_3;
					map.Load("Level3.txt");
					for (FlareMapEntity a : map.entities) {
						if (a.type == "Player1") {
							Entity newEntity(a.x*TILE_SIZE, a.y*-TILE_SIZE, 75);
							state.player1 = newEntity;
							state.player1.type = "player1";
							state.player1.velocity.x = 0.5f;
							state.player1.velocity.y = 0.5f;
						}
						if (a.type == "Player2") {
							Entity newEntity(a.x*TILE_SIZE, a.y*-TILE_SIZE, 59);
							state.player2 = newEntity;
							state.player2.type = "player2";
							state.player2.velocity.x = 0.8f;
							state.player2.velocity.y = 0.8f;
						}
						else if (a.type == "Heavy") {
							Entity newEntity(a.x*TILE_SIZE, a.y*-TILE_SIZE, 8);
							newEntity.type = "Heavy";
							state.items.push_back(newEntity);
						}
						else if (a.type == "Wave") {
							Entity newEntity(a.x*TILE_SIZE, a.y*-TILE_SIZE, 37);
							newEntity.type = "Wave";
							state.items.push_back(newEntity);
						}
						else if (a.type == "Spark") {
							Entity newEntity(a.x*TILE_SIZE, a.y*-TILE_SIZE, 48);
							newEntity.type = "Spark";
							state.items.push_back(newEntity);
						}
					}
				}
				if (event.key.keysym.scancode == SDL_SCANCODE_LSHIFT) {
					if (state.player2.bullet_type == "default" && state.player2.sincelastshot >= 0.5f) {
						Mix_PlayChannel(-1, DefaultGunSound, 0);
					}
					else if (state.player2.bullet_type == "Heavy" && state.player2.sincelastshot >= 0.5f) {
						Mix_PlayChannel(-1, HeavyGunSound, 0);
					}
					else if (state.player2.bullet_type == "Wave" && state.player2.sincelastshot >= 0.5f) {
						Mix_PlayChannel(-1, WaveGunSound, 0);
					}
					else if (state.player2.bullet_type == "Spark" && state.player2.sincelastshot >= 0.5f) {
						Mix_PlayChannel(-1, SparkGunSound, 0);
					}
				}
				if (event.key.keysym.scancode == SDL_SCANCODE_RSHIFT) {
					if (state.player1.bullet_type == "default" && state.player1.sincelastshot >= 0.5f) {
						Mix_PlayChannel(-1, DefaultGunSound, 0);
					}
					else if (state.player1.bullet_type == "Heavy" && state.player1.sincelastshot >= 0.5f) {
						Mix_PlayChannel(-1, HeavyGunSound, 0);
					}
					else if (state.player1.bullet_type == "Wave" && state.player1.sincelastshot >= 0.5f) {
						Mix_PlayChannel(-1, WaveGunSound, 0);
					}
					else if (state.player1.bullet_type == "Spark" && state.player1.sincelastshot >= 0.5f) {
						Mix_PlayChannel(-1, SparkGunSound, 0);
					}
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
