#include <raylib.h>
#include <iostream>
#include "player.h"
#include "GameState.h"
#include <iostream>
#include <thread>
#include <chrono>


int main() {
	void* rawMemory = new uint8_t[150 * 1024];
	memset(rawMemory, 0, 150 * 1024);
	GameState* gameState = (GameState*)rawMemory;
	if (gameState->isInitialized == false) {
		gameState->arena.base = (uint8_t*)rawMemory + sizeof(GameState);
		gameState->arena.used = 0;
		gameState->arena.capacity = 150 * 1024 - sizeof(GameState);
		gameState->entitiesCapacity = 100;
		gameState->goalFps = 165;
		gameState->isInitialized = true;
		gameState->gravityConstant = 98 * 4;
		gameState->nextAvailableId = 1;
		gameState->WINDOW_HEIGHT = 800;
		gameState->WINDOW_WIDTH = 800;
		gameState->gridSquareEdgeLength = 100;
	}
	int numberOfPartitionsOnWidthAxis = gameState->WINDOW_WIDTH / gameState->gridSquareEdgeLength;
	int numberOfPartitionsOnHeightAxis = gameState->WINDOW_HEIGHT / gameState->gridSquareEdgeLength;
	if (gameState->WINDOW_HEIGHT % gameState->gridSquareEdgeLength != 0) { numberOfPartitionsOnHeightAxis++; }
	if (gameState->WINDOW_WIDTH % gameState->gridSquareEdgeLength != 0) { numberOfPartitionsOnWidthAxis++; }
	int numOfIdsPerCell = 50;
	gameState->spatialGrid = (uint32_t*)PushSize(gameState, numberOfPartitionsOnHeightAxis * numberOfPartitionsOnWidthAxis * sizeof(uint32_t) * numOfIdsPerCell);
	gameState->gridDimentions[0] = numberOfPartitionsOnHeightAxis;
	gameState->gridDimentions[1] = numberOfPartitionsOnWidthAxis;
	gameState->gridDimentions[2] = numOfIdsPerCell;
	
	for (int i = 0; i < numberOfPartitionsOnWidthAxis; i++) {
		for (int j = 0; j < numberOfPartitionsOnHeightAxis; j++) {

		}
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
	*(triDataEnd++) = VertexData{ 33.3333f,  33.3333f };
	*(triDataEnd++) = VertexData{ 33.3333f, -66.6667f };
	*(triDataEnd++) = VertexData{ -66.6667f,  33.3333f };

	VertexData* tri2Data = (VertexData*)PushSize(gameState, sizeof(VertexData) * 3);
	VertexData* tri2DataEnd = tri2Data;
	*(tri2DataEnd++) = VertexData{ -33.3333f, -33.3333f };
	*(tri2DataEnd++) = VertexData{ -33.3333f,  66.6667f };
	*(tri2DataEnd++) = VertexData{ 66.6667f, -33.3333f };
	spatial grid gets  filled with the same values for the entire cell at some point

	uint32_t playerFlags = GRAVITY_FLAG | PLAYER_FLAG | GROUND_COLLISION_FLAG;
	Vector2 playerCenterPos = { 400, 301 };
	Entity* player = InitializeAndPushEntity(gameState, tri2Data, tri2DataEnd, 0.1, playerFlags, playerCenterPos);
	Vector2 player2CenterPos = { 300, 200 };
	Entity* player2 = InitializeAndPushEntity(gameState, triData, triDataEnd, 0.1, GRAVITY_FLAG | GROUND_COLLISION_FLAG, player2CenterPos);
	player2->frictionCons = 4.9;
	player2->elasticity = 0.2;
	
	Vector2 floorCenterPos = { 400, 600 };
	Entity* floor = InitializeAndPushEntity(gameState, floorRectData, floorRectDataEnd, 0.2, NON_MOVING_FLAG | GROUND_COLLISION_FLAG, floorCenterPos);
	floor->frictionCons = 0.1;

	InitWindow(gameState->WINDOW_WIDTH, gameState->WINDOW_HEIGHT, "asd");
	SetTargetFPS(gameState->goalFps);
	Entity** relevantEntities = (Entity**)PushSize(gameState, sizeof(Entity*) * 20);
	int numOfRelevantEntities = CalculateRelevantEntitiesFor(gameState, player, relevantEntities, 0);
	std::cout << numOfRelevantEntities << " is the number of importants first importants ";

	while (!WindowShouldClose()) {
		//std::cout << "Frame number " << gameState->frameCount << "is starting" << std::endl << std::endl;
		BeginDrawing();
			ClearBackground(RAYWHITE);
			if (IsKeyPressed(KEY_W)) {
				float jumpForce = (float)(-1000 * GetFPS() / 65);
				if (GetFPS() > 165) {
					jumpForce = (float)(-1000 * 165 / 65);
				}
				ApplyForceToEntity(player, { 0, jumpForce});
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
			if (IsKeyPressed(KEY_E)) {
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
				CalibrateEntityWithGrid(gameState, entity);
				DrawEntity(entity);
				if (entity->flags & PLAYER_FLAG && entity->netForce.y < -1) {
					//std::cout << "Net force on the player: (" << entity->netForce.x << ", " << entity->netForce.y << ") " << std::endl;
					//std::cout << "FPS: " << GetFPS() << std::endl;
					//std::cout << "grid pos of the player(row, column): " << entity->gridRowIdx << ", " << entity->gridColumnIdx << std::endl;
					std::cout << "Num of relevant entities: " << numOfRelevantEntities << std::endl;
				}
				DrawEntityForceLine(entity);
				entity->netForce = { 0, 0 };
			}

		EndDrawing();
		//std::cout << "Frame number " << gameState->frameCount++ << "has ended" << std::endl << std::endl;
		//throw std::runtime_error("asd");
	}
	RetractSize(gameState, sizeof(Entity*) * 20);
	CloseWindow();
	return 0;
}
