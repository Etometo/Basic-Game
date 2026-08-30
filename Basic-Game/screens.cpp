#include "GameState.h"
#include <iostream>

void RestartGameAndChangeScreenToMainScreen(GameState* gameState);

void ChangeScreenTo(GameState* gameState, SCREEN_CODES screenCode) {
	if (screenCode == MAIN_SCREEN && gameState->currentScreenCode == END_SCREEN) {
		RestartGameAndChangeScreenToMainScreen(gameState);
	}
	else {
		gameState->currentScreenCode = screenCode;
		for (int i = 0; i < (gameState->lastEntityOnEntities - gameState->entities) + 1; i++) {
			Entity* entity = gameState->entities + i;
			if (entity->id == 0) {
				continue;
			}
			if (entity->screenCode != screenCode) {
				entity->flags |= INVISIBLE_FLAG;
			}
			else {
				entity->flags &= ~INVISIBLE_FLAG;
			}
		}
	}
}

void InitializeGameplayScreen(GameState* gameState) {
	VertexData* rectData = (VertexData*)PushSize(gameState, sizeof(VertexData) * 4);
	VertexData* rectDataEnd = rectData;
	*(rectDataEnd++) = VertexData{ 50, 50 };
	*(rectDataEnd++) = VertexData{ 50, -50 };
	*(rectDataEnd++) = VertexData{ -50, -50 };
	*(rectDataEnd++) = VertexData{ -50, 50 };
	gameState->rectData = rectData;
	gameState->rectDataEnd = rectDataEnd;

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

	Vector2 floorCenterPos = { 400, 750 };
	Entity* floor = InitializeAndPushEntity(gameState, floorRectData, floorRectDataEnd, 0.2, NON_MOVING_FLAG | GROUND_COLLISION_FLAG | PHYSICS_FLAG | IN_CONTACT_WITH_GROUND_FLAG, floorCenterPos, GAMEPLAY_SCREEN);
	floor->frictionCons = 130;

	gameState->gameplayState = CUTTING_OR_CHOOSING_AN_ENTITY;
	gameState->floor = floor;
}

void ResetChosenEntityAndGameplayStateIntoCuttingOrChoosingState(GameState* gameState) {
	if (gameState->entityBeingCut != nullptr) {
		gameState->chosenPiece->centerPosition = gameState->newEntitySpawnPoint;
		gameState->chosenPiece->centerPosition.x -= 100;
		gameState->chosenPiece->temporaryPositionChange.x = -100;
		gameState->chosenPiece->physicsVelocity = { 0, 0 };
		gameState->chosenPiece->flags = BEING_CHOSEN_FLAG | IS_A_BUILDING_BLOCK_FLAG;
		gameState->chosenPiece->angle = 0;
		gameState->chosenPiece->rotationalVelocity = 0;
		gameState->chosenPiece->physicsVelocity = { 0, 0 };

		gameState->entityBeingCut->centerPosition.x += 100;
		gameState->entityBeingCut->temporaryPositionChange.x = 100;
		gameState->entityBeingCut->flags = BEING_CHOSEN_FLAG | IS_A_BUILDING_BLOCK_FLAG;

		gameState->cutPiece1 = gameState->chosenPiece;
		gameState->cutPiece2 = gameState->entityBeingCut;

		gameState->chosenPiece = nullptr;
		Entity* emptyEntity = InitializeAndPushEntity(gameState, gameState->rectData, gameState->rectDataEnd, 0, 0, { 0, 0 }, GAMEPLAY_SCREEN);
		DeleteEntity(gameState, emptyEntity);
		gameState->entityBeingCut = emptyEntity;

		gameState->gameplayState = CUTTING_OR_CHOOSING_AN_ENTITY;
		CalibrateEntityWithGrid(gameState, gameState->cutPiece1);
		CalibrateEntityWithGrid(gameState, gameState->cutPiece2);
	}
	else {
		gameState->chosenPiece->centerPosition = gameState->newEntitySpawnPoint;
		gameState->chosenPiece->physicsVelocity = { 0, 0 };
		gameState->chosenPiece->flags |= BEING_CUT_FLAG | BEING_CHOSEN_FLAG;
		gameState->chosenPiece->flags &= ~(PHYSICS_FLAG | GRAVITY_FLAG | GROUND_COLLISION_FLAG | NOT_IN_FREE_FALL_FLAG);
		gameState->chosenPiece->angle = 0;
		gameState->chosenPiece->rotationalVelocity = 0;
		gameState->chosenPiece->physicsVelocity = { 0, 0 };
		gameState->entityBeingCut = gameState->chosenPiece;
		gameState->chosenPiece = nullptr;
		gameState->gameplayState = CUTTING_OR_CHOOSING_AN_ENTITY;
		gameState->framesElapsedWaitingForTheEntityToStop = 0;
		CalibrateEntityWithGrid(gameState, gameState->entityBeingCut);
	}
}

