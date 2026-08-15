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
		gameState->arena.capacity = 150 * 1024 * 1024 - sizeof(GameState);
		gameState->arena.temporaryCapacity = 100 * 1024;
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
	
	InitWindow(gameState->WINDOW_WIDTH, gameState->WINDOW_HEIGHT, "asd");
	SetTargetFPS(gameState->goalFps);

	gameState->currentScreenCode = MAIN_SCREEN;

	InitializeMainMenu(gameState);
	InitializeGameplayScreen(gameState);
	gameState->gameplayScreenInitialized = true;

	bool mouseLeftBeingHeld = false;
	uint64_t mouseLeftHoldFramesCount = 0;
	Vector2 cuttingLineStartPosition = { 0, 0 }, cuttingLineEndPosition = { 0, 0 };
	MouseInputInfo mouseInputInfo = { NONE, false, 0, {{0, 0}, {0, 0}}};

	while (!WindowShouldClose()) {
		//std::cout << "Frame number " << gameState->frameCount << "is starting" << std::endl << std::endl;
		BeginDrawing();
			ClearBackground(RAYWHITE);

			if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
				if(mouseInputInfo.inputType == NONE){
					mouseInputInfo.inputType = LEFT_CLICK;
					mouseInputInfo.inputPositions[0] = GetMousePosition();
				}
				mouseInputInfo.inputPositions[1] = GetMousePosition();
				mouseInputInfo.inputDurationFrames++;
			}
			else {
				if (!mouseInputInfo.mouseReleasedThisFrame && mouseInputInfo.inputType == LEFT_CLICK) {
					mouseInputInfo.mouseReleasedThisFrame = true;
					mouseInputInfo.inputPositions[1] = GetMousePosition();
				}
				else {
					mouseInputInfo.inputType = NONE;
					mouseInputInfo.inputDurationFrames = 0;
					mouseInputInfo.mouseReleasedThisFrame = false;
				}
			}

			switch (gameState->currentScreenCode) {
				case MAIN_SCREEN:
					UpdateMainMenu(gameState, mouseInputInfo);
				case GAMEPLAY_SCREEN:
					UpdateGameplayScreen(gameState, mouseInputInfo);
			}

			for (int i = 0; i < gameState->lastEntityOnEntities + 1 - gameState->entities; i++) {
				Entity* entity = gameState->entities + i;
				if (entity->screenCode == gameState->currentScreenCode) {
					DrawEntity(entity);
				}
			}

		EndDrawing();
	}
	RetractSize(gameState, sizeof(Entity*) * 20);
	CloseWindow();
	return 0;
}
