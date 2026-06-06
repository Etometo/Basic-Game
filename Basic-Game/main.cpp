#include <raylib.h>
#include <iostream>
#include "player.h"
#include "GameState.h"
#include <iostream>


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
		gameState->gravityConstant = 9.8;
	}

	VertexData* rectData = (VertexData*)PushSize(gameState, sizeof(VertexData) * 4);
	VertexData* rectDataEnd = rectData;
	*(rectDataEnd++) = VertexData{ 50, 50 };
	*(rectDataEnd++) = VertexData{ 50, -50 };
	*(rectDataEnd++) = VertexData{ -50, -50 };
	*(rectDataEnd++) = VertexData{ -50, 50 };

	VertexData* floorRectData = (VertexData*)PushSize(gameState, sizeof(VertexData) * 4);
	VertexData* floorRectDataEnd = floorRectData;
	*(floorRectDataEnd++) = VertexData{ 150, 50 };
	*(floorRectDataEnd++) = VertexData{ 150, -50 };
	*(floorRectDataEnd++) = VertexData{ -150, -50 };
	*(floorRectDataEnd++) = VertexData{ -150, 50 };

	VertexData* triData = (VertexData*)PushSize(gameState, sizeof(VertexData) * 3);
	VertexData* triDataEnd = triData;
	*(triDataEnd++) = VertexData{ 50, 50 };
	*(triDataEnd++) = VertexData{ 50, -50 };
	*(triDataEnd++) = VertexData{ -50, -50 };

	uint32_t playerFlags = GRAVITY_FLAG | PLAYER_FLAG;
	Entity* player = PushAndInitializePlayer(gameState, rectData, rectDataEnd, 0.1, playerFlags);
	player->centerPosition = { 100, 400 };
	Entity* player2 = PushAndInitializePlayer(gameState, rectData, rectDataEnd, 0.1, GRAVITY_FLAG);
	player2->centerPosition = { 300, 200 };
	
	Entity* floor = PushAndInitializePlayer(gameState, floorRectData, floorRectDataEnd, 0.1, 0x2);
	floor->centerPosition = { 400, 600 };

	InitWindow(800, 800, "asd");
	SetTargetFPS(60);
	Entity** relevantEntities = (Entity**)PushSize(gameState, sizeof(Entity*) * 20);
	int numOfRelevantEntities = CalculateRelevantEntitiesFor(gameState, player, relevantEntities);
	std::cout << numOfRelevantEntities << " is the number of importants first importants ";

	
	while (!WindowShouldClose()) {
		BeginDrawing();
			ClearBackground(RAYWHITE);
			if (IsKeyDown(KEY_W)) {
				MovePlayer(player, { 0, -1 });
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
				
			if (numOfRelevantEntities == 0) {
				std::cout << "no relevant entities" << std::endl;
			}

			for (int i = 0; i < gameState->addedEntities; i++) {
				numOfRelevantEntities = CalculateRelevantEntitiesFor(gameState, gameState->entities + i, relevantEntities);
				for (int j = 0; j < numOfRelevantEntities; j++) {
					CalculateAndApplyCollisionWithEntity(gameState->entities + i, relevantEntities[j]);
				}
				ApplyGravityAndMovement(gameState, gameState->entities + i);
				DrawEntity((gameState->entities + i));
			}

			for (int i = 0; i < gameState->addedEntities; i++) {

			}
		EndDrawing();
		//throw std::runtime_error("asd");
	}
	RetractSize(gameState, sizeof(Entity*) * 20);
	CloseWindow();
	return 0;
}


