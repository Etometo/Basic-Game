#include "GameState.h"
#include <iostream>
#include <cassert>

void InitializeGameState(GameState* gameState) {

	gameState->arena.base = (uint8_t*)gameState + sizeof(GameState);
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

	gameState->freeTimeLimitInSeconds = 5;
	//there is no free time at the start
	gameState->freeTimeFramesCounter = gameState->freeTimeLimitInSeconds * gameState->goalFps * 2;

	gameState->newEntitySpawnPoint = { 400, 100 };
	gameState->entityBeingCut = nullptr;
	gameState->cutPiece1 = nullptr;
	gameState->cutPiece2 = nullptr;
	gameState->readyForNewEntityInitialization = true;
	gameState->entityInitialized = false;

	gameState->limitOfSpawnedEntities = 3;

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
}

void* PushSize(GameState* state, size_t sizeInBytes) {
	assert((char*)state->arena.capacity - (char*)state->arena.used > sizeInBytes);

	state->arena.used += sizeInBytes;
	return state->arena.base + state->arena.used - sizeInBytes;
} 

Entity* PushEntity(GameState* state) {
	//give preallocated memory space to entity and give it an id
	assert(state->addedEntities < state->entitiesCapacity);
	Entity* placeToBeGiven = state->nextEmptyPlaceForEntity;
	if ((placeToBeGiven + 1)->id == 0) {
		state->nextEmptyPlaceForEntity++;
	}
	else {
		for (int i = placeToBeGiven - state->entities + 1; i < state->entitiesCapacity; i++) {
			if ((state->entities + i)->id == 0) {
				state->nextEmptyPlaceForEntity = (state->entities + i);
				break;
			}
		}
	}

	if ((placeToBeGiven - state->lastEntityOnEntities) > 0) {
		state->lastEntityOnEntities = placeToBeGiven;
	}
	state->addedEntities++;
	placeToBeGiven->id = (placeToBeGiven - state->entities) + 1;
	return placeToBeGiven;
}

void RetractSize(GameState* state, size_t sizeInBytes) {
	state->arena.used -= sizeInBytes;
}

void DeleteEntity(GameState* state, Entity* entity) {
	std::memset((void*)entity, 0, sizeof(Entity));
	state->addedEntities -= 1;
	if ((state->nextEmptyPlaceForEntity - entity) > 0) {
		state->nextEmptyPlaceForEntity = entity;
	}
	if (entity == state->lastEntityOnEntities) {
		for (int i = 0; i < state->entitiesCapacity; i++) {
			Entity* entity = state->entities + (state->entitiesCapacity - 1 - i);
			if (entity->id != 0) {
				state->lastEntityOnEntities = entity;
				break;
			}
		}
	}
}

void* PushTemporarySize(GameState* state, uint32_t sizeInBytes) {
	assert(state->arena.temporaryCapacity - state->arena.usedTemporary > sizeInBytes);
	state->arena.usedTemporary += sizeInBytes;
	return state->arena.base + state->arena.capacity - state->arena.usedTemporary;
}

void RetractTemporarySize(GameState* state, uint32_t sizeInBytes) {
	state->arena.usedTemporary -= sizeInBytes;
}
