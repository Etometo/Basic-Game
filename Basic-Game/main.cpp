#include <raylib.h>
#include <iostream>
#include "player.h"
#include "GameState.h"
#include "screens.h"
#include <thread>
#include <chrono>


int main() {
	void* rawMemory = new uint8_t[150 * 1024 * 1024];
	memset(rawMemory, 0, 150 * 1024 * 1024);
	GameState* gameState = (GameState*)rawMemory;
	if (gameState->isInitialized == false) {
		gameState->arena.base = (uint8_t*)rawMemory + sizeof(GameState);
		gameState->arena.used = 0;
		gameState->arena.usedTemporary = 0;
		gameState->arena.capacity = 150 * 1024 - sizeof(GameState);
		gameState->arena.temporaryCapacity = 10 * 1024;
		gameState->arena.usableCapacity = gameState->arena.capacity - gameState->arena.temporaryCapacity;
		gameState->entitiesCapacity = 2000;
		gameState->nextEmptyPlaceForEntity = gameState->entities;
		gameState->lastEntityOnEntities = gameState->entities;

		gameState->goalFps = 60;
		gameState->isInitialized = true;
		gameState->gravityConstant = 98 * 4;
		gameState->EPSILON = 1e-5f;
		gameState->ClickThresholdFrames = gameState->goalFps / 3;
		gameState->nextAvailableId = 1;

		gameState->WINDOW_HEIGHT = 800;
		gameState->WINDOW_WIDTH = 800;
		gameState->gridSquareEdgeLength = 100;
		gameState->SOLVER_ITERATIONS = 5;

		gameState->entityBeingCut = nullptr;
		gameState->cutPiece1 = nullptr;
		gameState->cutPiece2 = nullptr;
		gameState->readyForNewEntityInitialization = true;
		gameState->entityInitialized = false;
	}
	//initialize the spatial grid
	int numberOfPartitionsOnWidthAxis = gameState->WINDOW_WIDTH / gameState->gridSquareEdgeLength;
	int numberOfPartitionsOnHeightAxis = gameState->WINDOW_HEIGHT / gameState->gridSquareEdgeLength;
	if (gameState->WINDOW_HEIGHT % gameState->gridSquareEdgeLength != 0) { numberOfPartitionsOnHeightAxis++; }
	if (gameState->WINDOW_WIDTH % gameState->gridSquareEdgeLength != 0) { numberOfPartitionsOnWidthAxis++; }
	int numOfIdsPerCell = 50;
	gameState->spatialGrid = (uint32_t*)PushSize(gameState, numberOfPartitionsOnHeightAxis * numberOfPartitionsOnWidthAxis * sizeof(uint32_t) * numOfIdsPerCell);
	gameState->gridDimentions[0] = numberOfPartitionsOnHeightAxis;
	gameState->gridDimentions[1] = numberOfPartitionsOnWidthAxis;
	gameState->gridDimentions[2] = numOfIdsPerCell;
	
	VertexData* rectData = (VertexData*)PushSize(gameState, sizeof(VertexData) * 4);
	VertexData* rectDataEnd = rectData;
	*(rectDataEnd++) = VertexData{ 50, 50 };
	*(rectDataEnd++) = VertexData{ 50, -50 };
	*(rectDataEnd++) = VertexData{ -50, -50 };
	*(rectDataEnd++) = VertexData{ -50, 50 };
	gameState->rectData = rectData;
	gameState->rectDataEnd = rectDataEnd;

	VertexData* floorRectData = (VertexData*)PushSize(gameState, sizeof(VertexData) * 6);
	VertexData* floorRectDataEnd = floorRectData;
	*(floorRectDataEnd++) = VertexData{ 399, 50 };
	*(floorRectDataEnd++) = VertexData{ 399, -50 };
	*(floorRectDataEnd++) = VertexData{ 0, -50 };
	*(floorRectDataEnd++) = VertexData{ -399, -50 };
	*(floorRectDataEnd++) = VertexData{ -399, 50 };
	*(floorRectDataEnd++) = VertexData{ 0, 50 };

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

	//spatial grid gets  filled with the same values for the entire cell at some point
	uint32_t playerFlags = GRAVITY_FLAG | PLAYER_FLAG | GROUND_COLLISION_FLAG | PHYSICS_FLAG;
	
	Vector2 floorCenterPos = { 400, 750 };
	Entity* floor = InitializeAndPushEntity(gameState, floorRectData, floorRectDataEnd, 0.2, NON_MOVING_FLAG | GROUND_COLLISION_FLAG | PHYSICS_FLAG, floorCenterPos);
	floor->frictionCons = 0.1;

	//throw std::runtime_error("do the rotation stuff and fix the very little impulses preventing objects from staying on top of each other");
	InitWindow(gameState->WINDOW_WIDTH, gameState->WINDOW_HEIGHT, "asd");
	SetTargetFPS(gameState->goalFps);

	gameState->currentScreenCode = GAMEPLAY_SCREEN;

	bool inputGiven = false;
	Vector2 inputForce = { 0, 0 };
	MouseInputInfo mouseInputInfo = { NONE, {{0, 0}, {0, 0}}};

	while (!WindowShouldClose()) {
		//std::cout << "Frame number " << gameState->frameCount << "is starting" << std::endl << std::endl;
		BeginDrawing();
			ClearBackground(RAYWHITE);

			switch (gameState->currentScreenCode) {
			case GAMEPLAY_SCREEN:
				UpdateGameplayScreen(gameState, mouseInputInfo);
			}

			inputGiven = false;
			inputForce = { 0, 0 };
		EndDrawing();
		//std::cout << "Frame number " << gameState->frameCount++ << "has ended" << std::endl << std::endl;
	}
	RetractSize(gameState, sizeof(Entity*) * 20);
	CloseWindow();
	return 0;
}
