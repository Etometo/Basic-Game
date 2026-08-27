#ifndef PLAYER_H
#define PLAYER_H

#include <cstddef>
#include <stdint.h>
#include "raylib.h"
#include "screens.h"

constexpr float INSIGNIFICANT_NORMAL_VELOCITY_THRESHOLD = 0.8;
constexpr float INSIGNIFICANT_ANGULAR_VELOCITY_THRESHOLD = 0.01;

constexpr float INSIGNIFICANT_DISPLACEMENT_THRESHOLD = 20;
constexpr float INSIGNIFICANT_ANGULAR_DISPLACEMENT_THRESHOLD = 0.6;

constexpr float SECONDS_FOR_ENTITY_TO_FREEZE = 1.5;

constexpr int freeFallSpeed = 2;

constexpr int SECONDS_ENTITY_HAS_TO_FREEZE_BEFORE_BEING_RESET = 10;

struct GameState;

typedef struct VertexData {
	Vector2 position;
};

enum {
	GRAVITY_FLAG = 1,
	NON_MOVING_FLAG = 2,
	//change the name
	IN_CONTACT_WITH_GROUND_FLAG = 4,
	GROUND_COLLISION_FLAG = 8,
	HAS_TEXT = 16,
	PHYSICS_FLAG = 32,
	BUTTON_FLAG = 64,
	BEING_CUT_FLAG = 128,
	BEING_CHOSEN_FLAG = 256,
	ADJUSTING_POSITION_FLAG = 512,
	NOT_IN_FREE_FALL_FLAG = 1024,
	IS_A_BUILDING_BLOCK_FLAG = 2048,
	INVISIBLE_FLAG = 4096,
};

enum {
	ENTITY_WAS_CUT = 1,
	ENTITY_WASNT_CUT = 0
};

typedef struct Entity {
	Vector2 temporaryPositionChange;
	Vector2 centerPosition = {-100, -100};
	Color color = {255, 255, 255, 255};
	VertexData* vertexData;
	VertexData* vertexDataEnd;
	unsigned int* triangulationIndices;
	unsigned int* triangulationIndicesEnd;

	uint32_t id;
	uint32_t flags;
	uint8_t screenCode;

	uint64_t frameCount;
	uint16_t averageFPS;

	float elasticity;
	float mass;
	//this is zeroed in the first time it is used.
	Vector2 penetrationResolveForce;
	Vector2 forcesMultipliedByAppliedTime;
	Vector2 netForce;
	Vector2 acceleration;
	//amount the object was moved directly to prevent penetrations
	Vector2 penetrationVelocity;
	Vector2 physicsVelocity;
	Vector2 lastSpeed;
	float linearDamping;
	float frictionCons;

	//inside the screen is positive
	float inertia;
	float torque;
	float rotationalAcceleration;
	float rotationalVelocity;
	float angle;
	float angularDamping;

	uint8_t framesWithConsequentialInsignificantMovement;
	Vector2 positionWhenMovementBecameInsignificant;
	float angleWhenMovementBecameInsignificant;

	//two dimentional, contains pair elements the first one being the row index the second one being the column index
	uint32_t* gridPositionsOfVertices; 
	uint32_t gridPositionOfVerticesSize;

	bool gravityApplied;

	void(*buttonFunction)(GameState* gameState, SCREEN_CODES screenCode);

	char* text;
};

struct CollisionInfo {
	Vector3 normalizedOverlapLine;
	float minOverlap;
};

//Entity* InitializeAndPushEntity(GameState* gameState, VertexData* vertexData, VertexData* vertexDataEnd, float mass, uint32_t flags);

float GetDeltaTime();

void PrintVector(Vector2 vec);

void PrintVector(Vector3 vec);

double DotProduct(Vector2& vec1, Vector2& vec2);

float triangleArea(Vector2 a, Vector2 b, Vector2 c);

float distance(Vector2 v1, Vector2 v2);

float magnitude(Vector2 v);

bool CheckIfAPointIsInsideAnEntity(Vector2 positionOfPoint, Entity* entity);

int MakeAnArrayFullOfUniqueItems(GameState* gameState, char* arrayStart, char* arrayEnd, uint32_t numOfElements, uint32_t typeSize);

Entity* InitializeAndPushEntity(GameState* gameState, VertexData* vertexData, VertexData* vertexDataEnd, float mass, uint32_t flags, Vector2 centerPos, SCREEN_CODES screenCode);

int CutEntityIntoTwoPiecesByALine(GameState* gameState, Entity* entity, Vector2 cutStart, Vector2 cutEnd, uint32_t entityFlags, Entity* &newE1, Entity* &newE2);

void CalibrateEntityWithGrid(GameState* gameState, Entity* e);

void DrawEntity(Entity* player);

void DrawEntityOutline(Entity* entity);

void DrawEntityForceLine(Entity* entity);

void Triangulate2DPoints(VertexData* begin, std::size_t numOfPoints, GameState* gameState, unsigned int** indices, unsigned int** indicesEnd);

bool IsCounterClockwise(Vector2 v1, Vector2 v2, Vector2 v3);

Vector2 AddVectors(Vector2& v1, Vector2& v2);

void MoveAndRotateEntity(Entity* player, float deltaTime);

void ApplyForceToEntity(Entity* player, Vector2 movement);

void ApplyForceToEntitiesVelocityImmediately(Entity* entity, Vector2 force, float deltaTime, Vector2 forceApplicationPoint);

int CalculateRelevantEntitiesForEntity(GameState* gameState, Entity* entity, Entity** relevanEntities, int offset);

int CalculateRelevantEntitiesForPosition(GameState* gameState, Vector2 position, Entity** relevanEntities);

CollisionInfo DetectCollisionWithEntity(Entity* e1, Entity* e2);

Vector2 CalculateForceApplicationPoint(Entity* e1, Entity* e2, float vertexOutsidePush, CollisionInfo collInfo);

void CalculateAndApplyImpulse(GameState* gameState, Entity* e1, Entity* e2, CollisionInfo collInfo, Vector2& impulse, Vector2& relativeVelocity, Vector2 forceApplicationPoint, float deltaTime);

void HandleFriction(GameState* gameState, Entity* e1, Entity* e2, CollisionInfo collInfo, Vector2 &impulse, Vector2 &relativeVel, Vector2 forceApplicationPoint, float deltaTime);

void ApplyGravityCalculatePhysicsAndMoveEntity(GameState* gameState, Entity* entity);

unsigned int CheckHowManyVerticesOfE1IsInE2(Entity* e1, Entity* e2, Vector2& sumOfInsiderVerticesPositions, CollisionInfo collInfo);

void ApplyFrictionToEntity(Entity* e, Vector3 normalizedFrictionAxis, float frictionMagnitude, int frictionDirection, float deltaTime, Vector2 &relativeVelocity, Vector2 forceApplicationPoint);

#endif