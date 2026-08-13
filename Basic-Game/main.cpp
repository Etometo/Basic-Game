#include <raylib.h>
#include <iostream>
#include "player.h"
#include "GameState.h"
#include <iostream>
#include <thread>
#include <chrono>


int main() {
	void* rawMemory = new uint8_t[150 * 1024 * 1024];
	memset(rawMemory, 0, 150 * 1024 * 1024);
	GameState* gameState = (GameState*)rawMemory;
	if (gameState->isInitialized == false) {
		gameState->arena.base = (uint8_t*)rawMemory + sizeof(GameState);
		gameState->arena.used = 0;
		gameState->arena.usedTemporary = 0;
		gameState->arena.capacity = 150 * 1024 - sizeof(GameState);
		gameState->arena.usableCapacity = gameState->arena.capacity - 1024;
		gameState->entitiesCapacity = 2000;
		gameState->nextEmptyPlaceForEntity = gameState->entities;
		gameState->lastEntityOnEntities = gameState->entities;
		gameState->goalFps = 60;
		gameState->isInitialized = true;
		gameState->gravityConstant = 98 * 4;
		gameState->EPSILON = 1e-5f;
		gameState->ClickThresholdFrames = gameState->goalFps / 3;
		gameState->nextAvailableId = 1;
		gameState->WINDOW_HEIGHT = 800;
		gameState->WINDOW_WIDTH = 800;
		gameState->gridSquareEdgeLength = 100;
		gameState->SOLVER_ITERATIONS = 5;
	}
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
	
	VertexData* rectData = (VertexData*)PushSize(gameState, sizeof(VertexData) * 4);
	VertexData* rectDataEnd = rectData;
	*(rectDataEnd++) = VertexData{ 50, 50 };
	*(rectDataEnd++) = VertexData{ 50, -50 };
	*(rectDataEnd++) = VertexData{ -50, -50 };
	*(rectDataEnd++) = VertexData{ -50, 50 };

	VertexData* floorRectData = (VertexData*)PushSize(gameState, sizeof(VertexData) * 6);
	VertexData* floorRectDataEnd = floorRectData;
	*(floorRectDataEnd++) = VertexData{ 399, 50 };
	*(floorRectDataEnd++) = VertexData{ 399, -50 };
	*(floorRectDataEnd++) = VertexData{ 0, -50 };
	*(floorRectDataEnd++) = VertexData{ -399, -50 };
	*(floorRectDataEnd++) = VertexData{ -399, 50 };
	*(floorRectDataEnd++) = VertexData{ 0, 50 };

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
	uint32_t playerFlags = GRAVITY_FLAG | PLAYER_FLAG | GROUND_COLLISION_FLAG | PHYSICS_FLAG;
	
	Vector2 floorCenterPos = { 400, 750 };
	Entity* floor = InitializeAndPushEntity(gameState, floorRectData, floorRectDataEnd, 0.2, NON_MOVING_FLAG | GROUND_COLLISION_FLAG | PHYSICS_FLAG, floorCenterPos);
	floor->frictionCons = 0.1;

	//throw std::runtime_error("do the rotation stuff and fix the very little impulses preventing objects from staying on top of each other");
	InitWindow(gameState->WINDOW_WIDTH, gameState->WINDOW_HEIGHT, "asd");
	SetTargetFPS(gameState->goalFps);
	Entity** relevantEntities = (Entity**)PushSize(gameState, sizeof(Entity*) * 500);
	int numOfRelevantEntities;

	bool inputGiven = false;
	Vector2 inputForce = { 0, 0 };

	bool mouseLeftBeingHeld = false;
	uint64_t mouseLeftHoldFramesCount = 0;
	Vector2 cuttingLineStartPosition, cuttingLineEndPosition;

	Entity* entityBeingCut;
	bool readyForNewEntityInitialization = false;
	bool entityInitialized = false;
	uint32_t idOfEntityBeingCut = 0;

	entityBeingCut = (Entity*)InitializeAndPushEntity(gameState, rectData, rectDataEnd, 10, BEING_CUT_FLAG, { 400, 100 });
	readyForNewEntityInitialization = false;
	entityInitialized = true;
	idOfEntityBeingCut = entityBeingCut->id;
	Entity* cutEntityPiece1 = nullptr;
	Entity* cutEntityPiece2 = nullptr;

	while (!WindowShouldClose()) {
		//std::cout << "Frame number " << gameState->frameCount << "is starting" << std::endl << std::endl;
		BeginDrawing();
			ClearBackground(RAYWHITE);

			if (readyForNewEntityInitialization) {
				entityBeingCut = (Entity*)InitializeAndPushEntity(gameState, rectData, rectDataEnd, 10, BEING_CUT_FLAG, { 400, 100 });
				readyForNewEntityInitialization = false;
				entityInitialized = true;
				idOfEntityBeingCut = entityBeingCut->id;
			}

			if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
				Vector2 mousePos = GetMousePosition();
				mouseLeftHoldFramesCount++;
				if (mouseLeftBeingHeld) {
					DrawLine(cuttingLineStartPosition.x, cuttingLineStartPosition.y, mousePos.x, mousePos.y, RED);
				}
				else {
					mouseLeftBeingHeld = true;
					cuttingLineStartPosition = mousePos;
				}
			}
			else {
				//hold and release
				if (mouseLeftBeingHeld && mouseLeftHoldFramesCount > gameState->ClickThresholdFrames) {
					Vector2 mousePos = GetMousePosition();
					cuttingLineEndPosition = mousePos;
					Entity* newEntity1 = 0;
					Entity* newEntity2 = 0;
					uint32_t newEntityFlags = BEING_CHOSEN_FLAG;
					int operationStatus = CutEntityIntoTwoPiecesByALine(gameState, entityBeingCut, cuttingLineStartPosition, cuttingLineEndPosition, newEntityFlags, newEntity1, newEntity2);
					if (operationStatus == ENTITY_WAS_CUT) {
						cutEntityPiece1 = newEntity1;
						cutEntityPiece2 = newEntity2;
						cutEntityPiece1->centerPosition.x -= 100;
						cutEntityPiece1->temporaryPositionChange.x = -100;
						cutEntityPiece2->centerPosition.x += 100;
						cutEntityPiece2->temporaryPositionChange.x = 100;

						DeleteEntity(gameState, entityBeingCut);
					}
				}
				//click
				if (mouseLeftBeingHeld && mouseLeftHoldFramesCount < gameState->ClickThresholdFrames) {
					std::cout << "click" << std::endl;
					Vector2 mousePos = GetMousePosition();
					int numOfCloseEntities = CalculateRelevantEntitiesForPosition(gameState, mousePos, relevantEntities);
					bool selectedAnEntity = false;
					for (int i = 0; i < numOfCloseEntities; i++) {
						Entity* entity = relevantEntities[i];
						bool pointIsInsideEntity = CheckIfAPointIsInsideAnEntity(mousePos, entity);
						if (pointIsInsideEntity) {
							std::cout << "Entity " << entity->id << " clicked" << std::endl;
						}
						if (pointIsInsideEntity && (entity->flags & BEING_CHOSEN_FLAG)) {
							if (entity->id == cutEntityPiece1->id) {
								DeleteEntity(gameState, cutEntityPiece2);
								cutEntityPiece1->flags &= !BEING_CHOSEN_FLAG;
								cutEntityPiece1->flags |= (PHYSICS_FLAG | GRAVITY_FLAG | GROUND_COLLISION_FLAG);
								cutEntityPiece1->centerPosition.x -= cutEntityPiece1->temporaryPositionChange.x;
								cutEntityPiece1->centerPosition.y -= cutEntityPiece1->temporaryPositionChange.y;
								selectedAnEntity = true;
							}
							else if (entity->id == cutEntityPiece2->id) {
								DeleteEntity(gameState, cutEntityPiece1);
								cutEntityPiece2->flags &= !BEING_CHOSEN_FLAG;
								cutEntityPiece2->flags |= (PHYSICS_FLAG | GRAVITY_FLAG | GROUND_COLLISION_FLAG);
								cutEntityPiece2->centerPosition.x -= cutEntityPiece2->temporaryPositionChange.x;
								cutEntityPiece2->centerPosition.y -= cutEntityPiece2->temporaryPositionChange.y;
								selectedAnEntity = true;
							}
							cutEntityPiece1 = nullptr;
							cutEntityPiece2 = nullptr;
							readyForNewEntityInitialization = true;
						}
					}
					if (selectedAnEntity == false) {
						std::cout << " ";
					}
				}
				mouseLeftHoldFramesCount = 0;
				mouseLeftBeingHeld = false;
			}

			float deltaTime = GetDeltaTime();
			for (int i = 0; i < gameState->lastEntityOnEntities + 1 - gameState->entities; i++) {
				Entity* entity = gameState->entities + i;
				if (entity->id == 0) {
					continue;
				}
				numOfRelevantEntities = CalculateRelevantEntitiesForEntity(gameState, entity, relevantEntities, i);
				float solverIterationTimeStep = deltaTime / gameState->SOLVER_ITERATIONS;
				if ((entity->flags & PHYSICS_FLAG)) {
					if ((entity->flags & GRAVITY_FLAG) > 0) {
						float forcePerVertex = (entity->mass * gameState->gravityConstant) / (entity->vertexDataEnd - entity->vertexData);
						for (int v = 0; v < entity->vertexDataEnd - entity->vertexData; v++) {
							Vector2 vertexPos = entity->centerPosition;
							vertexPos.x += entity->vertexData[v].position.x;
							vertexPos.y += entity->vertexData[v].position.y;
							ApplyForceToEntitiesVelocityImmediately(entity, { 0, forcePerVertex}, deltaTime, vertexPos);
						}
						entity->gravityApplied = true;
					}
					for (int j = 0; j < numOfRelevantEntities; j++) {
						Entity* relevantEntity = relevantEntities[j];
						if ((relevantEntity->flags & PHYSICS_FLAG) == 0) {
							continue;
						}
						CollisionInfo collInfo = DetectCollisionWithEntity(entity, relevantEntity);
						float totalMass = entity->mass + relevantEntity->mass;

						if (collInfo.minOverlap > 0) {
							Vector2 forceApplicationPoint = CalculateForceApplicationPoint(entity, relevantEntity);
							if (forceApplicationPoint.x == 0 && forceApplicationPoint.y == 0) {
								forceApplicationPoint = entity->centerPosition;
							}
							
							for(int k = 0; k < gameState->SOLVER_ITERATIONS; k++){
								Vector2 impulse = { 0, 0 }, relativeVelocityOfForceApplicationPoint = {0, 0};
								CalculateAndApplyImpulse(gameState, entity, relevantEntity, collInfo, impulse, relativeVelocityOfForceApplicationPoint, forceApplicationPoint, solverIterationTimeStep);
								HandleFriction(gameState, entity, relevantEntity, collInfo, impulse, relativeVelocityOfForceApplicationPoint, forceApplicationPoint, solverIterationTimeStep);
							}
						}
					}
					MoveEntity(entity, deltaTime);
					CalibrateEntityWithGrid(gameState, entity);
					DrawEntityForceLine(entity);
					entity->netForce = { 0, 0 };
					entity->forceAppliedToAccelerationAndVelocity = { 0, 0 };
					entity->forcesMultipliedByAppliedTime = { 0, 0 };
					entity->torque = 0;
				}
				DrawEntity(entity);
			}

			for (int i = 0; i < gameState->addedEntities; i++) {
				(gameState->entities + i)->gravityApplied = false;
			}

			inputGiven = false;
			inputForce = { 0, 0 };

		EndDrawing();
		//std::cout << "Frame number " << gameState->frameCount++ << "has ended" << std::endl << std::endl;
	}
	RetractSize(gameState, sizeof(Entity*) * 20);
	CloseWindow();
	return 0;
}
