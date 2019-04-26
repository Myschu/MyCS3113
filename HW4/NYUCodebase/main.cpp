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

float lerp(float v0, float v1, float t) {
	return (1.0 - t)*v0 + t * v1;
}

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

	GLuint textureID;
	int index;
	string type;
	float timeAlive;

	glm::vec3 position;
	glm::vec3 velocity;
	glm::vec3 acceleration;
	glm::vec3 size;

	void Draw(ShaderProgram& program) {
		glm::mat4 modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, glm::vec3(position.x, position.y, 0.0f));
		program.SetModelMatrix(modelMatrix);
		DrawSpriteSheetSprite(program, index, SPRITE_COUNT_X, SPRITE_COUNT_Y, TILE_SIZE);
	}
	void remove() {
		position.x = 2000.0f;
		position.y = 3000.0f;
	}
};

Entity player;
vector<Entity> enemies;

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

	glDrawArrays(GL_TRIANGLES, 0, int (vertexData.size() / 2));
	glDisableVertexAttribArray(program.positionAttribute);
	glDisableVertexAttribArray(program.texCoordAttribute);
}

pair<int, int> worldToTileCoordinates(float worldX, float worldY) {
	return { (int)(worldX / TILE_SIZE), (int)(worldY / -TILE_SIZE) };
}
/*
void placeEntity(string type, float placeX, float placeY) {
	Entity entity;
	entity.type = type;
	entity.position.x = placeX;
	entity.position.y = placeY;

}
*/
int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("Forming Plats", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
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


	map.Load("Map1.txt");
	spriteTexture = LoadTexture(RESOURCE_FOLDER"arne_sprites.png");


	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glUseProgram(program.programID);

	SDL_Event event;
	bool done = false;
	bool start = false;

	for (FlareMapEntity a : map.entities) {
		if (a.type == "player") {
			Entity newEntity(a.x*TILE_SIZE, a.y*-TILE_SIZE, 98);
			player = newEntity;
		}
		else if (a.type == "enemy") {
			Entity newEntity(a.x*TILE_SIZE, a.y*-TILE_SIZE, 80);
			enemies.push_back(newEntity);
		}
	}

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
		float ticks = (float)SDL_GetTicks() / 1000.0f;
		elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;
		glClear(GL_COLOR_BUFFER_BIT);


		player.acceleration.x = 0.0f;
		player.acceleration.y = -1.0f;
		const Uint8 *keys = SDL_GetKeyboardState(NULL);

		if (keys[SDL_SCANCODE_LEFT]) {
			player.acceleration.x = -1.0f;
		}
		if (keys[SDL_SCANCODE_RIGHT]) {
			player.acceleration.x = 1.0f;
		}
		player.velocity.x = lerp(player.velocity.x, 0.0f, elapsed * friction_x);
		player.velocity.y = lerp(player.velocity.y, 0.0f, elapsed * friction_y);
		player.velocity.x += player.acceleration.x * elapsed;
		player.velocity.y += player.acceleration.y * elapsed;
		player.position.y += player.velocity.y * elapsed;
		//check y collision
		
		float playerBottom = (player.position.y - player.size.y / 2.0f);
		pair<int, int> tiledcoord = worldToTileCoordinates(player.position.x, playerBottom);
		int gridY = tiledcoord.second;
		int gridX = tiledcoord.first;
		if (gridY >= 0 && gridX>=0 && map.mapData[gridY][gridX] != 0) {
			float penetration = fabs((-TILE_SIZE * gridY) - (player.position.y - player.size.y / 2.0f));
			player.position.y += penetration;
		}

		for (size_t i = 0; i < enemies.size(); i++) {
			enemies[i].acceleration.y = -1.0f;
		}

		for (size_t i = 0; i < enemies.size(); i++) {
			float enemyBottom = (enemies[i].position.y - enemies[i].size.y / 2.0f);
			pair<int, int> tiledcoord2 = worldToTileCoordinates(enemies[i].position.x, enemyBottom);
			int gridY2 = tiledcoord2.second;
			int gridX2 = tiledcoord2.first;
			if (gridY2 >= 0 && gridX2 >= 0 && map.mapData[gridY2][gridX2] != 0) {
				float penetration2 = fabs((-TILE_SIZE * gridY2) - (enemies[i].position.y - enemies[i].size.y / 2.0f));
				enemies[i].position.y += penetration2;
			}
		}

		player.position.x += player.velocity.x * elapsed;
		//check x collision

		for (size_t i = 0; i < enemies.size(); i++) {
			float p1 = (abs(enemies[i].position.x - player.position.x) - ((enemies[i].size.x) + (player.size.x)) / 2.0f);
			float p2 = (abs(enemies[i].position.y - player.position.y) - ((enemies[i].size.y) + (player.size.y)) / 2.0f);
			if (p1 < 0 && p2 < 0) {
				enemies[i].remove();
			}
		}


		for (size_t i = 0; i < enemies.size(); i++) {
			enemies[i].acceleration.x = -0.5f;
			enemies[i].velocity.x = lerp(enemies[i].velocity.x, 0.0f, elapsed * friction_x);
			enemies[i].velocity.y = lerp(enemies[i].velocity.y, 0.0f, elapsed * friction_y);
			enemies[i].velocity.x += enemies[i].acceleration.x * elapsed;
			enemies[i].velocity.y += enemies[i].acceleration.y * elapsed;
			enemies[i].position.y += enemies[i].velocity.y * elapsed;
			enemies[i].position.x += enemies[i].velocity.x * elapsed;
		}



		glm::mat4 modelMatrix = glm::mat4(1.0f);
		program.SetModelMatrix(modelMatrix);
		
		
		Draw(program);
		glm::mat4 viewMatrix = glm::mat4(1.0f);
		viewMatrix = glm::translate(viewMatrix, glm::vec3(-player.position.x, -player.position.y, 0.0f));
		program.SetViewMatrix(viewMatrix);
		
		player.Draw(program);
		for (size_t i = 0; i < enemies.size(); i++) {
			enemies[i].Draw(program);
		}


		SDL_GL_SwapWindow(displayWindow);
	}

	SDL_Quit();
	return 0;
}