void UpdateGameplayScreen(GameState* gameState, InputInfo inputInfo) {
	uint32_t relevantEntitiesSize = sizeof(Entity*) * 500;
	Entity** relevantEntities = (Entity**)PushTemporarySize(gameState, relevantEntitiesSize);

	
	if (gameState->gameplayState == CUTTING_OR_CHOOSING_AN_ENTITY) {
		if (gameState->entityBeingCut == nullptr) {
			if(gameState->freeTimeFramesCounter >= gameState->averageFPS*gameState->freeTimeLimitInSeconds){
				for (int i = 0; i < (gameState->lastEntityOnEntities - gameState->entities) + 1; i++) {
					Entity* entity = gameState->entities + i;
					if (entity->flags & (IS_A_BUILDING_BLOCK_FLAG)) {
						entity->flags |= NON_MOVING_FLAG;
						entity->flags &= ~GRAVITY_FLAG;
					}
				}
				VertexData* newVertexData = (VertexData*)PushSize(gameState, sizeof(VertexData) * 4);
				std::memcpy(newVertexData, gameState->rectData, sizeof(VertexData) * 4);

				uint32_t entityFlags = BEING_CUT_FLAG | BEING_CHOSEN_FLAG | IS_A_BUILDING_BLOCK_FLAG;
				gameState->entityBeingCut = (Entity*)InitializeAndPushEntity(gameState, newVertexData, newVertexData + 4, 10, entityFlags, gameState->newEntitySpawnPoint, GAMEPLAY_SCREEN);
				gameState->entityBeingCut->frictionCons = 200;
				gameState->readyForNewEntityInitialization = false;
				gameState->entityInitialized = true;
				gameState->numOfEntitiesSpawnedAndUsed++;
				if (gameState->numOfEntitiesSpawnedAndUsed > gameState->limitOfSpawnedEntities) {
					InitializeEndScreen(gameState);
					ChangeScreenTo(gameState, END_SCREEN);
					return;
				}
				gameState->freeTimeFramesCounter = 0;
			}
			else if (gameState->freeTimeFramesCounter == 0){
				for (int i = 0; i < (gameState->lastEntityOnEntities - gameState->entities) + 1; i++) {
					Entity* entity = gameState->entities + i;
					if (entity->flags & (IS_A_BUILDING_BLOCK_FLAG | NON_MOVING_FLAG) && entity->id != gameState->floor->id) {
						entity->flags &= ~NON_MOVING_FLAG;
						entity->flags |= GRAVITY_FLAG;
						entity->physicsVelocity = { 0, 0 };
					}
				}
				gameState->freeTimeFramesCounter++;
				gameState->gameplayState = FREE_TIME_OF_ENTITIES;
			}
		}
		else {
			gameState->entityBeingCut->flags &= ~INVISIBLE_FLAG;
		}

		if (inputInfo.mouseInputInfo.mouseReleasedThisFrame) {
			//hold and release
			if (inputInfo.mouseInputInfo.inputDurationFrames > gameState->ClickThresholdFrames && gameState->gameplayState == CUTTING_OR_CHOOSING_AN_ENTITY) {
				Entity* newEntity1 = 0;
				Entity* newEntity2 = 0;
				uint32_t newEntityFlags = BEING_CHOSEN_FLAG | IS_A_BUILDING_BLOCK_FLAG;
				int operationStatus = CutEntityIntoTwoPiecesByALine(gameState, gameState->entityBeingCut, inputInfo.mouseInputInfo.inputPositions[0], inputInfo.mouseInputInfo.inputPositions[1], newEntityFlags, newEntity1, newEntity2);
				if (operationStatus == ENTITY_WAS_CUT) {
					gameState->cutPiece1 = newEntity1;
					gameState->cutPiece2 = newEntity2;
					gameState->cutPiece1->centerPosition.x -= 100;
					gameState->cutPiece1->temporaryPositionChange.x = -100;
					gameState->cutPiece2->centerPosition.x += 100;
					gameState->cutPiece2->temporaryPositionChange.x = 100;

					DeleteEntity(gameState, gameState->entityBeingCut);
					std::cout << " ";
				}
			}
			//click
			if (inputInfo.mouseInputInfo.inputDurationFrames < gameState->ClickThresholdFrames) {
				Vector2 mousePos = inputInfo.mouseInputInfo.inputPositions[1];
				int numOfCloseEntities = CalculateRelevantEntitiesForPosition(gameState, mousePos, relevantEntities);
				bool selectedAnEntity = false;
				for (int i = 0; i < numOfCloseEntities; i++) {
					Entity* clickedEntity = relevantEntities[i];
					bool pointIsInsideEntity = CheckIfAPointIsInsideAnEntity(mousePos, clickedEntity, clickedEntity->vertexData);

					if (pointIsInsideEntity) {
						if (clickedEntity->flags & BEING_CHOSEN_FLAG && gameState->chosenPiece == nullptr) {
							clickedEntity->flags &= ~(BEING_CHOSEN_FLAG | BEING_CUT_FLAG);
							clickedEntity->flags |= ADJUSTING_POSITION_FLAG;

							//this means the entity was cut
							if (gameState->entityBeingCut->id == 0) {
								gameState->cutPiece1->centerPosition.x -= gameState->cutPiece1->temporaryPositionChange.x;
								gameState->cutPiece1->centerPosition.y -= gameState->cutPiece1->temporaryPositionChange.y;
								gameState->cutPiece2->centerPosition.x -= gameState->cutPiece2->temporaryPositionChange.x;
								gameState->cutPiece2->centerPosition.y -= gameState->cutPiece2->temporaryPositionChange.y;
								gameState->cutPiece1->temporaryPositionChange = { 0, 0 };
								gameState->cutPiece2->temporaryPositionChange = { 0, 0 };

								if (clickedEntity->id == gameState->cutPiece1->id) {
									gameState->entityBeingCut = gameState->cutPiece2;
									gameState->cutPiece2->flags |= INVISIBLE_FLAG;
								}
								else if (clickedEntity->id == gameState->cutPiece2->id) {
									gameState->entityBeingCut = gameState->cutPiece1;
									gameState->cutPiece1->flags |= INVISIBLE_FLAG;
								}
							}
							else if(gameState->entityBeingCut->id == clickedEntity->id){
								gameState->entityBeingCut = nullptr;
							}
							gameState->cutPiece1 = nullptr;
							gameState->cutPiece2 = nullptr;
							gameState->gameplayState = ADJUSTING_THE_POSITION_OF_THE_CHOSEN_ENTITY;
							gameState->chosenPiece = clickedEntity;
						}

					}
				}
			}
		}
		else {
			if (inputInfo.mouseInputInfo.inputType == LEFT_CLICK) {
				DrawLine(inputInfo.mouseInputInfo.inputPositions[0].x, inputInfo.mouseInputInfo.inputPositions[0].y, inputInfo.mouseInputInfo.inputPositions[1].x, inputInfo.mouseInputInfo.inputPositions[1].y, RED);
			}
		}
	}
	else if (gameState->gameplayState == ADJUSTING_THE_POSITION_OF_THE_CHOSEN_ENTITY) {
		if (gameState->chosenPiece != nullptr) {
			if (inputInfo.keyCodes & A_KEY_CODE && gameState->chosenPiece->centerPosition.x >= 100) {
				gameState->chosenPiece->centerPosition.x -= 2;
			}
			if (inputInfo.keyCodes & D_KEY_CODE && gameState->chosenPiece->centerPosition.x <= 700) {
				gameState->chosenPiece->centerPosition.x += 2;
			}
			if (inputInfo.keyCodes & ENTER_KEY_CODE) {
				gameState->chosenPiece->flags &= ~ADJUSTING_POSITION_FLAG;
				gameState->chosenPiece->flags |= (PHYSICS_FLAG | GRAVITY_FLAG | GROUND_COLLISION_FLAG | NOT_IN_FREE_FALL_FLAG);
				gameState->gameplayState = WAITING_FOR_THE_ENTITY_TO_STOP;
			}
		}
	}
	else if (gameState->gameplayState == WAITING_FOR_THE_ENTITY_TO_STOP) {
		float deltaTime = GetDeltaTime();
		for (int i = 0; i < (gameState->lastEntityOnEntities - gameState->entities) + 1; i++) {
			Entity* entity = gameState->entities + i;
			if (entity->id == 0) {
				continue;
			}
			int numOfRelevantEntities = CalculateRelevantEntitiesForEntity(gameState, entity, relevantEntities, i);
			float solverIterationTimeStep = deltaTime / gameState->SOLVER_ITERATIONS;
			if ((entity->flags & PHYSICS_FLAG)) {
				if (entity->flags & GRAVITY_FLAG) {
					if(!(entity->flags & NOT_IN_FREE_FALL_FLAG)){
						float forcePerVertex = (entity->mass * gameState->gravityConstant) / (entity->vertexDataEnd - entity->vertexData);
						for (int v = 0; v < entity->vertexDataEnd - entity->vertexData; v++) {
							Vector2 vertexPos = entity->centerPosition;
							vertexPos.x += entity->vertexData[v].position.x;
							vertexPos.y += entity->vertexData[v].position.y;
							ApplyForceToEntitiesVelocityImmediately(entity, { 0, forcePerVertex}, deltaTime, vertexPos);
						}
						entity->gravityApplied = true;
					}
					else {
						entity->centerPosition.y += freeFallSpeed;
					}
				}

				for (int j = 0; j < numOfRelevantEntities; j++) {
					Entity* relevantEntity = relevantEntities[j];
					if ((relevantEntity->flags & PHYSICS_FLAG) == 0) {
						continue;
					}
					CollisionInfo collInfo = DetectCollisionWithEntity(gameState, entity, relevantEntity);
					float totalMass = entity->mass + relevantEntity->mass;

					if (collInfo.minOverlap > FLT_EPSILON) {
						if (entity->flags & NOT_IN_FREE_FALL_FLAG) {
							entity->flags &= ~NOT_IN_FREE_FALL_FLAG;
						}
						if (relevantEntity->flags & NOT_IN_FREE_FALL_FLAG) {
							relevantEntity->flags &= ~NOT_IN_FREE_FALL_FLAG;
						}
						Vector2 forceApplicationPoint = CalculateForceApplicationPoint(gameState, entity, relevantEntity, 0, collInfo);
						if (forceApplicationPoint.x == 0 && forceApplicationPoint.y == 0) {
							forceApplicationPoint = entity->centerPosition;
						}
						
						for(int k = 0; k < gameState->SOLVER_ITERATIONS; k++){
							Vector2 impulse = { 0, 0 }, relativeVelocityOfForceApplicationPoint = {0, 0};
							CalculateAndApplyImpulse(gameState, entity, relevantEntity, collInfo, impulse, relativeVelocityOfForceApplicationPoint, forceApplicationPoint, solverIterationTimeStep);
							HandleFriction(gameState, entity, relevantEntity, collInfo, impulse, relativeVelocityOfForceApplicationPoint, forceApplicationPoint, solverIterationTimeStep);
						}
						if (entity->flags & IN_CONTACT_WITH_GROUND_FLAG) {
							relevantEntity->flags |= IN_CONTACT_WITH_GROUND_FLAG;
						}
					}

				}

				if (!(entity->flags & NON_MOVING_FLAG)) {
					MoveAndRotateEntity(entity, deltaTime);
					CalibrateEntityWithGrid(gameState, entity);
				}
				entity->netForce = { 0, 0 };
				entity->torque = 0;
			}

			if(gameState->chosenPiece != nullptr && !(gameState->chosenPiece->flags & NOT_IN_FREE_FALL_FLAG)) {
				if(magnitude(gameState->chosenPiece->physicsVelocity) < INSIGNIFICANT_NORMAL_VELOCITY_THRESHOLD && 
					abs(gameState->chosenPiece->rotationalVelocity) < INSIGNIFICANT_ANGULAR_VELOCITY_THRESHOLD) 
				{
					if (gameState->chosenPiece->framesWithConsequentialInsignificantMovement == 0) {
						gameState->chosenPiece->positionWhenMovementBecameInsignificant.x = gameState->chosenPiece->centerPosition.x;
						gameState->chosenPiece->positionWhenMovementBecameInsignificant.y = gameState->chosenPiece->centerPosition.y;
						gameState->chosenPiece->angleWhenMovementBecameInsignificant = gameState->chosenPiece->angle;
					}
					else if (gameState->chosenPiece->framesWithConsequentialInsignificantMovement >= SECONDS_FOR_ENTITY_TO_FREEZE * gameState->averageFPS
						&& distance(gameState->chosenPiece->centerPosition, gameState->chosenPiece->positionWhenMovementBecameInsignificant) < INSIGNIFICANT_DISPLACEMENT_THRESHOLD 
						&& abs(gameState->chosenPiece->angle - gameState->chosenPiece->angleWhenMovementBecameInsignificant) < INSIGNIFICANT_ANGULAR_DISPLACEMENT_THRESHOLD
						) 
					{
						gameState->chosenPiece->flags |= NON_MOVING_FLAG;
						gameState->chosenPiece->flags &= ~GRAVITY_FLAG;
						gameState->chosenPiece = nullptr;
						if (gameState->entityBeingCut == nullptr) {
							gameState->readyForNewEntityInitialization = true;
						}
						gameState->gameplayState = CUTTING_OR_CHOOSING_AN_ENTITY;
					}
					if (gameState->chosenPiece != nullptr) {
						gameState->chosenPiece->framesWithConsequentialInsignificantMovement++;
					}
				}
				else {
					if (magnitude(gameState->chosenPiece->physicsVelocity) >= INSIGNIFICANT_NORMAL_VELOCITY_THRESHOLD) {
						int a = 1 + 1;
					}
					if (abs(gameState->chosenPiece->rotationalVelocity) > INSIGNIFICANT_ANGULAR_VELOCITY_THRESHOLD) {
						int a = 1 + 1;
					}
					gameState->chosenPiece->framesWithConsequentialInsignificantMovement = 0;
				}
				
			}
		}

		for (int i = 0; i < gameState->addedEntities; i++) {
			(gameState->entities + i)->gravityApplied = false;
		}

		//we check the gameplay state again because it can be changed before this if statement
		if (gameState->gameplayState == WAITING_FOR_THE_ENTITY_TO_STOP) {
			if (gameState->chosenPiece->centerPosition.y > 1000) {
				ResetChosenEntityAndGameplayStateIntoCuttingOrChoosingState(gameState);
			}
			gameState->framesElapsedWaitingForTheEntityToStop++;
			if (gameState->framesElapsedWaitingForTheEntityToStop > SECONDS_ENTITY_HAS_TO_FREEZE_BEFORE_BEING_RESET * gameState->averageFPS) {
				ResetChosenEntityAndGameplayStateIntoCuttingOrChoosingState(gameState);
			}
		}
		if(gameState->gameplayState != WAITING_FOR_THE_ENTITY_TO_STOP){
			gameState->framesElapsedWaitingForTheEntityToStop = 0;
		}
	}
	else if (gameState->gameplayState == FREE_TIME_OF_ENTITIES) {
		float deltaTime = GetDeltaTime();
		for (int i = 0; i < (gameState->lastEntityOnEntities - gameState->entities) + 1; i++) {
			Entity* entity = gameState->entities + i;
			if (entity->id == 0) {
				continue;
			}
			if (entity->centerPosition.y > 1000) {
				DeleteEntity(gameState, entity);
				continue;
			}
			int numOfRelevantEntities = CalculateRelevantEntitiesForEntity(gameState, entity, relevantEntities, i);
			float solverIterationTimeStep = deltaTime / gameState->SOLVER_ITERATIONS;
			if ((entity->flags & PHYSICS_FLAG)) {
				if (entity->flags & GRAVITY_FLAG) {
					float forcePerVertex = (entity->mass * gameState->gravityConstant) / (entity->vertexDataEnd - entity->vertexData);
					for (int v = 0; v < entity->vertexDataEnd - entity->vertexData; v++) {
						Vector2 vertexPos = entity->centerPosition;
						vertexPos.x += entity->vertexData[v].position.x;
						vertexPos.y += entity->vertexData[v].position.y;
						ApplyForceToEntitiesVelocityImmediately(entity, { 0, forcePerVertex }, deltaTime, vertexPos);
					}
					entity->gravityApplied = true;
				}
				for (int j = 0; j < numOfRelevantEntities; j++) {
					Entity* relevantEntity = relevantEntities[j];
					if ((relevantEntity->flags & PHYSICS_FLAG) == 0) {
						continue;
					}
					CollisionInfo collInfo = DetectCollisionWithEntity(gameState, entity, relevantEntity);
					float totalMass = entity->mass + relevantEntity->mass;

					if (collInfo.minOverlap > FLT_EPSILON) {
						Vector2 forceApplicationPoint = CalculateForceApplicationPoint(gameState, entity, relevantEntity, 0, collInfo);
						if (forceApplicationPoint.x == 0 && forceApplicationPoint.y == 0) {
							forceApplicationPoint = entity->centerPosition;
						}
						for (int k = 0; k < gameState->SOLVER_ITERATIONS; k++) {
							Vector2 impulse = { 0, 0 }, relativeVelocityOfForceApplicationPoint = { 0, 0 };
							CalculateAndApplyImpulse(gameState, entity, relevantEntity, collInfo, impulse, relativeVelocityOfForceApplicationPoint, forceApplicationPoint, solverIterationTimeStep);
							HandleFriction(gameState, entity, relevantEntity, collInfo, impulse, relativeVelocityOfForceApplicationPoint, forceApplicationPoint, solverIterationTimeStep);
						}
						if (entity->flags & IN_CONTACT_WITH_GROUND_FLAG) {
							relevantEntity->flags |= IN_CONTACT_WITH_GROUND_FLAG;
						}
					}
				}
				if (!(entity->flags & NON_MOVING_FLAG)) {
					MoveAndRotateEntity(entity, deltaTime);
					CalibrateEntityWithGrid(gameState, entity);
				}
				entity->netForce = { 0, 0 };
				entity->torque = 0;
			}
		}
		gameState->freeTimeFramesCounter++;
		if (gameState->freeTimeFramesCounter >= gameState->freeTimeLimitInSeconds * gameState->averageFPS) {
			gameState->gameplayState = CUTTING_OR_CHOOSING_AN_ENTITY;
		}


	}
	RetractTemporarySize(gameState, relevantEntitiesSize);
}

