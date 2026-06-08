#ifndef PLAYER_H
#define PLAYER_H

#include <cstddef>
#include <stdint.h>
#include "raylib.h"

struct GameState;

typedef struct VertexData {
	Vector2 position;
};

enum {
	GRAVITY_FLAG = 0x1,
	NON_MOVING_FLAG = 0x2,
	PLAYER_FLAG = 0x4,
};

typedef struct Entity {
	bool isPlayer = false;
	Vector2 centerPosition = {-100, -100};
	Color color = {255, 255, 255, 255};
	VertexData* vertexData;
	VertexData* vertexDataEnd;
	unsigned int* triangulationIndices = nullptr;
	unsigned int* triangulationIndicesEnd = nullptr;

	uint32_t flags = 0;
	Vector2 netForce = { 0, 0 };
	Vector2 acceleration = { 0, 0 };
	Vector2 speed = {0, 0};
	float mass = 0;
	float frictionCons = 0;
};

// Initializes the player memory using the custom arena/push allocator
Entity* PushAndInitializePlayer(GameState* gameState, VertexData* vertexData, VertexData* vertexDataEnd, float mass, uint32_t flags);

// Renders the player based on the triangulated mesh
void DrawEntity(Entity* player);

// Triangulates a set of 2D points (exposed if you need to use it outside of player initialization)
void Triangulate2DPoints(VertexData* begin, std::size_t numOfPoints, GameState* gameState, unsigned int** indices, unsigned int** indicesEnd);

bool IsCounterClockwise(Vector2 v1, Vector2 v2, Vector2 v3);

Vector2 AddVectors(Vector2& v1, Vector2& v2);

void MoveEntity(Entity* player);

void ApplyForceToEntity(Entity* player, Vector2 movement);

int CalculateRelevantEntitiesFor(GameState* gameState, Entity* entity, Entity** relevanEntities, int offset);

void CalculateAndApplyCollisionWithEntity(Entity* e1, Entity* e2);

void ApplyGravityCalculatePhysicsAndMoveEntity(GameState* gameState, Entity* entity);

#endif