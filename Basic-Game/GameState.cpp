#include "GameState.h"
#include <iostream>
#include <cassert>

void* PushSize(GameState* state, size_t sizeInBytes) {
	assert((char*)state->arena.usableCapacity - (char*)state->arena.used > sizeInBytes);

	state->arena.used += sizeInBytes;
	return state->arena.base + state->arena.used - sizeInBytes;
} 

Entity* PushEntity(GameState* state) {
	assert(state->addedEntities < state->entitiesCapacity);
	return (state->entities + state->addedEntities++);
}

void RetractSize(GameState* state, size_t sizeInBytes) {
	state->arena.used -= sizeInBytes;
}

void* PushTemporarySize(GameState* state, uint32_t sizeInBytes) {
	assert(state->arena.capacity - state->arena.usableCapacity - state->arena.usedTemporary > sizeInBytes);
	state->arena.usedTemporary += sizeInBytes;
	return state->arena.base + state->arena.capacity - state->arena.usedTemporary;
}

void RetractTemporarySize(GameState* state, uint32_t sizeInBytes) {
	state->arena.usedTemporary -= sizeInBytes;
}