void InitializeMainMenu(GameState* gameState) {
	VertexData* buttonData = (VertexData*)PushSize(gameState, sizeof(VertexData) * 4);
	VertexData* buttonDataEnd = buttonData;
	*(buttonDataEnd++) = VertexData{ 60, 50 };
	*(buttonDataEnd++) = VertexData{ 60, -50 };
	*(buttonDataEnd++) = VertexData{ -60, -50 };
	*(buttonDataEnd++) = VertexData{ -60, 50 };
	Entity* playButton = InitializeAndPushEntity(gameState, buttonData, buttonDataEnd, 0, BUTTON_FLAG, { (float)gameState->WINDOW_HEIGHT / 2, (float)gameState->WINDOW_WIDTH / 2 }, MAIN_SCREEN);
	char* text = (char*)PushSize(gameState, sizeof(char) * 5);
	snprintf(text, sizeof(char) * 5, "%s", "PLAY");
	playButton->text = text;
	playButton->flags |= HAS_TEXT;
	playButton->buttonFunction = ChangeScreenTo;
}

void UpdateMainMenu(GameState* gameState, InputInfo inputInfo) {
	uint32_t relevantEntitiesSize = sizeof(Entity*) * 500;
	Entity** relevantEntities = (Entity**)PushTemporarySize(gameState, relevantEntitiesSize);
	if (inputInfo.mouseInputInfo.mouseReleasedThisFrame) {
		//click
		if (inputInfo.mouseInputInfo.inputDurationFrames < gameState->ClickThresholdFrames) {
			Vector2 mousePos = inputInfo.mouseInputInfo.inputPositions[1];
			int numOfCloseEntities = CalculateRelevantEntitiesForPosition(gameState, mousePos, relevantEntities);
			bool selectedAnEntity = false;
			for (int i = 0; i < numOfCloseEntities; i++) {
				Entity* entity = relevantEntities[i];
				bool pointIsInsideEntity = CheckIfAPointIsInsideAnEntity(mousePos, entity, entity->vertexData);
				if (pointIsInsideEntity && (entity->flags & BUTTON_FLAG)) {
					entity->buttonFunction(gameState, GAMEPLAY_SCREEN);
				}
			}
		}
	}
	else {
		if (inputInfo.mouseInputInfo.inputType == LEFT_CLICK) {
			DrawLine(inputInfo.mouseInputInfo.inputPositions[0].x, inputInfo.mouseInputInfo.inputPositions[0].y, inputInfo.mouseInputInfo.inputPositions[1].x, inputInfo.mouseInputInfo.inputPositions[1].y, RED);
		}
	}
	RetractTemporarySize(gameState, relevantEntitiesSize);
}

