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
		gameState->arena.usedTemporary = 0;
		gameState->arena.capacity = 150 * 1024 - sizeof(GameState);
		gameState->arena.usableCapacity = gameState->arena.capacity - 1024;
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
	
	VertexData* rectData = (VertexData*)PushSize(gameState, sizeof(VertexData) * 4);
	VertexData* rectDataEnd = rectData;
	*(rectDataEnd++) = VertexData{ 50, 50 };
	*(rectDataEnd++) = VertexData{ 50, -50 };
	*(rectDataEnd++) = VertexData{ -50, -50 };
	*(rectDataEnd++) = VertexData{ -50, 50 };

	VertexData* floorRectData = (VertexData*)PushSize(gameState, sizeof(VertexData) * 6);
	VertexData* floorRectDataEnd = floorRectData;
	*(floorRectDataEnd++) = VertexData{ 400, 50 };
	*(floorRectDataEnd++) = VertexData{ 400, -50 };
	*(floorRectDataEnd++) = VertexData{ 0, -50 };
	*(floorRectDataEnd++) = VertexData{ -400, -50 };
	*(floorRectDataEnd++) = VertexData{ -400, 50 };
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
	Entity** relevantEntities = (Entity**)PushSize(gameState, sizeof(Entity*) * 50);
	int numOfRelevantEntities;

	bool inputGiven = false;
	Vector2 inputForce = { 0, 0 };

	bool mouseLeftBeingHeld = false;
	Vector2 cuttingLineStartPosition, cuttingLineEndPosition;

	Entity* entityBeingCut;
	bool readyForNewEntityInitialization = false;
	bool entityInitialized = false;
	uint32_t idOfEntityBeingCut = 0;

	entityBeingCut = (Entity*)InitializeAndPushEntity(gameState, rectData, rectDataEnd, 10, BEING_CUT_FLAG, { 400, 100 });
	readyForNewEntityInitialization = false;
	entityInitialized = true;
	idOfEntityBeingCut = entityBeingCut->id;

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
				if (mouseLeftBeingHeld) {
					std::cout << "mouseBeingHeld" << std::endl;
					DrawLine(cuttingLineStartPosition.x, cuttingLineStartPosition.y, mousePos.x, mousePos.y, RED);
				}
				else {
					std::cout << "mouse clicked";
					mouseLeftBeingHeld = true;
					cuttingLineStartPosition = mousePos;
				}
			}
			else {
				if (mouseLeftBeingHeld) {
					Vector2 mousePos = GetMousePosition();
					mouseLeftBeingHeld = false;
					cuttingLineEndPosition = mousePos;
					uint16_t vertexCount = entityBeingCut->vertexDataEnd - entityBeingCut->vertexData;

					bool cutIsVertical = false, edgeIsVertical = false;
					float cutDx = cuttingLineEndPosition.x - cuttingLineStartPosition.x;
					float cuttingLineSlope;
					if (abs(cutDx) < FLT_EPSILON) {
						cutIsVertical = true;
					}
					if (!cutIsVertical) {
						cuttingLineSlope = (cuttingLineEndPosition.y - cuttingLineStartPosition.y) / (cutDx);
					}
					Vector2* intersectionPoints = (Vector2*)PushTemporarySize(gameState, (vertexCount - 1) * sizeof(Vector2));
					Vector2* intersectionPointsEnd = intersectionPoints;
					uint32_t* indicesOfPointOnLineIntersections = (uint32_t*)PushTemporarySize(gameState, vertexCount * sizeof(uint32_t));
					uint32_t* indicesOfPointOnLineIntersectionsEnd = indicesOfPointOnLineIntersections;

					for (int i = 0; i < vertexCount; i++) {
						Vector2 vertex1Pos = entityBeingCut->vertexData[i].position;
						Vector2 vertex2Pos;
						if (i + 1 >= vertexCount) {
							vertex2Pos = entityBeingCut->vertexData[0].position;
						}
						else {
							vertex2Pos = entityBeingCut->vertexData[i+1].position;
						}
						vertex1Pos.x += entityBeingCut->centerPosition.x;
						vertex2Pos.x += entityBeingCut->centerPosition.x;
						vertex1Pos.y += entityBeingCut->centerPosition.y;
						vertex2Pos.y += entityBeingCut->centerPosition.y;

						float edgeDx = vertex2Pos.x - vertex1Pos.x;
						edgeIsVertical = abs(edgeDx) < FLT_EPSILON ? true : false;
						float edgeSlope;

						float xValueOfIntersection;
						float yValueOfIntersection;
						if (cutIsVertical && edgeIsVertical) {
							continue;
						}
						else if (cutIsVertical) {
							edgeSlope = (vertex2Pos.y - vertex1Pos.y) / edgeDx;
							xValueOfIntersection = cuttingLineStartPosition.x;
							yValueOfIntersection = edgeSlope * (xValueOfIntersection - vertex1Pos.x) + vertex1Pos.y;
						}
						else if (edgeIsVertical) {
							xValueOfIntersection = vertex1Pos.x;
							yValueOfIntersection = cuttingLineSlope * (xValueOfIntersection - cuttingLineStartPosition.x) + cuttingLineStartPosition.y;
						}
						else {
							edgeSlope = (vertex2Pos.y - vertex1Pos.y) / edgeDx;
							xValueOfIntersection = (cuttingLineSlope * cuttingLineStartPosition.x - edgeSlope * vertex1Pos.x + vertex1Pos.y - cuttingLineStartPosition.y) / (cuttingLineSlope - edgeSlope);
							yValueOfIntersection = cuttingLineSlope * (xValueOfIntersection - cuttingLineStartPosition.x) + cuttingLineStartPosition.y;
						}
						Vector2 positionFromVertex1 = { xValueOfIntersection - vertex1Pos.x, yValueOfIntersection - vertex1Pos.y };
						Vector2 positionFromVertex2 = { xValueOfIntersection - vertex2Pos.x, yValueOfIntersection - vertex2Pos.y };
						if (DotProduct(positionFromVertex1, positionFromVertex2) <= 0) {
							*(intersectionPointsEnd) = { xValueOfIntersection, yValueOfIntersection };
							intersectionPointsEnd++;
							if (magnitude(positionFromVertex1) < FLT_EPSILON) {
								*indicesOfPointOnLineIntersectionsEnd = i;
								indicesOfPointOnLineIntersectionsEnd++;
							}
							if (magnitude(positionFromVertex2) < FLT_EPSILON) {
								*indicesOfPointOnLineIntersectionsEnd = i + 1;
								indicesOfPointOnLineIntersectionsEnd++;
							}
						}
						std::cout << " ";
					}

					unsigned int intersectionCount = intersectionPointsEnd - intersectionPoints;
					if (intersectionCount > 0) {
						VertexData* vertexData1 = (VertexData*)PushSize(gameState, (vertexCount + intersectionCount) * sizeof(VertexData));
						VertexData* vertexData1End = vertexData1;
						VertexData* vertexData2 = (VertexData*)PushSize(gameState, (vertexCount + intersectionCount) * sizeof(VertexData));
						VertexData* vertexData2End = vertexData2;
						Vector2 vertexData1center = { 0, 0 };
						Vector2 vertexData2center = { 0, 0 };
						for (int i = 0; i < intersectionCount; i++) {
							(*vertexData1End).position = intersectionPoints[i];
							(*vertexData1End).position.x -= entityBeingCut->centerPosition.x;
							(*vertexData1End).position.y -= entityBeingCut->centerPosition.y;
							vertexData1center.x += (*vertexData1End).position.x;
							vertexData1center.y += (*vertexData1End).position.y;

							(*vertexData2End).position = intersectionPoints[i];
							(*vertexData2End).position.x -= entityBeingCut->centerPosition.x;
							(*vertexData2End).position.y -= entityBeingCut->centerPosition.y;
							vertexData2center.x += (*vertexData2End).position.x;
							vertexData2center.y += (*vertexData2End).position.y;

							vertexData1End++;
							vertexData2End++;
						}
						for (int i = 0; i < vertexCount; i++) {
							bool thisVertexAlreadyAdded = false;
							for (int j = 0; j < indicesOfPointOnLineIntersectionsEnd - indicesOfPointOnLineIntersections; j++) {
								if (i == indicesOfPointOnLineIntersections[j]) {
									thisVertexAlreadyAdded = true;
								}
							}
							if (thisVertexAlreadyAdded) {
								continue;
							}
							Vector2 vertexPos = entityBeingCut->vertexData[i].position;
							vertexPos.x += entityBeingCut->centerPosition.x;
							vertexPos.y += entityBeingCut->centerPosition.y;
							float sideOfThePoint = (cuttingLineEndPosition.x - cuttingLineStartPosition.x) * (vertexPos.y - cuttingLineStartPosition.y) - (cuttingLineEndPosition.y - cuttingLineStartPosition.y) * (vertexPos.x - cuttingLineStartPosition.x);
							if (sideOfThePoint < 0) {
								(*vertexData1End).position = entityBeingCut->vertexData[i].position;
								vertexData1End++;
								vertexData1center.x += entityBeingCut->vertexData[i].position.x;
								vertexData1center.y += entityBeingCut->vertexData[i].position.y;
							}
							else {
								(*vertexData2End).position = entityBeingCut->vertexData[i].position;
								vertexData2End++;
								vertexData2center.x += entityBeingCut->vertexData[i].position.x;
								vertexData2center.y += entityBeingCut->vertexData[i].position.y;
							}
						}
						vertexData1center.x /= vertexData1End - vertexData1;
						vertexData1center.y /= vertexData1End - vertexData1;

						vertexData2center.x /= vertexData2End - vertexData2;
						vertexData2center.y /= vertexData2End - vertexData2;

						for (int i = 0; i < vertexData1End - vertexData1; i++) {
							vertexData1[i].position.x -= vertexData1center.x;
							vertexData1[i].position.y -= vertexData1center.y;
						}
						for (int i = 0; i < vertexData2End - vertexData2; i++) {
							vertexData2[i].position.x -= vertexData2center.x;
							vertexData2[i].position.y -= vertexData2center.y;
						}

						vertexData1center.x += entityBeingCut->centerPosition.x;
						vertexData1center.y += entityBeingCut->centerPosition.y;

						vertexData2center.x += entityBeingCut->centerPosition.x;
						vertexData2center.y += entityBeingCut->centerPosition.y;

						uint32_t entityFlags = GRAVITY_FLAG | PHYSICS_FLAG | GROUND_COLLISION_FLAG;
						Entity* newEntity1 = (Entity*)InitializeAndPushEntity(gameState, vertexData1, vertexData1End, 10, entityFlags, vertexData1center);
						Entity* newEntity2 = (Entity*)InitializeAndPushEntity(gameState, vertexData2, vertexData2End, 10, entityFlags, vertexData2center);
					}
				}
			}


			float deltaTime = GetDeltaTime();
			for (int i = 0; i < gameState->addedEntities; i++) {
				Entity* entity = gameState->entities + i;
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
				}
				MoveEntity(entity, deltaTime);
				CalibrateEntityWithGrid(gameState, entity);

				DrawEntity(entity);
				DrawEntityForceLine(entity);
				entity->netForce = { 0, 0 };
				entity->forceAppliedToAccelerationAndVelocity = { 0, 0 };
				entity->forcesMultipliedByAppliedTime = { 0, 0 };
				entity->torque = 0;
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
