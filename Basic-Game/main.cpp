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
		gameState->gravityConstant = 98;
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
	player->centerPosition = { 300, 70 };
	Entity* player2 = PushAndInitializePlayer(gameState, rectData, rectDataEnd, 0.1, GRAVITY_FLAG);
	player2->centerPosition = { 300, 200 };
	
	Entity* floor = PushAndInitializePlayer(gameState, floorRectData, floorRectDataEnd, 0.2, 0x2);
	floor->centerPosition = { 400, 600 };
	floor->frictionCons = 1;

	InitWindow(800, 800, "asd");
	SetTargetFPS(120);
	Entity** relevantEntities = (Entity**)PushSize(gameState, sizeof(Entity*) * 20);
	int numOfRelevantEntities = CalculateRelevantEntitiesFor(gameState, player, relevantEntities, 0);
	std::cout << numOfRelevantEntities << " is the number of importants first importants ";

	
	while (!WindowShouldClose()) {
		BeginDrawing();
			ClearBackground(RAYWHITE);
			if (IsKeyPressed(KEY_W)) {
				ApplyForceToEntity(player, { 0, -1000 });
			}
			if (IsKeyDown(KEY_S)) {
				ApplyForceToEntity(player, { 0, 20 });
			}
			if (IsKeyDown(KEY_A)) {
				ApplyForceToEntity(player, { -20, 0 });
			}
			if (IsKeyDown(KEY_D)) {
				ApplyForceToEntity(player, { 20, 0 });
			}

			for (int i = 0; i < gameState->addedEntities; i++) {
				Entity* entity = gameState->entities + i;
				numOfRelevantEntities = CalculateRelevantEntitiesFor(gameState, entity, relevantEntities, i);

				if((entity->flags & GRAVITY_FLAG) > 0){
					entity->netForce.y += gameState->gravityConstant * entity->mass;
				}


				for (int j = 0; j < numOfRelevantEntities; j++) {
					CalculateAndApplyCollisionWithEntity(entity, relevantEntities[j]);
				}

				entity->acceleration.x = entity->netForce.x / entity->mass;
				entity->acceleration.y = entity->netForce.y / entity->mass;

				float deltaTime = GetFrameTime();
				entity->speed.x += entity->acceleration.x * deltaTime;
				entity->speed.y += entity->acceleration.y * deltaTime;

				MoveEntity(entity);
				entity->netForce = { 0, 0 };
				DrawEntity(entity);
			}

		EndDrawing();
		//throw std::runtime_error("asd");
	}
	RetractSize(gameState, sizeof(Entity*) * 20);
	CloseWindow();
	return 0;
}


