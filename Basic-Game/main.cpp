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
		gameState->arena.temporaryCapacity = 100 * 1024;
		gameState->arena.capacity = 150 * 1024 * 1024 - gameState->arena.temporaryCapacity - sizeof(GameState);
		gameState->entitiesCapacity = 2000;
		gameState->nextEmptyPlaceForEntity = gameState->entities;
		gameState->lastEntityOnEntities = gameState->entities;

		gameState->goalFps = 120;
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

	gameState->isInitialized = true;

	SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
	InitWindow(gameState->WINDOW_WIDTH, gameState->WINDOW_HEIGHT, "asd");
	SetTargetFPS(gameState->goalFps);

	gameState->currentScreenCode = MAIN_SCREEN;

	InitializeMainMenu(gameState);
	InitializeGameplayScreen(gameState);
	gameState->gameplayScreenInitialized = true;

	MouseInputInfo mouseInputInfo = { NONE, false, 0, {{0, 0}, {0, 0}}};
	InputInfo inputInfo = { mouseInputInfo, 0 };
	while (!WindowShouldClose()) {
		//std::cout << "Frame number " << gameState->frameCount << "is starting" << std::endl << std::endl;
		BeginDrawing();
			ClearBackground(RAYWHITE);

			if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
				if(inputInfo.mouseInputInfo.inputType == NONE){
					inputInfo.mouseInputInfo.inputType = LEFT_CLICK;
					inputInfo.mouseInputInfo.inputPositions[0] = GetMousePosition();
				}
				inputInfo.mouseInputInfo.inputPositions[1] = GetMousePosition();
				inputInfo.mouseInputInfo.inputDurationFrames++;
			}
			else {
				if (!inputInfo.mouseInputInfo.mouseReleasedThisFrame && inputInfo.mouseInputInfo.inputType == LEFT_CLICK) {
					inputInfo.mouseInputInfo.mouseReleasedThisFrame = true;
					inputInfo.mouseInputInfo.inputPositions[1] = GetMousePosition();
				}
				else {
					inputInfo.mouseInputInfo.inputType = NONE;
					inputInfo.mouseInputInfo.inputDurationFrames = 0;
					inputInfo.mouseInputInfo.mouseReleasedThisFrame = false;
				}
			}
			//I made new key codes because i wanna use them like flags so I can store multiple key inputs in one int
			if (IsKeyDown(KEY_A)) {
				inputInfo.keyCodes |= A_KEY_CODE;
			}
			if (IsKeyDown(KEY_D)) {
				inputInfo.keyCodes |= D_KEY_CODE;
			}
			if (IsKeyDown(KEY_ENTER)) {
				inputInfo.keyCodes |= ENTER_KEY_CODE;
			}

			switch (gameState->currentScreenCode) {
				case MAIN_SCREEN:
					UpdateMainMenu(gameState, inputInfo);
				case GAMEPLAY_SCREEN:
					UpdateGameplayScreen(gameState, inputInfo);
			}

			for (int i = 0; i < gameState->lastEntityOnEntities + 1 - gameState->entities; i++) {
				Entity* entity = gameState->entities + i;
				if (entity->id != 0 && entity->screenCode == gameState->currentScreenCode) {
					DrawEntity(entity);
				}
			}

		EndDrawing();
		inputInfo.keyCodes = 0;
	}
	RetractSize(gameState, sizeof(Entity*) * 20);
	CloseWindow();
	return 0;
}
