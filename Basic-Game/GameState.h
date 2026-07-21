#pragma once
#include <stdint.h>
#include "player.h"

typedef struct MemoryArena {
	size_t used;
	size_t capacity;
	uint8_t* base;
};
typedef struct GameState {
	bool isInitialized = false;
	MemoryArena arena;
	unsigned int goalFps = 60;
	Entity entities[100];
	unsigned int addedEntities = 0;
	unsigned int entitiesCapacity = 100;
	float gravityConstant = 0.098;
	float EPSILON;
	int SOLVER_ITERATIONS;
	uint32_t nextAvailableId = 1;

	uint64_t frameCount = 0;

	uint32_t WINDOW_HEIGHT, WINDOW_WIDTH;
	uint32_t gridSquareEdgeLength;
	//grid is a four dimentional array, outest dimention is the row, second outest is the column and inside a cell there is an array
	//capable of holding 10 uint32_t values(ids)
	uint32_t* spatialGrid;
	uint32_t gridDimentions[3];
};

void* PushSize(GameState* state, size_t sizeInBytes);

void RetractSize(GameState* state, size_t sizeInBytes);

Entity* PushEntity(GameState* state);

