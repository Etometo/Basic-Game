#include "GameState.h"
#include <iostream>
#include <cassert>

void* PushSize(GameState* state, size_t sizeInBytes) {
	assert((char*)state->arena.usableCapacity - (char*)state->arena.used > sizeInBytes);

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
		for (int i = 0; i < state->entitiesCapacity; i++) {
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
				std::cout << " ";
				break;
			}
		}
	}
}

void* PushTemporarySize(GameState* state, uint32_t sizeInBytes) {
	assert(state->arena.capacity - state->arena.usableCapacity - state->arena.usedTemporary > sizeInBytes);
	state->arena.usedTemporary += sizeInBytes;
	return state->arena.base + state->arena.capacity - state->arena.usedTemporary;
}

void RetractTemporarySize(GameState* state, uint32_t sizeInBytes) {
	state->arena.usedTemporary -= sizeInBytes;
}
