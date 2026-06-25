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
	GRAVITY_FLAG = 1,
	NON_MOVING_FLAG = 2,
	PLAYER_FLAG = 4,
	GROUND_COLLISION_FLAG = 8,
	ENTITY_COLLISION_FLAG = 16,
	COLLISION_FLAGS = GROUND_COLLISION_FLAG + ENTITY_COLLISION_FLAG,
};

typedef struct Entity {
	bool isPlayer = false;
	Vector2 centerPosition = {-100, -100};
	Color color = {255, 255, 255, 255};
	VertexData* vertexData;
	VertexData* vertexDataEnd;
	unsigned int* triangulationIndices = nullptr;
	unsigned int* triangulationIndicesEnd = nullptr;

	uint32_t id;
	uint32_t flags = 0;

	float elasticity = 0;
	float mass = 0;
	Vector2 netForce = { 0, 0 };
	Vector2 acceleration = { 0, 0 };
	Vector2 speed = {0, 0};
	Vector2 lastSpeed;
	float frictionCons = 0;

	uint32_t gridRowIdx, gridColumnIdx;
	
};

//Entity* InitializeAndPushEntity(GameState* gameState, VertexData* vertexData, VertexData* vertexDataEnd, float mass, uint32_t flags);

Entity* InitializeAndPushEntity(GameState* gameState, VertexData* vertexData, VertexData* vertexDataEnd, float mass, uint32_t flags, Vector2 centerPos);

void CalibrateEntityWithGrid(GameState* gameState, Entity* e);

void DrawEntity(Entity* player);

void DrawEntityForceLine(Entity* entity);

void Triangulate2DPoints(VertexData* begin, std::size_t numOfPoints, GameState* gameState, unsigned int** indices, unsigned int** indicesEnd);

bool IsCounterClockwise(Vector2 v1, Vector2 v2, Vector2 v3);

Vector2 AddVectors(Vector2& v1, Vector2& v2);

void MoveEntity(Entity* player);

void ApplyForceToEntity(Entity* player, Vector2 movement);

int CalculateRelevantEntitiesFor(GameState* gameState, Entity* entity, Entity** relevanEntities, int offset);

void CalculateAndApplyCollisionWithEntity(Entity* e1, Entity* e2);

void ApplyGravityCalculatePhysicsAndMoveEntity(GameState* gameState, Entity* entity);

unsigned int CheckHowManyVerticesOfE1IsInE2(Entity* e1, Entity* e2, Vector2& sumOfInsiderVerticesPositions);

void ApplyFrictionToEntity(Entity* e, Vector3 normalizedFrictionAxis, float frictionMagnitude, int frictionDirection);

#endif