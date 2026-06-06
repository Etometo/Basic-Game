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
};

void* PushSize(GameState* state, size_t sizeInBytes);

void RetractSize(GameState* state, size_t sizeInBytes);

Entity* PushEntity(GameState* state);

