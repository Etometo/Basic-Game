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
	Vector2 forceAppliedToAccelerationAndVelocity;
	Vector2 forcesMultipliedByAppliedTime;
	Vector2 netForce = { 0, 0 };
	Vector2 acceleration = { 0, 0 };
	//amount the object was moved directly to prevent penetrations
	Vector2 penetrationVelocity;
	Vector2 physicsVelocity = {0, 0};
	Vector2 lastSpeed;
	float frictionCons = 0;

	//inside the screen is positive
	float inertia;
	float torque;
	float rotationalAcceleration;
	float rotationalVelocity;


	//two dimentional, contains pair elements the first one being the row index the second one being the column index
	uint32_t* gridPositionsOfVertices; 
	uint32_t gridPositionOfVerticesSize;

	bool isPlayer = false;
	bool gravityApplied;
	bool stoppedByFriction;
};

struct CollisionInfo {
	Vector3 normalizedOverlapLine;
	float minOverlap;
};

//Entity* InitializeAndPushEntity(GameState* gameState, VertexData* vertexData, VertexData* vertexDataEnd, float mass, uint32_t flags);

float GetDeltaTime();

void PrintVector(Vector2 vec);

void PrintVector(Vector3 vec);

Entity* InitializeAndPushEntity(GameState* gameState, VertexData* vertexData, VertexData* vertexDataEnd, float mass, uint32_t flags, Vector2 centerPos);

void CalibrateEntityWithGrid(GameState* gameState, Entity* e);

void DrawEntity(Entity* player);

void DrawEntityForceLine(Entity* entity);

void Triangulate2DPoints(VertexData* begin, std::size_t numOfPoints, GameState* gameState, unsigned int** indices, unsigned int** indicesEnd);

bool IsCounterClockwise(Vector2 v1, Vector2 v2, Vector2 v3);

Vector2 AddVectors(Vector2& v1, Vector2& v2);

void MoveEntity(Entity* player, float deltaTime);

void ApplyForceToEntity(Entity* player, Vector2 movement);

void ApplyForceToEntitiesVelocityImmediately(Entity* entity, Vector2 force, float deltaTime, Vector2 forceApplicationPoint);

int CalculateRelevantEntitiesFor(GameState* gameState, Entity* entity, Entity** relevanEntities, int offset);

CollisionInfo DetectCollisionWithEntity(Entity* e1, Entity* e2);

Vector2 CalculateForceApplicationPoint(Entity* e1, Entity* e2);

float ResolvePenetrationAndReturnThePush(Entity* e1, Entity* e2, CollisionInfo collInfo, float sumOfInverseMasses, Vector2 &impulse);

void CalculateAndApplyImpulse(GameState* gameState, Entity* e1, Entity* e2, CollisionInfo collInfo, Vector2& impulse, Vector2& relativeVelocity, Vector2 forceApplicationPoint, float deltaTime);

void HandleFriction(GameState* gameState, Entity* e1, Entity* e2, CollisionInfo collInfo, Vector2 &impulse, Vector2 &relativeVel, Vector2 forceApplicationPoint, float deltaTime);

void ApplyGravityCalculatePhysicsAndMoveEntity(GameState* gameState, Entity* entity);

unsigned int CheckHowManyVerticesOfE1IsInE2(Entity* e1, Entity* e2, Vector2& sumOfInsiderVerticesPositions);

void ApplyFrictionToEntity(Entity* e, Vector3 normalizedFrictionAxis, float frictionMagnitude, int frictionDirection, float deltaTime, Vector2 &relativeVelocity, Vector2 forceApplicationPoint);

#endif