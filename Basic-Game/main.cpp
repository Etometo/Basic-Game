#include <raylib.h>
#include <iostream>
#include "player.h"
#include "GameState.h"


int main() {
	void* rawMemory = new uint8_t[15 * 1024];
	GameState* gameState = (GameState*)rawMemory;
	gameState->arena.base = (uint8_t*)rawMemory + sizeof(GameState);
	gameState->arena.used = 0;
	gameState->arena.capacity = 5 * 1024 - sizeof(GameState);

	VertexData* vertexData = (VertexData*)PushSize(gameState, sizeof(VertexData) * 4);
	VertexData* vertexDataEnd = vertexData;
	*(vertexDataEnd++) = VertexData{ 50, 50 };
	*(vertexDataEnd++) = VertexData{ 50, -50 };
	*(vertexDataEnd++) = VertexData{ -50, -50 };
	*(vertexDataEnd++) = VertexData{ -50, 50 };
	Player* player = PushAndInitializePlayer(gameState, vertexData, vertexDataEnd);
	player->centerPosition = { 100, 200 };
	for (int i = 0; i < 4; i++) {
		std::cout << player->vertexData[i].position.x << player->vertexData[i].position.y << std::endl;
	}
	throw new std::runtime_error("as");

	InitWindow(800, 800, "nigga");
	SetTargetFPS(60);
	while (!WindowShouldClose()) {
		BeginDrawing();
			ClearBackground(RAYWHITE);
			Vector2 vec1 = { 450, 450 };
			Vector2 vec2 = { 300, 200 };
			Vector2 vec3 = { 150, 450 };
			Color color = { 255, 255, 0, 255 };
			DrawTriangle(vec1,vec2,vec3,color);
			DrawText("Congrats! You created your first window!", 190, 200, 20, color);
			DrawText("window!", 190, 200, 20, color);
		EndDrawing();
	}
	CloseWindow();
	return 0;
}