void RestartGameAndChangeScreenToMainScreen(GameState* gameState) {
	uint64_t highestScore = gameState->highestScore;
	char highestScoreText[50];
	uint64_t frameCount = gameState->frameCount;
	uint16_t averageFPS = gameState->averageFPS;
	std::memcpy(highestScoreText, gameState->highestScoreText, sizeof(char) * 50);
	std::memset(gameState, 0, gameState->arena.capacity + gameState->arena.temporaryCapacity + sizeof(GameState));
	InitializeGameState(gameState);
	InitializeMainMenu(gameState);
	InitializeGameplayScreen(gameState);
	gameState->gameplayScreenInitialized = true;
	ChangeScreenTo(gameState, MAIN_SCREEN);
	gameState->highestScore = highestScore;
	std::memcpy(gameState->highestScoreText, highestScoreText, sizeof(char) * 50);
	gameState->frameCount = frameCount;
	gameState->averageFPS = averageFPS;
}
	
void InitializeEndScreen(GameState* gameState) {
	float floorsCeiling = -FLT_MAX;
	long double score = 0;
	for (int i = 0; i < gameState->floor->vertexDataEnd - gameState->floor->vertexData; i++) {
		float yValueOfVertex = gameState->floor->centerPosition.y + gameState->floor->vertexData[i].position.y;
		if (yValueOfVertex > floorsCeiling) {
			floorsCeiling = yValueOfVertex;
		}
	}

	for (int i = 0; i < gameState->lastEntityOnEntities - gameState->entities; i++) {
		Entity* entity = gameState->entities + i;
		float shapeArea = 0;
		if (entity->id == 0 || entity->screenCode != GAMEPLAY_SCREEN || !(entity->flags & IS_A_BUILDING_BLOCK_FLAG)) {
			continue;
		}
		shapeArea = EntityArea(entity);
		score += sqrtf(pow(entity->centerPosition.y - floorsCeiling, 4) * shapeArea);
	}
	score /= 10000;
	gameState->score = (long unsigned int)score;
	snprintf(gameState->scoreText, sizeof(char) * 50, "%d", gameState->score);
	if (score > gameState->highestScore) {
		gameState->highestScore = score;
		std::memcpy(gameState->highestScoreText, gameState->scoreText, sizeof(char) * 50);
	}

	VertexData* newVertexData = (VertexData*)PushSize(gameState, sizeof(VertexData) * 4);
	std::memcpy(newVertexData, gameState->rectData, sizeof(VertexData) * 4);
	Vector2 playAgainButtonPos = { gameState->WINDOW_WIDTH / 2, gameState->WINDOW_HEIGHT / 2 + 300 };
	Entity* playAgainButton = InitializeAndPushEntity(gameState, newVertexData, newVertexData + 4, 0, BUTTON_FLAG | HAS_TEXT, playAgainButtonPos, END_SCREEN);
	playAgainButton->buttonFunction = &ChangeScreenTo;

	char* playAgainText = (char*)PushSize(gameState, sizeof(char) * 11);
	snprintf(playAgainText, sizeof(char) * 11, "PLAY AGAIN");
	playAgainButton->text = playAgainText;

	for (int i = 0; i < 4; i++) {
		newVertexData[i].position.x *= 2;
	}
}

