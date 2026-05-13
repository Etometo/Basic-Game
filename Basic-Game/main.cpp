#include <raylib.h>
#include <iostream>
#include "player.h"
#include "GameState.h"


int main() {
	void* rawMemory = new uint8_t[150 * 1024];
	memset(rawMemory, 0, 150 * 1024);
	GameState* gameState = (GameState*)rawMemory;
	if (gameState->isInitialized == false) {
		gameState->arena.base = (uint8_t*)rawMemory + sizeof(GameState);
		gameState->arena.used = 0;
		gameState->arena.capacity = 150 * 1024 - sizeof(GameState);
		gameState->entitiesCapacity = 100;
		gameState->goalFps = 60;
		gameState->isInitialized = true;
	}

	VertexData* vertexData = (VertexData*)PushSize(gameState, sizeof(VertexData) * 4);
	VertexData* vertexDataEnd = vertexData;
	*(vertexDataEnd++) = VertexData{ 50, 50 };
	*(vertexDataEnd++) = VertexData{ 50, -50 };
	*(vertexDataEnd++) = VertexData{ -50, -50 };
	*(vertexDataEnd++) = VertexData{ -50, 50 };

	Entity* player = PushAndInitializePlayer(gameState, vertexData, vertexDataEnd);
	player->centerPosition = { 100, 200 };
	Entity* player2 = PushAndInitializePlayer(gameState, vertexData, vertexDataEnd);
	player2->centerPosition = { 300, 200 };
	for (int i = 0; i < 4; i++) {
		std::cout << player->vertexData[i].position.x << player->vertexData[i].position.y << std::endl;
	}

	InitWindow(800, 800, "asd");
	SetTargetFPS(60);
	Entity** relevantEntities = (Entity**)PushSize(gameState, sizeof(Entity*) * 20);
	int numOfRelevantEntities = CalculateRelevantEntitiesFor(gameState, player, relevantEntities);
	std::cout << numOfRelevantEntities << " is the number of importants first importants ";
	while (!WindowShouldClose()) {
		BeginDrawing();
			ClearBackground(RAYWHITE);
			if (IsKeyDown(KEY_W)) {
				MovePlayer(player, { 0, -2 });
			}
			if (IsKeyDown(KEY_S)) {
				MovePlayer(player, { 0, 2 });
			}
			if (IsKeyDown(KEY_A)) {
				MovePlayer(player, { -2, 0 });
			}
			if (IsKeyDown(KEY_D)) {
				MovePlayer(player, { 2, 0 });
			}
			numOfRelevantEntities = CalculateRelevantEntitiesFor(gameState, player, relevantEntities);
			if (numOfRelevantEntities == 0) {
				std::cout << "no relevant entities" << std::endl;
			}
			for (int i = 0; i < numOfRelevantEntities; i++) {
				CalculateAndApplyCollisionWithEntity(player, player2);
			}
			for (int i = 0; i < gameState->addedEntities; i++) {
				DrawPlayer((gameState->entities + i));
			}
		EndDrawing();
	}
	RetractSize(gameState, sizeof(Entity*) * 20);
	CloseWindow();
	return 0;
}


