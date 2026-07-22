#include <raylib.h>
#include <iostream>
#include "player.h"
#include "GameState.h"
#include <iostream>
#include <thread>
#include <chrono>


int main() {
	void* rawMemory = new uint8_t[150 * 1024];
	memset(rawMemory, 0, 150 * 1024);
	GameState* gameState = (GameState*)rawMemory;
	if (gameState->isInitialized == false) {
		gameState->arena.base = (uint8_t*)rawMemory + sizeof(GameState);
		gameState->arena.used = 0;
		gameState->arena.capacity = 150 * 1024 - sizeof(GameState);
		gameState->entitiesCapacity = 100;
		gameState->goalFps = 60;
		gameState->isInitialized = true;
		gameState->gravityConstant = 98 * 4;
		gameState->EPSILON = 1e-5f;
		gameState->nextAvailableId = 1;
		gameState->WINDOW_HEIGHT = 800;
		gameState->WINDOW_WIDTH = 800;
		gameState->gridSquareEdgeLength = 100;
		gameState->SOLVER_ITERATIONS = 5;
	}
	int numberOfPartitionsOnWidthAxis = gameState->WINDOW_WIDTH / gameState->gridSquareEdgeLength;
	int numberOfPartitionsOnHeightAxis = gameState->WINDOW_HEIGHT / gameState->gridSquareEdgeLength;
	if (gameState->WINDOW_HEIGHT % gameState->gridSquareEdgeLength != 0) { numberOfPartitionsOnHeightAxis++; }
	if (gameState->WINDOW_WIDTH % gameState->gridSquareEdgeLength != 0) { numberOfPartitionsOnWidthAxis++; }
	int numOfIdsPerCell = 50;
	gameState->spatialGrid = (uint32_t*)PushSize(gameState, numberOfPartitionsOnHeightAxis * numberOfPartitionsOnWidthAxis * sizeof(uint32_t) * numOfIdsPerCell);
	gameState->gridDimentions[0] = numberOfPartitionsOnHeightAxis;
	gameState->gridDimentions[1] = numberOfPartitionsOnWidthAxis;
	gameState->gridDimentions[2] = numOfIdsPerCell;
	
	for (int i = 0; i < numberOfPartitionsOnWidthAxis; i++) {
		for (int j = 0; j < numberOfPartitionsOnHeightAxis; j++) {

		}
	}

	VertexData* rectData = (VertexData*)PushSize(gameState, sizeof(VertexData) * 4);
	VertexData* rectDataEnd = rectData;
	*(rectDataEnd++) = VertexData{ 50, 50 };
	*(rectDataEnd++) = VertexData{ 50, -50 };
	*(rectDataEnd++) = VertexData{ -50, -50 };
	*(rectDataEnd++) = VertexData{ -50, 50 };

	VertexData* floorRectData = (VertexData*)PushSize(gameState, sizeof(VertexData) * 4);
	VertexData* floorRectDataEnd = floorRectData;
	*(floorRectDataEnd++) = VertexData{ 150, 50 };
	*(floorRectDataEnd++) = VertexData{ 150, -50 };
	*(floorRectDataEnd++) = VertexData{ -150, -50 };
	*(floorRectDataEnd++) = VertexData{ -150, 50 };

	VertexData* triData = (VertexData*)PushSize(gameState, sizeof(VertexData) * 3);
	VertexData* triDataEnd = triData;
	*(triDataEnd++) = VertexData{ 33.3333f,  33.3333f };
	*(triDataEnd++) = VertexData{ 33.3333f, -66.6667f };
	*(triDataEnd++) = VertexData{ -66.6667f,  33.3333f };

	VertexData* tri2Data = (VertexData*)PushSize(gameState, sizeof(VertexData) * 3);
	VertexData* tri2DataEnd = tri2Data;
	*(tri2DataEnd++) = VertexData{ -33.3333f, -33.3333f };
	*(tri2DataEnd++) = VertexData{ -33.3333f,  66.6667f };
	*(tri2DataEnd++) = VertexData{ 66.6667f, -33.3333f };
	//spatial grid gets  filled with the same values for the entire cell at some point

	uint32_t playerFlags = GRAVITY_FLAG | PLAYER_FLAG | GROUND_COLLISION_FLAG;
	Vector2 playerCenterPos = { 400, 301 };
	Entity* player = InitializeAndPushEntity(gameState, tri2Data, tri2DataEnd, 0.1, playerFlags, playerCenterPos);
	player->isPlayer = true;
	Vector2 player2CenterPos = { 500, 100 };
	Entity* player2 = InitializeAndPushEntity(gameState, triData, triDataEnd, 0.1, GRAVITY_FLAG | GROUND_COLLISION_FLAG, player2CenterPos);
	player2->frictionCons = 200.9;
	player2->elasticity = 0;
	
	Vector2 floorCenterPos = { 400, 600 };
	Entity* floor = InitializeAndPushEntity(gameState, floorRectData, floorRectDataEnd, 0.2, NON_MOVING_FLAG | GROUND_COLLISION_FLAG, floorCenterPos);
	floor->frictionCons = 0.1;

	//throw std::runtime_error("do the rotation stuff and fix the very little impulses preventing objects from staying on top of each other");
	InitWindow(gameState->WINDOW_WIDTH, gameState->WINDOW_HEIGHT, "asd");
	SetTargetFPS(gameState->goalFps);
	Entity** relevantEntities = (Entity**)PushSize(gameState, sizeof(Entity*) * 20);
	int numOfRelevantEntities = CalculateRelevantEntitiesFor(gameState, player, relevantEntities, 0);
	std::cout << numOfRelevantEntities << " is the number of importants first importants ";

	bool inputGiven = false;
	Vector2 inputForce = { 0, 0 };

	while (!WindowShouldClose()) {
		std::cout << "Frame number " << gameState->frameCount << "is starting" << std::endl << std::endl;
		BeginDrawing();
			ClearBackground(RAYWHITE);
			if (IsKeyPressed(KEY_W)) {
				float jumpForce = (float)(-1000 * GetFPS() / 65);
				if (GetFPS() > 165) {
					jumpForce = (float)(-1000 * 165 / 65);
				}
				inputGiven = true;
				inputForce.y += jumpForce;
			}
			if (IsKeyDown(KEY_S)) {
				inputGiven = true;
				inputForce.y += 20;
			}
			if (IsKeyDown(KEY_A)) {
				inputGiven = true;
				inputForce.x += -20;
			}
			if (IsKeyDown(KEY_D)) {
				inputGiven = true;
				inputForce.x += 20;
			}
			if (IsKeyPressed(KEY_E)) {
			}

			for (int i = 0; i < gameState->addedEntities; i++) {
				Entity* entity = gameState->entities + i;
				numOfRelevantEntities = CalculateRelevantEntitiesFor(gameState, entity, relevantEntities, i);

				float deltaTime = GetDeltaTime();

				for(int k = 0; k < gameState->SOLVER_ITERATIONS; k++){
					float iterationTimeStep = deltaTime / gameState->SOLVER_ITERATIONS;
					if ((entity->flags & GRAVITY_FLAG) > 0) {
						ApplyForceToEntitiesVelocityImmediately(entity, { 0, entity->mass * gameState->gravityConstant }, iterationTimeStep);
						entity->gravityApplied = true;
					}
					if (entity->isPlayer && inputGiven) {
						ApplyForceToEntitiesVelocityImmediately(entity, inputForce, iterationTimeStep);
					}

					for (int j = 0; j < numOfRelevantEntities; j++) {
						Entity* relevantEntity = relevantEntities[j];
						CollisionInfo collInfo = DetectCollisionWithEntity(entity, relevantEntity);
						float totalMass = entity->mass + relevantEntity->mass;
						if (collInfo.minOverlap > 0) {
							Vector2 impulse = { 0, 0 }, relativeVel = {0, 0};

							CalculateAndApplyImpulse(gameState, entity, relevantEntity, collInfo, impulse, relativeVel, iterationTimeStep);

							HandleFriction(gameState, entity, relevantEntity, collInfo, impulse, relativeVel, iterationTimeStep);
						}
					}

					if ((entity->flags & NON_MOVING_FLAG) == 0) {

						//because we apply gravity at the start of the loop for these calculations we get rid of it
						entity->netForce.x -= entity->forceAppliedToAccelerationAndVelocity.x;
						entity->netForce.y -= entity->forceAppliedToAccelerationAndVelocity.y;
						entity->acceleration.x = entity->netForce.x / entity->mass;
						entity->acceleration.y = entity->netForce.y / entity->mass;

						float deltaTime = GetDeltaTime();
						entity->physicsVelocity.x += entity->acceleration.x * iterationTimeStep;
						entity->physicsVelocity.y += entity->acceleration.y * iterationTimeStep;
						entity->netForce.x += entity->forceAppliedToAccelerationAndVelocity.x;
						entity->netForce.y += entity->forceAppliedToAccelerationAndVelocity.y;

						MoveEntity(entity, iterationTimeStep);
						CalibrateEntityWithGrid(gameState, entity);

					}
					if (k != gameState->SOLVER_ITERATIONS - 1) {
						entity->netForce = { 0, 0 };
						entity->forceAppliedToAccelerationAndVelocity = { 0, 0 };
					}
				}
				if (entity->flags & PLAYER_FLAG && entity->netForce.y < -1) {
					//std::cout << "Net force on the player: (" << entity->netForce.x << ", " << entity->netForce.y << ") " << std::endl;
					//std::cout << "FPS: " << GetFPS() << std::endl;
					//std::cout << "grid pos of the player(row, column): " << entity->gridRowIdx << ", " << entity->gridColumnIdx << std::endl;
					std::cout << "Num of relevant entities: " << numOfRelevantEntities << std::endl;
				}

				DrawEntity(entity);
				DrawEntityForceLine(entity);
			}

			for (int i = 0; i < gameState->addedEntities; i++) {
				(gameState->entities + i)->gravityApplied = false;
			}

			inputGiven = false;
			inputForce = { 0, 0 };

		EndDrawing();
		std::cout << "Frame number " << gameState->frameCount++ << "has ended" << std::endl << std::endl;
		//throw std::runtime_error("asd");
	}
	RetractSize(gameState, sizeof(Entity*) * 20);
	CloseWindow();
	return 0;
}