void UpdateEndScreen(GameState* gameState, InputInfo inputInfo) {
	int fontSize = 60;
	if (gameState->highestScore != 0) {
		float highestScoreTextWidth = MeasureText("Highest Score:", fontSize);
		DrawText("Highest Score:", gameState->WINDOW_WIDTH / 2 - highestScoreTextWidth / 2, gameState->WINDOW_HEIGHT / 2 - fontSize / 2 - 300, fontSize, BLACK);
		float highestScoreWidth = MeasureText(gameState->highestScoreText, fontSize);
		DrawText(gameState->highestScoreText, gameState->WINDOW_WIDTH / 2 - highestScoreWidth / 2, gameState->WINDOW_HEIGHT / 2 - fontSize / 2 - 200, fontSize, BLACK);
	}
	float scoreTextWidth = MeasureText("Score:", fontSize);
	DrawText("Score:", gameState->WINDOW_WIDTH / 2 - scoreTextWidth / 2, gameState->WINDOW_HEIGHT / 2 - fontSize / 2, fontSize, BLACK);
	float scoreWidth = MeasureText(gameState->scoreText, fontSize);
	DrawText(gameState->scoreText, gameState->WINDOW_WIDTH / 2 - scoreWidth / 2, gameState->WINDOW_HEIGHT / 2 - fontSize / 2 + 100, fontSize, BLACK);

	uint32_t relevantEntitiesSize = sizeof(Entity*) * 500;
	Entity** relevantEntities = (Entity**)PushTemporarySize(gameState, relevantEntitiesSize);
	if (inputInfo.mouseInputInfo.mouseReleasedThisFrame) {
		//click
		if (inputInfo.mouseInputInfo.inputDurationFrames < gameState->ClickThresholdFrames) {
			Vector2 mousePos = inputInfo.mouseInputInfo.inputPositions[1];
			int numOfCloseEntities = CalculateRelevantEntitiesForPosition(gameState, mousePos, relevantEntities);
			bool selectedAnEntity = false;
			for (int i = 0; i < numOfCloseEntities; i++) {
				Entity* entity = relevantEntities[i];
				bool pointIsInsideEntity = CheckIfAPointIsInsideAnEntity(mousePos, entity, entity->vertexData);
				if (pointIsInsideEntity && (entity->flags & BUTTON_FLAG)) {
					entity->buttonFunction(gameState, MAIN_SCREEN);
					return;
				}
			}
		}
	}

	RetractTemporarySize(gameState, relevantEntitiesSize);
}
