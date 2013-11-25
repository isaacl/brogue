/*
 *  Architect.c
 *  Brogue
 *
 *  Created by Brian Walker on 1/10/09.
 *  Copyright 2010. All rights reserved.
 *  
 *  This file is part of Brogue.
 *
 *  Brogue is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Brogue is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Brogue.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "Rogue.h"
#include "IncludeGlobals.h"

short topBlobMinX, topBlobMinY, blobWidth, blobHeight;

levelSpecProfile levelSpec;

boolean checkLoopiness(short x, short y) {
	boolean inString;
	short newX, newY, dir, sdir;
	short numStrings, maxStringLength, currentStringLength;
	const short cDirs[8][2] = {{0, 1}, {1, 1}, {1, 0}, {1, -1}, {0, -1}, {-1, -1}, {-1, 0}, {-1, 1}};
	
	if (!(pmap[x][y].flags & IN_LOOP)) {
		return false;
	}
	
	// find a closed neighbor to start on
	for (sdir = 0; sdir < 8; sdir++) {
		newX = x + cDirs[sdir][0];
		newY = y + cDirs[sdir][1];
		if (!coordinatesAreInMap(newX, newY)
			|| !(pmap[newX][newY].flags & IN_LOOP)) {
			break;
		}
	}
	if (sdir == 8) { // no unloopy neighbors
		return false; // leave cell loopy
	}
	
	// starting on this unloopy neighbor, work clockwise and count up (a) the number of strings
	// of loopy neighbors, and (b) the length of the longest such string.
	numStrings = maxStringLength = currentStringLength = 0;
	inString = false;
	for (dir = sdir; dir < sdir + 8; dir++) {
		newX = x + cDirs[dir % 8][0];
		newY = y + cDirs[dir % 8][1];
		if (coordinatesAreInMap(newX, newY) && (pmap[newX][newY].flags & IN_LOOP)) {
			currentStringLength++;
			if (!inString) {
				if (numStrings > 0) {
					return false; // more than one string here; leave loopy
				}
				numStrings++;
				inString = true;
			}
		} else if (inString) {
			if (currentStringLength > maxStringLength) {
				maxStringLength = currentStringLength;
			}
			currentStringLength = 0;
			inString = false;
		}
	}
	if (numStrings == 1 && maxStringLength <= 3) {
		pmap[x][y].flags &= ~IN_LOOP;
		
		for (dir = 0; dir < 8; dir++) {
			newX = x + cDirs[dir][0];
			newY = y + cDirs[dir][1];
			if (coordinatesAreInMap(newX, newY)) {
				//checkLoopiness(newX, newY);
			}
		}
		return true;
	} else {
		return false;
	}
}

void findLoops() {
	short i, j;
	boolean madeChange;
	
	for(i=0; i<DCOLS; i++) {
		for(j=0; j<DROWS; j++) {	
			if (cellHasTerrainFlag(i, j, PATHING_BLOCKER)
				&& !cellHasTerrainFlag(i, j, IS_SECRET)) {
				pmap[i][j].flags &= ~IN_LOOP;
			} else {
				pmap[i][j].flags |= IN_LOOP;
			}
		}
	}
	
	do {
		madeChange = false;
		
		for(i=0; i<DCOLS; i++) {
			for(j=0; j<DROWS; j++) {
				if (checkLoopiness(i, j)) {
					madeChange = true;
//					displayLevel();
//					displayLoops();
//					pauseBrogue(1);
				}
			}
		}
	} while (madeChange);
}

void digDungeon() {
	short i, j, k, l;
	short x1, x2, y1, y2;
	short roomWidth, roomHeight, roomX, roomY;
	short randIndex, doorCandidateX, doorCandidateY;
	short direction;
	short numLoops;
	short lakeMaxWidth, lakeMaxHeight;
	short surfaceEffectOrigin[2];
	short depthRoomSizeScaleFactor;
	short deepLiquid, shallowLiquid, shallowLiquidWidth;
	char wreathMap[DCOLS][DROWS];
	boolean isCorridor = false;
	boolean isCross = false;
	boolean roomWasPlaced = true;
	boolean diagonalCornerRemoved;
	boolean foundExposure;
	room *builtRoom = NULL, *fromRoom = NULL, *seedRoom;
	
	rogue.scentTurnNumber = 10000;
	
	for ( i=0; i<4; i++) {
		numberOfWalls[i] = -1;
	}
	
	// Clear level and fill with granite
	for( i=0; i<DCOLS; i++ ) {
		for( j=0; j<DROWS; j++ ) {	
			pmap[i][j].layers[DUNGEON] = GRANITE;
			pmap[i][j].layers[LIQUID] = NOTHING;
			pmap[i][j].layers[GAS] = NOTHING;
			pmap[i][j].layers[SURFACE] = NOTHING;
			pmap[i][j].rememberedTerrain = NOTHING;
			pmap[i][j].rememberedItemCategory = 0;
			pmap[i][j].flags = 0;
			tmap[i][j].scent = 0;
			pmap[i][j].volume = 0;
		}
	}
	
	//depthRoomSizeScaleFactor = 1000 - 500 * (min(rogue.depthLevel - 1, 25)) / 25;
	depthRoomSizeScaleFactor = 1000;
	levelSpec.roomMinWidth = max(MIN_SCALED_ROOM_DIMENSION, ROOM_MIN_WIDTH * depthRoomSizeScaleFactor / 1000);
	levelSpec.roomMaxWidth = max(MIN_SCALED_ROOM_DIMENSION, ROOM_MAX_WIDTH * depthRoomSizeScaleFactor / 1000);
	levelSpec.roomMinHeight = max(MIN_SCALED_ROOM_DIMENSION, ROOM_MIN_HEIGHT * depthRoomSizeScaleFactor / 1000);
	levelSpec.roomMaxHeight = max(MIN_SCALED_ROOM_DIMENSION, ROOM_MAX_HEIGHT * depthRoomSizeScaleFactor / 1000);
	levelSpec.horCorrMinLength = max(MIN_SCALED_ROOM_DIMENSION, HORIZONTAL_CORRIDOR_MIN_LENGTH * depthRoomSizeScaleFactor / 1000);
	levelSpec.horCorrMaxLength = max(MIN_SCALED_ROOM_DIMENSION, HORIZONTAL_CORRIDOR_MAX_LENGTH * depthRoomSizeScaleFactor / 1000);
	levelSpec.vertCorrMinLength = max(MIN_SCALED_ROOM_DIMENSION, VERTICAL_CORRIDOR_MIN_LENGTH * depthRoomSizeScaleFactor / 1000);
	levelSpec.vertCorrMaxLength = max(MIN_SCALED_ROOM_DIMENSION, VERTICAL_CORRIDOR_MAX_LENGTH * depthRoomSizeScaleFactor / 1000);
	levelSpec.crossRoomMinWidth = max(MIN_SCALED_ROOM_DIMENSION, CROSS_ROOM_MIN_WIDTH * depthRoomSizeScaleFactor / 1000);
	levelSpec.crossRoomMaxWidth = max(MIN_SCALED_ROOM_DIMENSION, CROSS_ROOM_MAX_WIDTH * depthRoomSizeScaleFactor / 1000);
	levelSpec.crossRoomMinHeight = max(MIN_SCALED_ROOM_DIMENSION, CROSS_ROOM_MIN_HEIGHT * depthRoomSizeScaleFactor / 1000);
	levelSpec.crossRoomMaxHeight = max(MIN_SCALED_ROOM_DIMENSION, CROSS_ROOM_MAX_HEIGHT * depthRoomSizeScaleFactor / 1000);
	levelSpec.secretDoorChance = max(0, min(67, (rogue.depthLevel - 1) * 67 / 25));
	levelSpec.numberOfTraps = rand_range((rogue.depthLevel - 1) / 4, (rogue.depthLevel - 1) / 2);
	
	// Build a rectangular seed room or cave at a random place
	if (rand_percent(thisLevelProfile->caveLevelChance)) {
		generateCave();
	} else {		
		roomWidth = rand_range(4, 25);
		roomHeight = rand_range(2, 7);
		roomX = rand_range(1, DCOLS - roomWidth - 1);
		roomY = rand_range(1, DROWS - roomHeight - 1);
		carveRectangle(roomX, roomY, roomWidth, roomHeight);
		markRectangle(roomX, roomY, roomWidth, roomHeight);
		seedRoom = allocateRoom(roomX, roomY, roomWidth, roomHeight, 0, 0, 0, 0);
	}
	
	numberOfRooms = 1;
	
	// Now start placing random rooms branching off of preexisting rooms; make 600 attempts
	
	for(k=0; k<600 && numberOfRooms <= thisLevelProfile->maxNumberOfRooms; k++) {
		
		direction = rand_range(0,3); // Are we going to try to branch up, down, left, or right? Pick randomly.
		
		// Is it a corridor? Harder to place corridors, since you need space for a room at the end,
		// so the last 225 attempts are hard-wired not to be corridors so that they can fill in any
		// big gaps left by the corridor requirement.
		isCorridor = (( k > 375) ? false : (rand_percent(thisLevelProfile->corridorChance))); // boolean: is it a corridor?
		
		if (roomWasPlaced) { // We only randomize the crossRoom status if the previous room succeeded
			if (!isCorridor) {
				isCross = (rand_percent(thisLevelProfile->crossRoomChance));
			}
		}
		
		// Now choose a random wall cell of the given direction
		randIndex = rand_range(0, numberOfWalls[direction]);
		doorCandidateX = listOfWallsX[direction][randIndex];
		doorCandidateY = listOfWallsY[direction][randIndex];
		
		builtRoom = attemptRoom(doorCandidateX, doorCandidateY, direction, isCorridor, isCross, (isCorridor ? 10 : 5));
		// note: if it is a corridor, this call will also build a
		// non-corridor room at the end of the corridor, or it will fail.
		
		roomWasPlaced = (builtRoom != 0);
		
		if (builtRoom) {
			fromRoom = roomContainingCell(doorCandidateX + nbDirs[oppositeDirection(direction)][0],
										  doorCandidateY + nbDirs[oppositeDirection(direction)][1]);
			connectRooms(fromRoom, builtRoom, doorCandidateX, doorCandidateY, direction);
		}
	}
	//DEBUG printf("%i non-corridor rooms placed.", numberOfRooms);
	
	// now we add some loops to the otherwise simply connected network of rooms
	numLoops = 0;
	for (k=0; k<500 && numLoops <= thisLevelProfile->maxNumberOfLoops; k++) {
		
		direction = rand_range(0,3); // Are we going to try to loop up, down, left, or right? Pick randomly.
		
		// Now choose a random wall cell of the given direction
		randIndex = rand_range(0, numberOfWalls[direction]);
		doorCandidateX = listOfWallsX[direction][randIndex];
		doorCandidateY = listOfWallsY[direction][randIndex];
		
		x1 = doorCandidateX + nbDirs[direction][0];
		x2 = doorCandidateX + nbDirs[oppositeDirection(direction)][0];
		y1 = doorCandidateY + nbDirs[direction][1];
		y2 = doorCandidateY + nbDirs[oppositeDirection(direction)][1];
		
		if (coordinatesAreInMap(x1, y1) && coordinatesAreInMap(x2, y2)
			&& pmap[x1][y1].layers[DUNGEON] == FLOOR && pmap[x2][y2].layers[DUNGEON] == FLOOR &&
			distanceBetweenRooms(roomContainingCell(x1, y1), roomContainingCell(x2, y2)) > 2) {
			pmap[doorCandidateX][doorCandidateY].layers[DUNGEON] = (rand_percent(thisLevelProfile->doorChance) ? DOOR : FLOOR);
			pmap[doorCandidateX][doorCandidateY].flags |= DOORWAY;
			removeWallFromList(direction, doorCandidateX, doorCandidateY);
			connectRooms(roomContainingCell(x2, y2), roomContainingCell(x1, y1), doorCandidateX, doorCandidateY, direction);
			numLoops++;
		}
	}
	//DEBUG printf("\n%i loops created.", numLoops);
	//DEBUG logLevel();
	
	// Time to add lakes and chasms. Strategy is to generate a series of blob lakes of decreasing size. For each lake,
	// propose a position, and then check via a flood fill that the level would remain connected with that placement (i.e. that
	// each passable tile can still be reached). If not, make 9 more placement attempts before abandoning that lake
	// and proceeding to generate the next smaller one.
	// Canvas sizes start at 30x15 and decrease by 2x1 at a time down to a minimum of 20x10. Min generated size is always 4x4.
	
	// DEBUG logLevel();
	
	for (lakeMaxHeight = 15, lakeMaxWidth = 30; lakeMaxHeight >=10; lakeMaxHeight--, lakeMaxWidth -= 2) { // lake generations
		
		cellularAutomata(4, 4, lakeMaxWidth, lakeMaxHeight, 55, "ffffftttt", "ffffttttt");
		
		for (k=0; k<20; k++) { // placement attempts
			
			// propose a position
			roomX = rand_range(1, DCOLS - blobWidth - 1);
			roomY = rand_range(1, DROWS - blobHeight - 1);
			
			if (checkLakePassability(roomX, roomY)) { // level with lake is completely connected
				//printf("Placed a lake!");
				
				// copy in lake
				for (i = roomX; i < roomX + blobWidth; i++) {
					for (j = roomY; j < roomY + blobHeight; j++) {
						if (tmap[i][j].connected == -1) {
							pmap[i][j].layers[LIQUID] = UNFILLED_LAKE;
							pmap[i][j].layers[DUNGEON] = FLOOR;
							if (pmap[i][j].flags & DOORWAY) {
								pmap[i][j].flags &= ~DOORWAY; // so that waypoint selection is not blocked by former doorways
							}
							for (l=0; l<8; l++) {
								if (coordinatesAreInMap(i+nbDirs[l][0], j+nbDirs[l][1]) &&
									pmap[i+nbDirs[l][0]][j+nbDirs[l][1]].layers[DUNGEON] == GRANITE) {
									pmap[i+nbDirs[l][0]][j+nbDirs[l][1]].layers[DUNGEON] = PERM_WALL;
								}
							}
						}
					}
				}
				
				// DEBUG logLevel();
				break;
			}
		}
	}
	
	// Now fill all the lakes with various liquids
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			if (pmap[i][j].layers[LIQUID] == UNFILLED_LAKE) {
				liquidType(&deepLiquid, &shallowLiquid, &shallowLiquidWidth);
				zeroOutGrid(wreathMap);
				fillLake(i, j, deepLiquid, 4, wreathMap);
				createWreath(shallowLiquid, shallowLiquidWidth, wreathMap);
			}
		}
	}
	
	// Now replace all left, right and bottom walls with TOP_WALLS for homogeneity
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			if (pmap[i][j].layers[DUNGEON] == LEFT_WALL
				|| pmap[i][j].layers[DUNGEON] == RIGHT_WALL
				|| pmap[i][j].layers[DUNGEON] == BOTTOM_WALL) {
				pmap[i][j].layers[DUNGEON] = TOP_WALL;
			}
		}
	}
	
	// Now add some dungeon features
	for (k=1; k<NUMBER_DUNGEON_FEATURES; k++) {
		if (rogue.depthLevel < dungeonFeatureCatalog[k].minDepth || rogue.depthLevel > dungeonFeatureCatalog[k].maxDepth) {
			continue;
		}
		l = min((dungeonFeatureCatalog[k].minNumberIntercept + rogue.depthLevel * dungeonFeatureCatalog[k].minNumberSlope) / 100,
				dungeonFeatureCatalog[k].maxNumber);
		for (i = 0; i < l || rand_percent(dungeonFeatureCatalog[k].frequency); i++) {
			//if (randomMatchingLocation(surfaceEffectOrigin, dungeonFeatureCatalog[k].requiredDungeonFoundationType, NOTHING, -1)) {
			if (randomMatchingLocation(surfaceEffectOrigin, dungeonFeatureCatalog[k].requiredDungeonFoundationType, NOTHING, -1)) {
				spawnDungeonFeature(surfaceEffectOrigin[0], surfaceEffectOrigin[1], &dungeonFeatureCatalog[k], false);				
			}
		}
	}
	
	// Now remove diagonal openings
	do {
		diagonalCornerRemoved = false;
		for (i=0; i<DCOLS-1; i++) {
			for (j=0; j<DROWS-1; j++) {
				for (k=0; k<=1; k++) {
					if (!(tileCatalog[pmap[i + k][j].layers[DUNGEON]].flags & OBSTRUCTS_PASSABILITY)
						&& (tileCatalog[pmap[i + (1-k)][j].layers[DUNGEON]].flags & OBSTRUCTS_PASSABILITY)
						&& (tileCatalog[pmap[i + k][j+1].layers[DUNGEON]].flags & OBSTRUCTS_PASSABILITY)
						&& !(tileCatalog[pmap[i + (1-k)][j+1].layers[DUNGEON]].flags & OBSTRUCTS_PASSABILITY)) {
						diagonalCornerRemoved = true;
						if (rand_percent(50)) {
							x1 = i + (1-k);
							x2 = i + k;
							y1 = j;
						} else {
							x1 = i + k;
							x2 = i + (1-k);
							y1 = j + 1;
						}
						if (tileCatalog[pmap[x1][y1].layers[DUNGEON]].flags & OBSTRUCTS_PASSABILITY) {
							pmap[x1][y1].layers[LIQUID] = pmap[x2][y1].layers[LIQUID];
						}
						pmap[x1][y1].layers[DUNGEON] = FLOOR;
					}
				}
			}
		}
	} while (diagonalCornerRemoved == true);
	
	// Now add some bridges
	while (buildABridge());
	
	// Now remove orphaned doors and upgrade some doors to secret doors
	for (i=1; i<DCOLS-1; i++) {
		for (j=1; j<DROWS-1; j++) {
			if (pmap[i][j].layers[DUNGEON] == DOOR) {
				// if there's a floor to the left or right, and there's a floor above or below, then the door
				// is orphaned and must die.
				if ((pmap[i+1][j].layers[DUNGEON] == FLOOR || pmap[i-1][j].layers[DUNGEON] == FLOOR)
					&& (pmap[i][j+1].layers[DUNGEON] == FLOOR || pmap[i][j-1].layers[DUNGEON] == FLOOR)) {
					pmap[i][j].layers[DUNGEON] = FLOOR;
				} else if (rand_percent(levelSpec.secretDoorChance)) {
					pmap[i][j].layers[DUNGEON] = SECRET_DOOR;
				}
			}
		}
	}
	
	// Now finish any exposed granite with walls and revert any unexposed walls to granite
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			if (pmap[i][j].layers[DUNGEON] == GRANITE) {
				foundExposure = false;
				for (direction = 0; direction < 8 && !foundExposure; direction++) {
					x1 = i + nbDirs[direction][0];
					y1 = j + nbDirs[direction][1];
					if (coordinatesAreInMap(x1, y1)
						&& (!cellHasTerrainFlag(x1, y1, OBSTRUCTS_VISION) || !cellHasTerrainFlag(x1, y1, OBSTRUCTS_PASSABILITY))) {
						pmap[i][j].layers[DUNGEON] = TOP_WALL;
						foundExposure = true;
					}
				}
			} else if (pmap[i][j].layers[DUNGEON] == TOP_WALL || pmap[i][j].layers[DUNGEON] == PERM_WALL) {
				foundExposure = false;
				for (direction = 0; direction < 8 && !foundExposure; direction++) {
					x1 = i + nbDirs[direction][0];
					y1 = j + nbDirs[direction][1];
					if (coordinatesAreInMap(x1, y1)
						&& (!cellHasTerrainFlag(x1, y1, OBSTRUCTS_VISION) || !cellHasTerrainFlag(x1, y1, OBSTRUCTS_PASSABILITY))) {
						foundExposure = true;
					}
				}
				if (foundExposure == false) {
					pmap[i][j].layers[DUNGEON] = GRANITE;
				}
			}
		}
	}
	//findLoops();
}

// Scans from the top-left to the bottom-right looking for a good place to build a bridge.
// If it finds one, it builds a bridge there, halts and returns true.
boolean buildABridge() {
	short i, j, k, l;
	boolean foundExposure;
	
	for (i=1; i<DCOLS-1; i++) {
		for (j=1; j<DROWS-1; j++) {
			if (!cellHasTerrainFlag(i, j, (CAN_BE_BRIDGED | OBSTRUCTS_PASSABILITY))
				&& (pmap[i][j].layers[LIQUID] != BRIDGE || rand_percent(15))) {
				
				// try a horizontal bridge
				foundExposure = false;
				for (k = i + 1;
					 k < DCOLS
					 && cellHasTerrainFlag(k, j, CAN_BE_BRIDGED)
					 && !cellHasTerrainFlag(k, j, OBSTRUCTS_PASSABILITY)
					 && cellHasTerrainFlag(k, j-1, (CAN_BE_BRIDGED | OBSTRUCTS_PASSABILITY))
					 && cellHasTerrainFlag(k, j+1, (CAN_BE_BRIDGED | OBSTRUCTS_PASSABILITY));
					 k++) {
					if (!cellHasTerrainFlag(k, j-1, OBSTRUCTS_PASSABILITY)
						&& !cellHasTerrainFlag(k, j+1, OBSTRUCTS_PASSABILITY)) {
						foundExposure = true;
					}
				}
				if (k < DCOLS
					&& (k - i > 3)
					&& foundExposure
					&& !cellHasTerrainFlag(k, j, OBSTRUCTS_PASSABILITY | CAN_BE_BRIDGED)
					&& pathingDistance(i, j, k, j, PATHING_BLOCKER) / (k - i) > 2) {
					for (l=i+1; l < k; l++) {
						pmap[l][j].layers[LIQUID] = BRIDGE;
					}
					pmap[i][j].layers[SURFACE] = BRIDGE_EDGE;
					pmap[k][j].layers[SURFACE] = BRIDGE_EDGE;
					return true;
				}
				
				// try a vertical bridge
				foundExposure = false;
				for (k = j + 1;
					 k < DROWS
					 && cellHasTerrainFlag(i, k, CAN_BE_BRIDGED)
					 && !cellHasTerrainFlag(i, k, OBSTRUCTS_PASSABILITY)
					 && cellHasTerrainFlag(i-1, k, (CAN_BE_BRIDGED | OBSTRUCTS_PASSABILITY))
					 && cellHasTerrainFlag(i+1, k, (CAN_BE_BRIDGED | OBSTRUCTS_PASSABILITY));
					 k++) {
					if (!cellHasTerrainFlag(i-1, k, OBSTRUCTS_PASSABILITY)
						&& !cellHasTerrainFlag(i+1, k, OBSTRUCTS_PASSABILITY)) {
						foundExposure = true;
					}
				}
				if (k < DROWS
					&& (k - j > 3)
					&& foundExposure
					&& !cellHasTerrainFlag(i, k, OBSTRUCTS_PASSABILITY | CAN_BE_BRIDGED)
					&& pathingDistance(i, j, i, k, PATHING_BLOCKER) / (k - j) > 5) {
					for (l=j+1; l < k; l++) {
						pmap[i][l].layers[LIQUID] = BRIDGE;
					}
					pmap[i][j].layers[SURFACE] = BRIDGE_EDGE;
					pmap[i][k].layers[SURFACE] = BRIDGE_EDGE;
					return true;
				}
			}
		}
	}
	return false;
}

void updateMapToShore() {
	short i, j;
	boolean passMap[DCOLS][DROWS];
	
	// Calculate the map to shore for this level
	if (rogue.mapToShore) {
		//freeDynamicGrid(rogue.mapToShore);
	} else {
		rogue.mapToShore = allocDynamicGrid();
	}
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			if (cellHasTerrainFlag(i, j, OBSTRUCTS_PASSABILITY)) {
				passMap[i][j] = false;
				rogue.mapToShore[i][j] = 30000;
			} else {
				passMap[i][j] = true;
				rogue.mapToShore[i][j] = (cellHasTerrainFlag(i, j, LAVA_INSTA_DEATH | IS_DEEP_WATER | TRAP_DESCENT)
										  && !cellHasTerrainFlag(i, j, IS_SECRET)) ? 30000 : 0;
			}
		}
	}
	dijkstraScan(rogue.mapToShore, NULL, passMap, true);
}

void augmentAccessMapWithWaypoint(char accessMap[DCOLS][DROWS], boolean WPactive[MAX_WAYPOINTS], short x, short y) {
	short i, j;
	char grid[DCOLS][DROWS];
	
	zeroOutGrid(grid);
	getFOVMask(grid, x, y, WAYPOINT_SIGHT_RADIUS, WAYPOINT_BLOCKER, DOORWAY, false);
	
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			if (grid[i][j] && accessMap[i][j] != 3) { // i.e. if it's in the FOV mask and is not a wall/trap/lava/water
				accessMap[i][j] = 1;
			}
		}
	}
	accessMap[x][y] = 1;
	
	for (i = 0; i < numberOfWaypoints; i++) {
		if (!WPactive[i] && (accessMap[waypoints[i].x][waypoints[i].y] == 1 || accessMap[waypoints[i].x][waypoints[i].y] == 2)) {
			WPactive[i] = true;
			if (waypoints[i].pointsTo[0] && waypoints[i].pointsTo[1]) {
				accessMap[waypoints[i].pointsTo[0]][waypoints[i].pointsTo[1]] = 1;
			}
			augmentAccessMapWithWaypoint(accessMap, WPactive, waypoints[i].x, waypoints[i].y); // recurse!
		}
	}
}

short findEdges(char accessMap[DCOLS][DROWS], char centralityMap[DCOLS][DROWS]) {
	short i, j, count = 0;
	enum directions dir;
	
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			if (accessMap[i][j] == 2) {
				count += centralityMap[i][j];
			} else if (accessMap[i][j] == 1) {
				for (dir = 0; dir < 4; dir++) {
					if (accessMap[i + nbDirs[dir][0]][j + nbDirs[dir][1]] == 0) {
						accessMap[i][j] = 2;
						count += centralityMap[i][j];
						break;
					}
				}
			}
		}
	}
	return count;
}

boolean waypointLinkedTo(short i, short j) {
	short k;
	if (waypoints[i].pointsTo[0] == waypoints[j].x && waypoints[i].pointsTo[1] == waypoints[j].y) {
		return true;
	}
	for (k = 0; k < waypoints[i].connectionCount; k++) {
		if (waypoints[i].connection[k][0] == waypoints[j].x && waypoints[i].connection[k][1] == waypoints[j].y) {
			return true;
		}
	}
	return false;
}

void setUpWaypoints() {
	short i, j, k, x, y, edgesWeightCount, randIndex;
	room *theRoom;
	char accessMap[DCOLS][DROWS], centralityMap[DCOLS][DROWS], grid[DCOLS][DROWS];
	boolean WPactive[MAX_WAYPOINTS];
	
	numberOfWaypoints = 0;
	
	for (i = 0; i < MAX_WAYPOINTS; i++) {
		waypoints[i].connectionCount = 0;
		waypoints[i].pointsTo[0] = 0;
		waypoints[i].pointsTo[1] = 0;
		WPactive[i] = false;
	}
	
	// 1. generate the waypoints.
	
	// initialize the accessMap
	
	// set up access map
	// key: 0 = passable but not accessible
	//		1 = accessible
	//		2 = accessible and adjacent to type 0
	//		3 = wall, pit, lava, deep water, pit trap
	
	zeroOutGrid(accessMap);
	
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			if (cellHasTerrainFlag(i, j, WAYPOINT_BLOCKER) || (pmap[i][j].flags & DOORWAY)) {
				accessMap[i][j] = 3;
			}
		}
	}
	
	// DEBUG logBuffer(accessMap);
	
	// Set up centrality map, to be used as probability weights for picking waypoint candidates.
	// The idea here is that we want a weight map of how many cells are visible from a particular cell.
	// Could just do a FOV from each cell and add it all up, but that would be very expensive.
	// Instead, randomly choose N passable points from around the map, compute FOV for them,
	// and add all the FOV maps. So it's really a map of how many of these random points are visible
	// from any particular cell -- a monte carlo equivalent which proportionately approaches the right answer
	// as N->infty.
	
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			centralityMap[i][j] = 1; // starting it at 0 makes the generation prone to failure if unlucky
		}
	}
	for (k=0; k<200; k++) {
		x = rand_range(1, DCOLS - 1);
		y = rand_range(1, DROWS - 1);
		if (accessMap[x][y] == 3) { // i.e. passable
			zeroOutGrid(grid);
			getFOVMask(grid, x, y, WAYPOINT_SIGHT_RADIUS, WAYPOINT_BLOCKER, DOORWAY, false);
			for (i=1; i<DCOLS - 1; i++) {
				for (j=1; j<DROWS - 1; j++) {
					if (grid[i][j]) {
						centralityMap[i][j]++;
					}
				}
			}
		}
	}
	
	// DEBUG logBuffer(accessMap);
	
	//	a. place them on either side of doors and make them point to each other.
	
	// cycle through the rooms
	for (theRoom = rooms->nextRoom; theRoom != NULL; theRoom = theRoom->nextRoom) {
		// cycle through the doors of the room
		for (i = 0; i < theRoom->numberOfSiblings; i++) {
			
			// waypoint is immediately inside the door
			waypoints[numberOfWaypoints].x = theRoom->doors[i].x +
			nbDirs[oppositeDirection(theRoom->doors[i].direction)][0];
			waypoints[numberOfWaypoints].y = theRoom->doors[i].y +
			nbDirs[oppositeDirection(theRoom->doors[i].direction)][1];
			
			//augmentAccessMapWithWaypoint(accessMap, WPactive, waypoints[numberOfWaypoints].x, waypoints[numberOfWaypoints].y);
			
			// DEBUG logBuffer(accessMap);
			
			// waypoint points to the cell immediately on the other side of the door
			waypoints[numberOfWaypoints].pointsTo[0] = theRoom->doors[i].x + nbDirs[theRoom->doors[i].direction][0];
			waypoints[numberOfWaypoints].pointsTo[1] = theRoom->doors[i].y + nbDirs[theRoom->doors[i].direction][1];
			
			// link waypoint to the other waypoints of the room, if they're not too far apart
			waypoints[numberOfWaypoints].connectionCount = 0;
			for (j = 0; j < theRoom->numberOfSiblings; j++) {
				if (i != j // don't want to link the waypoint to itself
					&& (distanceBetween(waypoints[numberOfWaypoints].x, waypoints[numberOfWaypoints].y,
											  theRoom->doors[j].x, theRoom->doors[j].y) < 21
						|| specifiedPathBetween(waypoints[numberOfWaypoints].x, waypoints[numberOfWaypoints].y,
												theRoom->doors[j].x, theRoom->doors[j].y, WAYPOINT_BLOCKER, DOORWAY))
					&& theRoom->width <= 20) { // not a cave
					waypoints[numberOfWaypoints].connection[waypoints[numberOfWaypoints].connectionCount][0]
					= theRoom->doors[j].x +	nbDirs[oppositeDirection(theRoom->doors[j].direction)][0];
					
					waypoints[numberOfWaypoints].connection[waypoints[numberOfWaypoints].connectionCount][1]
					= theRoom->doors[j].y +	nbDirs[oppositeDirection(theRoom->doors[j].direction)][1];
					
					waypoints[numberOfWaypoints].connectionCount++;
				}
			}
			
			numberOfWaypoints++;
		}
	}
	
	// b. Place them near anywhere that is not within line of sight of a waypoint. This will cover caves and also certain
	// rooms in which there is an alcove not visible from the doors.
	
	// Activate the first waypoint! This process of activation is to ensure that all the WP's are connected.
	WPactive[0] = true;
	accessMap[waypoints[0].pointsTo[0]][waypoints[0].pointsTo[1]] = 1;
	augmentAccessMapWithWaypoint(accessMap, WPactive, waypoints[0].x, waypoints[0].y);
	
	edgesWeightCount = findEdges(accessMap, centralityMap);
	// DEBUG logBuffer(accessMap);
	while (edgesWeightCount) {
		randIndex = rand_range(1, edgesWeightCount);
		
		for (i=0; i<DCOLS; i++)	{
			for (j=0; j<DROWS; j++) {
				if (accessMap[i][j] == 2) {
					randIndex -= centralityMap[i][j];
					if (randIndex <= 0) {
						// this is the edge we've chosen. Add a waypoint.
						augmentAccessMapWithWaypoint(accessMap, WPactive, i, j);
						
						waypoints[numberOfWaypoints].x = i;
						waypoints[numberOfWaypoints].y = j;
						numberOfWaypoints++;
						
						// crappy way to break out of both loops:
						i = DCOLS;
						j = DROWS;
					}
				}
			}
		}
		
		edgesWeightCount = findEdges(accessMap, centralityMap);
		// DEBUG logBuffer(accessMap);
		// DEBUG printf("\n%i waypoints in the above map\n", numberOfWaypoints);
	}
	
	// 2. link the waypoints together.
	
	// i is the waypoint we're connecting
	for (i=0; i < numberOfWaypoints; i++) {
		x = waypoints[i].x;
		y = waypoints[i].y;
		
		zeroOutGrid(grid);
		getFOVMask(grid, x, y, WAYPOINT_SIGHT_RADIUS, WAYPOINT_BLOCKER, DOORWAY, false);
		
		// j is the waypoint to which we're contemplating linking it
		for (j=0; j < numberOfWaypoints && waypoints[i].connectionCount <= 10; j++) {
			if (i == j) {
				continue; // don't link to itself
			}
			
			// verify that the waypoint isn't already connected to the target
			if (!waypointLinkedTo(i, j)) {
				// check to make sure the path between is passable
				if (grid[waypoints[j].x][waypoints[j].y]) {
					waypoints[i].connection[waypoints[i].connectionCount][0] = waypoints[j].x;
					waypoints[i].connection[waypoints[i].connectionCount][1] = waypoints[j].y;					
					waypoints[i].connectionCount++;
					
					// create reciprocal link if it doesn't already exist (important to avoid one-way links)
					if (!waypointLinkedTo(j, i)) {
						waypoints[j].connection[waypoints[j].connectionCount][0] = waypoints[i].x;
						waypoints[j].connection[waypoints[j].connectionCount][1] = waypoints[i].y;					
						waypoints[j].connectionCount++;
					}
				}
			}
		}
	}
}

void zeroOutGrid(char grid[DCOLS][DROWS]) {
	short i, j;
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			grid[i][j] = 0;
		}
	}
}

void createWreath(short shallowLiquid, short wreathWidth, char wreathMap[DCOLS][DROWS]) {
	short i, j, k, l;
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			if (wreathMap[i][j]) {
				for (k = i-wreathWidth; k<= i+wreathWidth; k++) {
					for (l = j-wreathWidth; l <= j+wreathWidth; l++) {
						if (coordinatesAreInMap(k, l) && pmap[k][l].layers[LIQUID] == NOTHING &&
							(i-k)*(i-k) + (j-l)*(j-l) <= wreathWidth*wreathWidth) {
							pmap[k][l].layers[LIQUID] = shallowLiquid;
							if (pmap[k][l].layers[DUNGEON] == DOOR) {
								pmap[k][l].layers[DUNGEON] = FLOOR;
							}
						}
					}
				}
			}
		}
	}
}

short oppositeDirection(short theDir) {
	switch (theDir) {
	case UP:
		return DOWN;
	case DOWN:
		return UP;
	case LEFT:
		return RIGHT;
	case RIGHT:
		return LEFT;
	case UPRIGHT:
		return DOWNLEFT;
	case DOWNLEFT:
		return UPRIGHT;
	case UPLEFT:
		return DOWNRIGHT;
	case DOWNRIGHT:
		return UPLEFT;
	default:
		return -1;
	}
}

void connectRooms(room *fromRoom, room *toRoom, short x, short y, short direction) {
	short f = fromRoom->numberOfSiblings, t = toRoom->numberOfSiblings;
	fromRoom->doors[f].x = x;
	toRoom->doors[t].x = x;
	fromRoom->doors[f].y = y;
	toRoom->doors[t].y = y;
	fromRoom->doors[f].direction = direction;
	toRoom->doors[t].direction = oppositeDirection(direction);
	fromRoom->siblingRooms[f] = toRoom;
	toRoom->siblingRooms[t] = fromRoom;
	fromRoom->numberOfSiblings++;
	toRoom->numberOfSiblings++;
}

room *allocateRoom(short roomX, short roomY, short width, short height,
				   short roomX2, short roomY2, short width2, short height2) {
	room *theRoom;
	theRoom = (room *) malloc( sizeof(room) );
	theRoom->nextRoom = rooms->nextRoom;
	rooms->nextRoom = theRoom;
	theRoom->numberOfSiblings = 0;
	theRoom->roomX = roomX;
	theRoom->roomY = roomY;
	theRoom->width = width;
	theRoom->height = height;
	theRoom->roomX2 = roomX2;
	theRoom->roomY2 = roomY2;
	theRoom->width2 = width2;
	theRoom->height2 = height2;
	return theRoom;
}

room *roomContainingCell(short x, short y) {
	room *theRoom;
	short i;
	for (theRoom = rooms->nextRoom; theRoom != NULL; theRoom = theRoom->nextRoom) {
		if ((theRoom->roomX <= x && theRoom->roomX + theRoom->width > x &&
			 theRoom->roomY <= y && theRoom->roomY + theRoom->height > y) ||
			(theRoom->roomX2 <= x && theRoom->roomX2 + theRoom->width2 > x &&
			 theRoom->roomY2 <= y && theRoom->roomY2 + theRoom->height2 > y)) {
			return theRoom;
		}
		for (i=0; i < theRoom->numberOfSiblings; i++) {
			if (theRoom->doors[i].x == x && theRoom->doors[i].y == y) {
				return theRoom;
			}
		}
	}
	return NULL;
}

// Tries to build a room starting at a door with the specified location and direction and room type.
room *attemptRoom(short doorCandidateX, short doorCandidateY,
					short direction, boolean isCorridor, boolean isCross, short numAttempts) {
	
	// roomX2, roomY2, roomWidth2 and roomHeight2 are for cross-shaped rooms
	short i, roomX, roomY, roomWidth, roomHeight, roomX2, roomY2, roomWidth2, roomHeight2, newDoorX, newDoorY;
	
	room *builtRoom, *nextRoom;
	
	// Try numAttempts times to place a room adjacent to the previous one with the door candidate joining them.
	for(i=0; i<numAttempts; i++) {
		if (!isCorridor) {
			if (isCross) { // cross room
				if (direction == UP || direction == DOWN) {
					roomWidth = rand_range(levelSpec.crossRoomMinWidth, levelSpec.crossRoomMaxWidth);
					roomHeight = rand_range(levelSpec.roomMinHeight, levelSpec.roomMaxHeight);
					roomWidth2 = rand_range(levelSpec.roomMinWidth, levelSpec.roomMaxWidth);
					roomHeight2 = rand_range(levelSpec.crossRoomMinHeight, levelSpec.crossRoomMaxHeight);
				} else {
					roomWidth = rand_range(levelSpec.roomMinWidth, levelSpec.roomMaxWidth);
					roomHeight = rand_range(levelSpec.crossRoomMinHeight, levelSpec.crossRoomMaxHeight);
					roomWidth2 = rand_range(levelSpec.crossRoomMinWidth, levelSpec.crossRoomMaxWidth);
					roomHeight2 = rand_range(levelSpec.roomMinHeight, levelSpec.crossRoomMaxHeight);
				}
			} else { // rectangular room
				roomWidth = rand_range(levelSpec.roomMinWidth, levelSpec.roomMaxWidth);
				roomHeight = rand_range(levelSpec.roomMinHeight, levelSpec.roomMaxHeight);
			}
		}
		
		switch (direction) {
			case UP:
				if (isCorridor) {
					roomWidth = CORRIDOR_WIDTH;
					roomHeight = rand_range(levelSpec.vertCorrMinLength, levelSpec.vertCorrMaxLength);
					newDoorX = doorCandidateX;
					newDoorY = doorCandidateY - roomHeight - 1;
				}
				roomX = rand_range(max(0, doorCandidateX - (roomWidth - 1)), min(DCOLS, doorCandidateX));
				roomY = (doorCandidateY - roomHeight);
				if (isCross) {
					roomX2 = (roomX + (roomWidth / 2) + rand_range(0, 2) + rand_range(0, 2) - 3) - (roomWidth2 / 2);
					roomY2 = (doorCandidateY - roomHeight2 - (rand_range(0, 2) + rand_range(0, 1)));
				}
				break;
			case DOWN:
				if (isCorridor) {
					roomWidth = CORRIDOR_WIDTH;
					roomHeight = rand_range(levelSpec.vertCorrMinLength, levelSpec.vertCorrMaxLength);
					newDoorX = doorCandidateX;
					newDoorY = doorCandidateY + roomHeight + 1;
				}
				roomX = rand_range(max(0, doorCandidateX - (roomWidth - 1)), min(DCOLS, doorCandidateX));
				roomY = (doorCandidateY + 1);
				if (isCross) {
					roomX2 = (roomX + (roomWidth / 2) + rand_range(0, 2) + rand_range(0, 2) - 3) - (roomWidth2 / 2);
					roomY2 = (doorCandidateY + 1 + (rand_range(0, 2) + rand_range(0, 1)));
				}
				break;
			case LEFT:
				if (isCorridor) {
					roomHeight = CORRIDOR_WIDTH;
					roomWidth = rand_range(levelSpec.horCorrMinLength, levelSpec.horCorrMaxLength);
					newDoorX = doorCandidateX - roomWidth - 1;
					newDoorY = doorCandidateY;
				}
				roomX = (doorCandidateX - roomWidth);
				roomY = rand_range(max(0, doorCandidateY - (roomHeight - 1)), min(DCOLS, doorCandidateY));
				if (isCross) {
					roomX2 = (doorCandidateX - roomWidth2 - (rand_range(0, 2) + rand_range(0, 2)));
					roomY2 = (roomY + (roomHeight / 2) + (rand_range(-1, 1) + rand_range(-1, 1))) - (roomHeight2 / 2);
				}
				break;
			case RIGHT:
				if (isCorridor) {
					roomHeight = CORRIDOR_WIDTH;
					roomWidth = rand_range(levelSpec.horCorrMinLength, levelSpec.horCorrMaxLength);
					newDoorX = doorCandidateX + roomWidth + 1;
					newDoorY = doorCandidateY;
				}
				roomX = (doorCandidateX + 1);
				roomY = rand_range(max(0, doorCandidateY - (roomHeight - 1)), min(DCOLS, doorCandidateY));
				if (isCross) {
					roomX2 = (doorCandidateX + 1 + rand_range(0, 2) + rand_range(0, 2));
					roomY2 = (roomY + (roomHeight / 2) + (rand_range(-1, 1) + rand_range(-1, 1))) - (roomHeight2 / 2);
				}
				break;
		}
		if (checkRoom(roomX, roomY, roomWidth, roomHeight) &&
			(!isCross || checkRoom(roomX2, roomY2, roomWidth2, roomHeight2))) {
			// there is space for the room
			if (isCorridor) {
				// proceed only if there is space to build a non-corridor room at the end
				if (nextRoom = attemptRoom(newDoorX, newDoorY, direction, false,
										   rand_percent(thisLevelProfile->crossRoomChance), 15)) {
					// success; build corridor (room at end has already been built)
					carveRectangle(roomX, roomY, roomWidth, roomHeight);
					markRectangle(roomX, roomY, roomWidth, roomHeight);
					pmap[doorCandidateX][doorCandidateY].layers[DUNGEON] = (rand_percent(thisLevelProfile->doorChance) ? DOOR : FLOOR);
					pmap[doorCandidateX][doorCandidateY].flags |= DOORWAY;
					removeWallFromList(direction, doorCandidateX, doorCandidateY);
					builtRoom = allocateRoom(roomX, roomY, roomWidth, roomHeight, 0, 0, 0, 0);
					connectRooms(builtRoom, nextRoom, newDoorX, newDoorY, direction);
					
					//DEBUG printf("Door candidate (corr): (%i, %i):\n Roomloc: (%i, %i) Width: %i, Height: %i\n",
					//			 doorCandidateX, doorCandidateY, roomX, roomY, roomWidth, roomHeight);
					//DEBUG logLevel();
					
					return builtRoom;
				} else {
					// couldn't build room at the end of the corridor; abort corridor.
					return NULL;
				}
			} else {
				if (isCross) {
					carveRectangle(roomX, roomY, roomWidth, roomHeight);
					carveRectangle(roomX2, roomY2, roomWidth2, roomHeight2);
					markRectangle(roomX, roomY, roomWidth, roomHeight);
					markRectangle(roomX2, roomY2, roomWidth2, roomHeight2);
					builtRoom = allocateRoom(roomX, roomY, roomWidth, roomHeight, roomX2, roomY2, roomWidth2, roomHeight2);
				} else {
					carveRectangle(roomX, roomY, roomWidth, roomHeight);
					markRectangle(roomX, roomY, roomWidth, roomHeight);
					builtRoom = allocateRoom(roomX, roomY, roomWidth, roomHeight, 0, 0, 0, 0);
				}
				
				pmap[doorCandidateX][doorCandidateY].layers[DUNGEON] = (rand_percent(thisLevelProfile->doorChance) ? DOOR : FLOOR);
				pmap[doorCandidateX][doorCandidateY].flags |= DOORWAY;
				removeWallFromList(direction, doorCandidateX, doorCandidateY);
				
				//DEBUG printf("Door candidate: (%i, %i):\n", doorCandidateX, doorCandidateY);
				//DEBUG logLevel();
				
				numberOfRooms++;
				
				return builtRoom;
				break;
			}
		}
	}
	
	// couldn't find space for the room after numAttempts attempts.
	return NULL;
}

// Checks to see if a given roomX, roomY, roomWidth, and roomHeight describe a valid room.
// This check has two components: first, is the room within the bounds of the dungeon?
// Second, are all of its cells undeveloped (i.e. made of granite)?
boolean checkRoom(short roomX, short roomY, short roomWidth, short roomHeight) {
	int i, j;
	if ((roomX < 1) || (roomX + roomWidth > DCOLS - 1) || (roomY < 1) || (roomY + roomHeight > DROWS-1)) {
		return false;
	}
	
	for (i=roomX; i < (roomX + roomWidth); i++) {
		for (j=roomY; j < (roomY + roomHeight); j++) {
			if (pmap[i][j].layers[DUNGEON] != GRANITE) {
				return false;
			}
		}
	}
	return true;
}

// Carves a rectangular hole into the dungeon by marking contained GRANITE tiles as FLOOR
void carveRectangle(short roomX, short roomY, short roomWidth, short roomHeight) {
	short i, j;
	for ( i=roomX; i<(roomX + roomWidth); i++) {
		for ( j=roomY; j<(roomY + roomHeight); j++) {
			if (pmap[i][j].layers[DUNGEON] == GRANITE) {
				pmap[i][j].layers[DUNGEON] = FLOOR;
				//pmap[i][j].layers[LIQUID] = rand_percent(10) ? DEEP_WATER : NOTHING;
			}
		}
	}
}

// Marks the surrounding granite as various WALL tiles, if indeed they are walls
void markRectangle(short roomX, short roomY, short roomWidth, short roomHeight) {
	short i;
	
	// Fill top and bottom walls with WALL tiles
	for ( i=roomX; i<(roomX + roomWidth); i++) {
		if (pmap[i][roomY - 1].layers[DUNGEON] == GRANITE) {
			pmap[i][roomY - 1].layers[DUNGEON] = TOP_WALL;
			addWallToList(UP, i, roomY-1);
		}
		
		if (pmap[i][roomY + roomHeight].layers[DUNGEON] == GRANITE) {
			pmap[i][roomY + roomHeight].layers[DUNGEON] = BOTTOM_WALL;
			addWallToList(DOWN, i, roomY + roomHeight);
		}
	}
	
	// Fill left and right walls with WALL tiles
	for ( i=roomY; i<(roomY + roomHeight); i++) {
		if (pmap[roomX - 1][i].layers[DUNGEON] == GRANITE) {
			pmap[roomX - 1][i].layers[DUNGEON] = LEFT_WALL;
			addWallToList(LEFT, roomX - 1, i);
		}
		
		if (pmap[roomX + roomWidth][i].layers[DUNGEON] == GRANITE) {
			pmap[roomX + roomWidth][i].layers[DUNGEON] = RIGHT_WALL;
			addWallToList(RIGHT, roomX + roomWidth, i);
		}
	}
	
	// Set the corners of the room to PERM_WALL tiles
	if ((pmap[roomX - 1][roomY - 1].layers[DUNGEON]) == GRANITE) {
		pmap[roomX - 1][roomY - 1].layers[DUNGEON] = PERM_WALL;
	}
	if (pmap[roomX + roomWidth][roomY - 1].layers[DUNGEON] == GRANITE) {
		pmap[roomX + roomWidth][roomY - 1].layers[DUNGEON] = PERM_WALL;
	}
	if (pmap[roomX - 1][roomY + roomHeight].layers[DUNGEON] == GRANITE) {
		pmap[roomX - 1][roomY + roomHeight].layers[DUNGEON] = PERM_WALL;
	}
	if (pmap[roomX + roomWidth][roomY + roomHeight].layers[DUNGEON] == GRANITE) {
		pmap[roomX + roomWidth][roomY + roomHeight].layers[DUNGEON] = PERM_WALL;
	}
}

void addWallToList(short direction, short xLoc, short yLoc) {
	numberOfWalls[direction]++;
	listOfWallsX[direction][numberOfWalls[direction]] = xLoc;
	listOfWallsY[direction][numberOfWalls[direction]] = yLoc;
}

// returns the minimum number of rooms one must travel to get from the fromRoom to the toRoom
short distanceBetweenRooms(room *fromRoom, room *toRoom) {
	room *theRoom, *theNeighbor;
	boolean somethingChanged;
	short i;
	
	for (theRoom = rooms->nextRoom; theRoom != NULL; theRoom = theRoom->nextRoom) {
		theRoom->pathNumber = numberOfRooms + 5; // set it high
	}
	toRoom->pathNumber = 0;
	
	do {
		somethingChanged = false;
		for (theRoom=rooms->nextRoom; theRoom != NULL; theRoom=theRoom->nextRoom) {
			for (i=0; i < theRoom->numberOfSiblings; i++) {
				theNeighbor = theRoom->siblingRooms[i];
				if (theNeighbor->pathNumber + 1 < theRoom->pathNumber) {
					theRoom->pathNumber = theNeighbor->pathNumber + 1;
					somethingChanged = true;
				}
			}
		}
	} while (somethingChanged);
	return fromRoom->pathNumber;
}

void removeWallFromList(short direction, short xLoc, short yLoc) {
	int i;
	boolean locatedTheWall = false;
	for (i=0; i<numberOfWalls[direction]; i++) {
		if (listOfWallsX[direction][i] == xLoc && listOfWallsY[direction][i] == yLoc) {
			locatedTheWall = true;
			break;
		}
	}
	if (locatedTheWall) {
		for (; i < numberOfWalls[direction] - 1; i++) {
			listOfWallsX[direction][i] = listOfWallsX[direction][i + 1];
			listOfWallsY[direction][i] = listOfWallsY[direction][i + 1];
		}
		numberOfWalls[direction]--;
	} else {
		//DEBUG printf("Failed to remove wall at %i, %i", xLoc, yLoc);
	}
}

// loads up buffer[][] with a cellular automata. Returns nothing directly, but saves
// its results in globals buffer[][], topBlobMinX, topBlobMinY, blobWidth, and blobHeight.
void cellularAutomata(short minBlobWidth, short minBlobHeight,
					  short maxBlobWidth, short maxBlobHeight, short percentSeeded,
					  char birthParameters[9], char survivalParameters[9]) {
	short i, j, k, l, neighborCount, tempX, tempY;
	short blobNumber, blobSize, topBlobSize, topBlobNumber;
	short topBlobMaxX, topBlobMaxY;
	short buffer2[maxBlobWidth][maxBlobHeight]; // buffer[][] is already a global short array
	boolean foundACellThisLine;
	
	// Generate caves until they satisfy the MIN_CAVE_WIDTH and MIN_CAVE_HEIGHT restraints
	do {
		blobNumber = 2;
		
		// These are best-of variables; start them out at worst-case values.
		topBlobSize = 0;
		topBlobNumber = 0;
		topBlobMinX=maxBlobWidth;
		topBlobMaxX=0;
		topBlobMinY=maxBlobHeight;
		topBlobMaxY=0;
		
		// clear buffer
		for( i=0; i<DCOLS; i++ ) {
			for( j=0; j<DROWS; j++ ) {	
				buffer[i][j] = 0;
			}
		}
		
		// Fill relevant portion with 45% wall (0), 55% floor (1)
		for( i=0; i<maxBlobWidth; i++ ) {
			for( j=0; j<maxBlobHeight; j++ ) {	
				buffer[i][j] = (rand_percent(percentSeeded) ? 1 : 0);
			}
		}
		
		// DEBUG logBuffer(buffer);
		
		// Four iterations of automata
		for (k=0; k<4; k++) {
			for( i=0; i<maxBlobWidth; i++ ) {
				for( j=0; j<maxBlobHeight; j++ ) {	
					buffer2[i][j] = buffer[i][j];
				}
			}
			
			for( i=0; i<maxBlobWidth; i++ ) {
				for( j=0; j<maxBlobHeight; j++ ) {
					neighborCount = 0;
					for (l=0; l<8; l++) {
						tempX = i + nbDirs[l][0];
						if (tempX < 0 || tempX > maxBlobWidth) {
							break;
						}
						tempY = j + nbDirs[l][1];
						if (tempY < 0 || tempY > maxBlobHeight) {
							break;
						}
						if (buffer2[tempX][tempY] == 1) { // floor
							neighborCount++;
						}
					}
					if (!buffer2[i][j] && birthParameters[neighborCount] == 't') {
						buffer[i][j] = 1;	// birth
					} else if (buffer2[i][j] && survivalParameters[neighborCount] == 't') {
											// survival
					} else {
						buffer[i][j] = 0;	// death
					}
				}
			}
			// DEBUG logBuffer(buffer);
		}
		
		// DEBUG logBuffer();
		
		// Fill each blob with its own number, starting with 2 (since 1 means floor), and keeping track of the biggest:
		for( i=0; i<maxBlobWidth; i++ ) {
			for( j=0; j<maxBlobHeight; j++ ) {
				if (buffer[i][j] == 1) { // an unmarked blob
					
					// Marks all the cells and returns the total size:
					blobSize = markBlobCellAndIterate(i, j, blobNumber);
					
					//DEBUG printf("Blob %i is size %i.\n", blobNumber, blobSize);
					if (blobSize > topBlobSize) { // if this blob is a new record
						topBlobSize = blobSize;
						topBlobNumber = blobNumber;
					}
					blobNumber++;
				}
			}
		}
		
		// DEBUG logBuffer(buffer);
		
		// Figure out the top blob's height and width:
		// First find the max & min x:
		foundACellThisLine = false;
		for( i=0; i<maxBlobWidth; i++ ) {
			for( j=0; j<maxBlobHeight; j++ ) {
				if (buffer[i][j] == topBlobNumber) {
					foundACellThisLine = true;
					break;
				}
			}
			if (foundACellThisLine) {
				if (i < topBlobMinX) {
					topBlobMinX = i;
				}
				if (i > topBlobMaxX) {
					topBlobMaxX = i;
				}
			}
			foundACellThisLine = false;
		}
		
		// then the max & min y:
		for( j=0; j<maxBlobHeight; j++ ) {
			for( i=0; i<maxBlobWidth; i++ ) {
				if (buffer[i][j] == topBlobNumber) {
					foundACellThisLine = true;
					break;
				}
			}
			if (foundACellThisLine) {
				if (j < topBlobMinY) {
					topBlobMinY = j;
				}
				if (j > topBlobMaxY) {
					topBlobMaxY = j;
				}
			}
			foundACellThisLine = false;
		}
		// DEBUG printf("Blob #%i: minX %i, maxX %i, minY %i, maxY %i, size %i.\n", topBlobNumber,
		//			 topBlobMinX, topBlobMaxX, topBlobMinY, topBlobMaxY, topBlobSize);
		
		blobWidth =		(topBlobMaxX - topBlobMinX) + 1;
		blobHeight =	(topBlobMaxY - topBlobMinY) + 1;
		
	} while ((topBlobMaxX - topBlobMinX) <= minBlobWidth ||
			 (topBlobMaxY - topBlobMinY) <= minBlobHeight);
	
	// Replace the winning blob with 1's, and everything else with 0's:
	for( i=0; i<maxBlobWidth; i++ ) {
		for( j=0; j<maxBlobHeight; j++ ) {
			if (buffer[i][j] == topBlobNumber) {
				buffer[i][j] = 1;
			} else {
				buffer[i][j] = 0;
			}
		}
	}
}


// Uses four iterations of cellular automata with 55% floor initial pattern and a B678/S45678 ruleset.
// Chooses the biggest contiguous block after that process and then copies it into the tmap[][] array.
void generateCave() {
	short i, j, k;
	short roomX, roomY;
	room *theRoom;
	
	cellularAutomata(CAVE_MIN_WIDTH, CAVE_MIN_HEIGHT, DCOLS - 2, DROWS - 2, 55, "ffffffttt", "ffffttttt");
	
	// position the new room
	roomX = rand_range(1, DCOLS - blobWidth - 1);
	roomY = rand_range(1, DROWS - blobHeight - 1);
	
	// and copy it to tmap[][]
	short bufferX, bufferY, dungeonX, dungeonY;
	
	for( i = 0; i < blobWidth; i++ ) {
		for( j=0; j < blobHeight; j++ ) {
			bufferX = i + topBlobMinX;
			bufferY = j + topBlobMinY;
			dungeonX = i + roomX;
			dungeonY = j + roomY;
			if (buffer[bufferX][bufferY]) {
				pmap[dungeonX][dungeonY].layers[DUNGEON] = FLOOR;
			}
		}
	}
	
	// Mark the edges as walls
	for( i=0; i< blobWidth; i++ ) {
		for( j=0; j< blobHeight; j++ ) {
			bufferX = i + topBlobMinX;
			bufferY = j + topBlobMinY;
			dungeonX = i + roomX;
			dungeonY = j + roomY;
			if (buffer[bufferX][bufferY]) {
				if (dungeonX > 0 && pmap[dungeonX-1][dungeonY].layers[DUNGEON] == GRANITE) {
					pmap[dungeonX-1][dungeonY].layers[DUNGEON] = LEFT_WALL;
					if (dungeonX > ROOM_MIN_WIDTH) {
						addWallToList(LEFT, dungeonX-1, dungeonY);
					}
				}
				if (dungeonX < DCOLS && pmap[dungeonX+1][dungeonY].layers[DUNGEON] == GRANITE) {
					pmap[dungeonX+1][dungeonY].layers[DUNGEON] = RIGHT_WALL;
					addWallToList(RIGHT, dungeonX+1, dungeonY);
					if (dungeonX < DCOLS - ROOM_MIN_WIDTH) {
						addWallToList(LEFT, dungeonX-1, dungeonY);
					}
				}
				if (dungeonY > 0 && pmap[dungeonX][dungeonY-1].layers[DUNGEON] == GRANITE) {
					pmap[dungeonX][dungeonY-1].layers[DUNGEON] = TOP_WALL;
					if (dungeonY > ROOM_MIN_HEIGHT) {
						addWallToList(UP, dungeonX, dungeonY-1);
					}
				}
				if (dungeonY < DROWS && pmap[dungeonX][dungeonY+1].layers[DUNGEON] == GRANITE) {
					pmap[dungeonX][dungeonY+1].layers[DUNGEON] = BOTTOM_WALL;
					if (dungeonY < DROWS - ROOM_MIN_HEIGHT) {
						addWallToList(DOWN, dungeonX, dungeonY+1);
					}
				}
			}
		}
	}
	
	// Mark the corners as PERM_WALLs
	for (i = roomX; i < roomX + blobWidth; i++) {
		for (j=roomY; j < roomY + blobHeight; j++) {
			for (k=4; k<8; k++) {
				if ((pmap[i][j].layers[DUNGEON] == FLOOR) &&
					(i + nbDirs[k][0]) >=0 && (j + nbDirs[k][0]) <= DCOLS &&
					(j + nbDirs[k][1]) >=0 && (j + nbDirs[k][1]) <= DROWS &&
					(pmap[i+nbDirs[k][0]][j+nbDirs[k][1]].layers[DUNGEON] == GRANITE)) {
					pmap[i+nbDirs[k][0]][j+nbDirs[k][1]].layers[DUNGEON] = PERM_WALL;
				}
			}
		}
	}
	
	theRoom = allocateRoom(roomX, roomY, blobWidth, blobHeight, 0, 0, 0, 0);
}

// Marks a cell as being a member of blobNumber, then recursively iterates through the rest of the blob
short markBlobCellAndIterate(short xLoc, short yLoc, short blobNumber) {
	short l, newX, newY, numberOfCells = 1;
	
	buffer[xLoc][yLoc] = blobNumber;
	
	// iterate through the neighbors
	for (l=0; l<4; l++) { // only the four cardinal neighbors; no diagonals!
		newX = xLoc + nbDirs[l][0];
		if (newX < 0 || newX > DCOLS) {
			break;
		}
		newY = yLoc + nbDirs[l][1];
		if (newY < 0 || newY > DROWS) {
			break;
		}
		if (buffer[newX][newY] == 1) { // if the neighbor is an unmarked blob cell
			numberOfCells += markBlobCellAndIterate(newX, newY, blobNumber); // recursive!
		}
	}
	return numberOfCells;
}

void connectCell(short x, short y, char blockingMap[DCOLS][DROWS], char visited[DCOLS][DROWS]) {
	enum directions dir;
	short newX, newY;
	
	visited[x][y] = 1;
	for (dir = 0; dir < 4; dir++) {
		newX = x + nbDirs[dir][0];
		newY = y + nbDirs[dir][1];
		
		if (coordinatesAreInMap(newX, newY) && !visited[newX][newY] && !blockingMap[newX][newY]
			&& (!cellHasTerrainFlag(newX, newY, PATHING_BLOCKER)
				|| (cellHasTerrainFlag(newX, newY, IS_SECRET)
					&& cellHasTerrainFlag(newX, newY, OBSTRUCTS_PASSABILITY)))) {
			
			connectCell(newX, newY, blockingMap, visited);
		}
	}
}

boolean levelIsConnectedWithBlockingMap(char blockingMap[DCOLS][DROWS]) {
	char visited[DCOLS][DROWS];
	short i, j, startX, startY;
	
	zeroOutGrid(visited);
	
	startX = 0;
	
	// get starting location for the fill
	for (i=0; i<DCOLS && startX == 0; i++) {
		for (j=0; j<DROWS; j++) {
			if (!cellHasTerrainFlag(i, j, PATHING_BLOCKER) && !blockingMap[i][j]) {
				startX = i;
				startY = j;
				break;
			}
		}
	}
	
	connectCell(startX, startY, blockingMap, visited);
	
	// check if the whole level is filled
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			if (!cellHasTerrainFlag(i, j, PATHING_BLOCKER) && !blockingMap[i][j] && !visited[i][j]) {
				return false;
			}
		}
	}
	return true;
}

void floodFillLakeCheck(short xLoc, short yLoc) {
	short l, newX, newY;
	
	//logLevel();
	
	tmap[xLoc][yLoc].connected = 1;
	
	// iterate through the neighbors
	for (l=0; l<4; l++) { // only the four cardinal neighbors; no diagonals!
		newX = xLoc + nbDirs[l][0];
		if (newX < 0 || newX > DCOLS) {
			break;
		}
		newY = yLoc + nbDirs[l][1];
		if (newY < 0 || newY > DROWS) {
			break;
		}
		if ((tmap[newX][newY].connected == 0) &&					// If there's no proposed media and it's unmarked
			(!cellHasTerrainFlag(newX, newY, PATHING_BLOCKER)
			|| tileCatalog[pmap[newX][newY].layers[DUNGEON]].flags & IS_SECRET)	// and it's passable
			&& pmap[newX][newY].layers[LIQUID] == NOTHING) {		// and there's no real media either
			floodFillLakeCheck(newX, newY);							// ...then recurse.
		}
	}
}

boolean checkLakePassability(short lakeX, short lakeY) {
	short i, j, startX = -1, startY = -1;
	
	//logLevel();
	
	// get starting location for the fill
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			if (i < lakeX + blobWidth && i >= lakeX &&
				j < lakeY + blobHeight && j >= lakeY &&
				buffer[i - lakeX + topBlobMinX][j - lakeY + topBlobMinY] == 1) {
				tmap[i][j].connected = -1;
			} else {
				tmap[i][j].connected = 0;
			}
			if (pmap[i][j].layers[DUNGEON] == FLOOR && pmap[i][j].layers[LIQUID] == NOTHING && tmap[i][j].connected == 0) { // dry floor
				startX = i;
				startY = j;
			}
		}
	}
	floodFillLakeCheck(startX, startY);
	
	//logLevel();
	
	// if we can find an unmarked dry cell, the tmap is not connected.
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			if (pmap[i][j].layers[DUNGEON] == FLOOR && pmap[i][j].layers[LIQUID] == NOTHING && tmap[i][j].connected == 0) { // unmarked dry floor
				// printf("\nLake failed at (%i, %i)", i, j);
				return false;
			}
		}
	}
	return true;
}

void liquidType(short *deep, short *shallow, short *shallowWidth) {
	short randMin, randMax, rand;
	
	randMin = (rogue.depthLevel < 4 ? 1 : 0); // no lava before level 4
	randMax = (rogue.depthLevel < 17 ? 2 : 3); // no brimstone before level 18
	rand = rand_range(randMin, randMax);
	
	switch(rand) {
		case 0:
			*deep = LAVA;
			*shallow = NOTHING;
			*shallowWidth = 0;
			break;
		case 1:
			*deep = DEEP_WATER;
			*shallow = SHALLOW_WATER;
			*shallowWidth = 2;
			break;
		case 2:
			*deep = CHASM;
			*shallow = CHASM_EDGE;
			*shallowWidth = 1;
			break;
		case 3:
			*deep = INERT_BRIMSTONE;
			*shallow = OBSIDIAN;
			*shallowWidth = 2;
			break;
	}
}

// Fills a lake of UNFILLED_LAKE with the specified liquid type, scanning outward to reach other lakes within scanWidth.
// Any wreath of shallow liquid must be done elsewhere.
void fillLake(short x, short y, short liquid, short scanWidth, char wreathMap[DCOLS][DROWS]) {
	short i, j;
	
	for (i = x - scanWidth; i <= x + scanWidth; i++) {
		for (j = y - scanWidth; j <= y + scanWidth; j++) {
			if (coordinatesAreInMap(i, j) && pmap[i][j].layers[LIQUID] == UNFILLED_LAKE) {
				pmap[i][j].layers[LIQUID] = liquid;
				wreathMap[i][j] = 1;
				fillLake(i, j, liquid, scanWidth, wreathMap);	// recursive
			}
		}
	}
}

void fillSpawnMap(enum dungeonLayers layer, enum tileType surfaceTileType, char spawnMap[DCOLS][DROWS], boolean refresh) {
	short i, j;
	creature *monst;
	item *theItem;
	
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			if (spawnMap[i][j]
				&& pmap[i][j].layers[layer] != surfaceTileType
				&& (refresh || pmap[i][j].layers[LIQUID] == NOTHING)
				&& tileCatalog[pmap[i][j].layers[layer]].drawPriority >= tileCatalog[surfaceTileType].drawPriority
				&& (layer != SURFACE || (!cellHasTerrainFlag(i, j, OBSTRUCTS_SURFACE_EFFECTS)))
				&& (layer != SURFACE || tileCatalog[pmap[i][j].layers[highestPriorityLayer(i, j, true)]].drawPriority >= tileCatalog[surfaceTileType].drawPriority)) {
				
				if ((tileCatalog[surfaceTileType].flags & IS_FIRE)
					&& !(tileCatalog[pmap[i][j].layers[layer]].flags & IS_FIRE)) {
					pmap[i][j].flags |= CAUGHT_FIRE_THIS_TURN;
				}
				
				pmap[i][j].layers[layer] = surfaceTileType;
				
				if (refresh) {
					refreshDungeonCell(i, j);
					if (player.xLoc == i && player.yLoc == j && !player.status.levitating) {
						message(tileFlavor(player.xLoc, player.yLoc), false, false);
					}
					if (pmap[i][j].flags & (HAS_MONSTER | HAS_PLAYER)) {
						monst = monsterAtLoc(i, j);
						applyInstantTileEffectsToCreature(monst);
						if (rogue.gameHasEnded) {
							return;
						}
					}
					
					if (tileCatalog[surfaceTileType].flags & IS_FIRE) {
						if (pmap[i][j].flags & HAS_ITEM) {
							theItem = itemAtLoc(i, j);
							if (theItem->flags & ITEM_FLAMMABLE) {
								burnItem(theItem);
							}
						}
					}
				}
			}
		}
	}
}

void spawnMapDF(short x, short y, enum dungeonLayers layer, enum tileType surfaceTileType,
				enum tileType propagationTerrain, boolean requirePropTerrain, short startProb,
				short probDec, char spawnMap[DCOLS][DROWS]) {
	
	short i, j, dir, t, x2, y2;
	boolean madeChange;
	
	spawnMap[x][y] = t = 1;
	
	do {
		madeChange = false;
		t++;
		for (i = 0; i < DCOLS; i++) {
			for (j=0; j<DROWS; j++) {
				if (spawnMap[i][j] == t - 1) {
					for (dir = 0; dir < 4; dir++) {
						x2 = i + nbDirs[dir][0];
						y2 = j + nbDirs[dir][1];
						if (coordinatesAreInMap(x2, y2)
							&& (!requirePropTerrain || (propagationTerrain > 0 && cellHasTerrainType(x2, y2, propagationTerrain)))
							&& (!cellHasTerrainFlag(x2, y2, OBSTRUCTS_SURFACE_EFFECTS) || (propagationTerrain > 0 && cellHasTerrainType(x2, y2, propagationTerrain)))
							&& rand_percent(startProb)) {
							
							spawnMap[x2][y2] = t;
							madeChange = true;
						}
					}
				}
			}
		}
		startProb -= probDec;
		if (t > 100) {
			for (i = 0; i < DCOLS; i++) {
				for (j=0; j<DROWS; j++) {
					if (spawnMap[i][j] == t) {
						spawnMap[i][j] = 2;
					} else if (spawnMap[i][j] > 0) {
						spawnMap[i][j] = 1;
					}
				}
			}
			t = 2;
		}
	} while (madeChange && startProb > 0);
}

void spawnDungeonFeature(short x, short y, dungeonFeature *feat, boolean refreshCell) {
	char blockingMap[DCOLS][DROWS];
	boolean blocking = ((!refreshCell && tileCatalog[feat->tile].flags & (PATHING_BLOCKER)) ? true : false);
	
	if (feat->layer == GAS) {
		pmap[x][y].volume += randClump(feat->startProbability);
		pmap[x][y].layers[GAS] = feat->tile;
	} else {
		zeroOutGrid(blockingMap);
		spawnMapDF(x, y, feat->layer, feat->tile,
						  feat->propagationTerrain,
						  false,
						  randClump(feat->startProbability),
						  randClump(feat->probabilityDecrement),
						  blockingMap);
		if (!blocking || levelIsConnectedWithBlockingMap(blockingMap)) {
			fillSpawnMap(feat->layer, feat->tile, blockingMap, refreshCell);
		}		
	}
	if (feat->subsequentDF) {
		spawnDungeonFeature(x, y, &dungeonFeatureCatalog[feat->subsequentDF], refreshCell);
	}
	if (tileCatalog[feat->tile].flags & (IS_DEEP_WATER | LAVA_INSTA_DEATH | TRAP_DESCENT)) {
		updateMapToShore();
	}
}

/*void spawnSurfaceEffect(short x, short y, enum dungeonLayers layer, enum tileType surfaceTileType,
						enum tileType propagationTerrain, boolean requirePropTerrain, short startProbability,
						short probabilityDecrement, char spawnMap[DCOLS][DROWS], boolean refreshCell) {
	short i, x2, y2, dirs[4];
	creature *monst;
	item *theItem;
	
	if (rogue.gameHasEnded) {
		return;
	}
	if (tileCatalog[pmap[x][y].layers[layer]].drawPriority >= tileCatalog[surfaceTileType].drawPriority
		&& pmap[x][y].layers[layer] != surfaceTileType
		&& (!spawnMap || !spawnMap[x][y])
		&& (layer != SURFACE || (!cellHasTerrainFlag(x, y, OBSTRUCTS_SURFACE_EFFECTS)))
		&& (refreshCell || pmap[x][y].layers[LIQUID] == NOTHING) ) {
		if (spawnMap) {
			spawnMap[x][y] = 1;
		} else {
			if (tileCatalog[surfaceTileType].flags & IS_FIRE && !(tileCatalog[pmap[x][y].layers[layer]].flags & IS_FIRE)) {
				pmap[x][y].flags |= CAUGHT_FIRE_THIS_TURN;
			}
			if (layer != SURFACE
				|| tileCatalog[pmap[x][y].layers[highestPriorityLayer(x, y, true)]].drawPriority >= tileCatalog[surfaceTileType].drawPriority) {
				pmap[x][y].layers[layer] = surfaceTileType;
			}
			
			if (refreshCell) {
				if (pmap[x][y].flags & (HAS_MONSTER | HAS_PLAYER)) {
					monst = monsterAtLoc(x, y);
					applyInstantTileEffectsToCreature(monst);
					if (rogue.gameHasEnded) {
						return;
					}
				}
				
				if (tileCatalog[surfaceTileType].flags & IS_FIRE) {
					if (pmap[x][y].flags & HAS_ITEM) {
						for (theItem = floorItems->nextItem; theItem->xLoc != x || theItem->yLoc != y; theItem = theItem->nextItem);
						if (theItem->flags & ITEM_FLAMMABLE) {
							burnItem(theItem);
						}
					}
				}
			}
		}
		
		if (refreshCell) {
			refreshDungeonCell(x, y);
			if (player.xLoc == x && player.yLoc == y && !player.status.levitating) {
				message(tileFlavor(player.xLoc, player.yLoc), false, false);
			}
		}
	}
	for (i=0; i<4; i++) {
		dirs[i] = i;
	}
	shuffleList(dirs, 4);
	for (i=0; i<4; i++) {
		x2 = x + nbDirs[dirs[i]][0];
		y2 = y + nbDirs[dirs[i]][1];
		if (coordinatesAreInMap(x2, y2)
			&& ((startProbability / max(probabilityDecrement, 1) < 5) || pmap[x2][y2].layers[layer] != surfaceTileType)
			&& (!requirePropTerrain || (propagationTerrain > 0 && cellHasTerrainType(x2, y2, propagationTerrain)))
			&& (!cellHasTerrainFlag(x2, y2, OBSTRUCTS_SURFACE_EFFECTS) || (propagationTerrain > 0 && cellHasTerrainType(x2, y2, propagationTerrain)))
			&& rand_percent(startProbability)) {
			spawnSurfaceEffect(x2, y2, layer, surfaceTileType, propagationTerrain, requirePropTerrain,
							   startProbability - probabilityDecrement, probabilityDecrement, spawnMap, refreshCell);
		}
	}
}*/

void restoreMonster(creature *monst, short **mapToStairs, short **mapToPit) {
	short i, *x, *y, loc[2], dir, turnCount;
	creature *leader;
	boolean foundLeader = false;
	short **theMap;
	
	x = &(monst->xLoc);
	y = &(monst->yLoc);
	
	if (monst->status.entersLevelIn > 0) {
		if (monst->bookkeepingFlags & (MONST_APPROACHING_PIT)) {
			theMap = mapToPit;
		} else {
			theMap = mapToStairs;
		}
		if (theMap) {
			turnCount = ((theMap[monst->xLoc][monst->yLoc] * monst->movementSpeed / 100) - monst->status.entersLevelIn);
			for (i=0; i < turnCount; i++) {
				if ((dir = nextStep(theMap, monst->xLoc, monst->yLoc)) != -1) {
					monst->xLoc += nbDirs[dir][0];
					monst->yLoc += nbDirs[dir][1];	
				}
			}
		}
		monst->bookkeepingFlags |= MONST_PREPLACED;
	}
	
	if ((cellHasTerrainFlag(*x, *y, OBSTRUCTS_PASSABILITY) && !(monst->info.flags & MONST_ATTACKABLE_THRU_WALLS))
		|| (pmap[*x][*y].flags & HAS_PLAYER)
		|| monst->bookkeepingFlags & MONST_PREPLACED) {
		if (!(monst->bookkeepingFlags & MONST_PREPLACED)) {
			pmap[*x][*y].flags &= ~HAS_MONSTER;
		}
		getQualifyingLocNear(loc, *x, *y, DCOLS, (OBSTRUCTS_PASSABILITY | TRAP_DESCENT | IS_DEEP_WATER | LAVA_INSTA_DEATH),
							 (HAS_MONSTER | HAS_PLAYER | HAS_ITEM | HAS_UP_STAIRS | HAS_DOWN_STAIRS), true);
		*x = loc[0];
		*y = loc[1];
	}
	pmap[*x][*y].flags |= HAS_MONSTER;
	monst->bookkeepingFlags &= ~(MONST_PREPLACED | MONST_APPROACHING_DOWNSTAIRS | MONST_APPROACHING_UPSTAIRS | MONST_APPROACHING_PIT);
	
	if (monst->bookkeepingFlags & MONST_FOLLOWER) {
		// is the leader on the same level?
		for (leader = monsters->nextCreature; leader != NULL; leader = leader->nextCreature) {
			if (leader == monst->leader) {
				foundLeader = true;
				break;
			}
		}
		// if not, it is time to spread your wings and fly solo
		if (!foundLeader) {
			monst->bookkeepingFlags &= ~MONST_FOLLOWER;
			monst->leader = NULL;
		}
	}
	
	if (monst->info.flags & MONST_INTRINSIC_LIGHT) {
		monst->intrinsicLight = newLight(&lightCatalog[monst->info.intrinsicLightType], 0, 0, monst);
	}
	
	if (monst->status.burning && !(monst->info.flags & MONST_FIERY)) {
		monst->statusLight = newLight(&lightCatalog[BURNING_CREATURE_LIGHT], 0, 0, monst);
	}
}

void restoreItem(item *theItem) {
	short *x, *y, loc[2];
	x = &(theItem->xLoc);
	y = &(theItem->yLoc);
	
	if (cellHasTerrainFlag(*x, *y, OBSTRUCTS_PASSABILITY)) {
		randomMatchingLocation(loc, FLOOR, NOTHING, -1);
		*x = loc[0];
		*y = loc[1];
	}
	if (theItem->flags & ITEM_PREPLACED) {
		theItem->flags &= ~ITEM_PREPLACED;
		getQualifyingLocNear(loc, *x, *y, DCOLS, (OBSTRUCTS_ITEMS | TRAP_DESCENT | IS_DEEP_WATER | LAVA_INSTA_DEATH),
							 (HAS_MONSTER | HAS_ITEM | HAS_UP_STAIRS | HAS_DOWN_STAIRS), true);
		*x = loc[0];
		*y = loc[1];
	}
	pmap[*x][*y].flags |= HAS_ITEM;
	if (theItem->flags & ITEM_MAGIC_DETECTED && itemMagicChar(theItem)) {
		pmap[*x][*y].flags |= ITEM_DETECTED;
	}
}

// Places the player, monsters, items and stairs.
void initializeLevel(short oldLevelNumber, short stairDirection) {
	short upLoc[2], downLoc[2], **mapToStairs, **mapToPit;
	creature *monst;
	item *theItem, *prevItem;
	boolean amuletOnLevel;
	short n = rogue.depthLevel - 1;
	
	getQualifyingLocNear(downLoc, levels[n].downStairsLoc[0], levels[n].downStairsLoc[1],
						 DCOLS, (OBSTRUCTS_ITEMS | TRAP_DESCENT | IS_DEEP_WATER | LAVA_INSTA_DEATH | ALLOWS_SUBMERGING), // | IS_BRIDGE),
						 (HAS_MONSTER | HAS_ITEM | HAS_UP_STAIRS | HAS_DOWN_STAIRS), true);
	if (rogue.depthLevel < 100) {
		pmap[downLoc[0]][downLoc[1]].layers[DUNGEON] = DOWN_STAIRS;
	}
	if (!levels[n+1].visited) {
		levels[n+1].upStairsLoc[0] = downLoc[0];
		levels[n+1].upStairsLoc[1] = downLoc[1];
	}
	
	levels[n].downStairsLoc[0] = downLoc[0];
	levels[n].downStairsLoc[1] = downLoc[1];
	
	getQualifyingLocNear(upLoc, levels[n].upStairsLoc[0], levels[n].upStairsLoc[1],
						 DCOLS, (OBSTRUCTS_ITEMS | TRAP_DESCENT | IS_DEEP_WATER | LAVA_INSTA_DEATH | ALLOWS_SUBMERGING),
						 (HAS_MONSTER | HAS_ITEM | HAS_UP_STAIRS | HAS_DOWN_STAIRS), true);
	
	levels[n].upStairsLoc[0] = upLoc[0];
	levels[n].upStairsLoc[1] = upLoc[1];
	
	if (rogue.depthLevel == 1) {
		pmap[upLoc[0]][upLoc[1]].layers[DUNGEON] = DUNGEON_EXIT;
	} else {
		pmap[upLoc[0]][upLoc[1]].layers[DUNGEON] = UP_STAIRS;
	}
	if (stairDirection == 1) { // heading downward
		player.xLoc = upLoc[0];
		player.yLoc = upLoc[1];
		pmap[player.xLoc][player.yLoc].flags |= HAS_PLAYER;
	} else if (stairDirection == -1) { // heading upward
		player.xLoc = downLoc[0];
		player.yLoc = downLoc[1];
		pmap[player.xLoc][player.yLoc].flags |= HAS_PLAYER;
	}
	monsters->nextCreature = levels[rogue.depthLevel-1].monsters;
	floorItems->nextItem = levels[rogue.depthLevel-1].items;
	
	rogue.downLoc[0] = downLoc[0];
	rogue.downLoc[1] = downLoc[1];
	rogue.upLoc[0] = upLoc[0];
	rogue.upLoc[1] = upLoc[1];
	
	// Only one amulet per level!
	amuletOnLevel = (numberOfMatchingPackItems(AMULET, 0, 0, false) > 0);
	for (theItem = floorItems->nextItem; theItem != NULL; theItem = theItem->nextItem) {
		if (theItem->category & AMULET) {
			if (amuletOnLevel) {
				for (prevItem = floorItems; prevItem->nextItem != theItem; prevItem = prevItem->nextItem);
				prevItem->nextItem = theItem->nextItem;
				free(theItem);
				theItem = prevItem->nextItem;
			} else {
				amuletOnLevel = true;
			}
		}
	}
	if (amuletOnLevel) {
		for (monst = monsters->nextCreature; monst != NULL; monst = monst->nextCreature) {
			if (monst->carriedItem && monst->carriedItem->category == AMULET) {
				free(monst->carriedItem);
				monst->carriedItem = NULL;
			}
		}
	}
	
	for (theItem = floorItems->nextItem; theItem != NULL; theItem = theItem->nextItem) {
		restoreItem(theItem);
	}
	
	mapToStairs = allocDynamicGrid();
	mapToPit = allocDynamicGrid();
	calculateDistances(mapToStairs, player.xLoc, player.yLoc, 0, &player);
	calculateDistances(mapToStairs, levels[rogue.depthLevel - 1].playerExitedVia[0], levels[rogue.depthLevel - 1].playerExitedVia[1], 0, &player);
	for (monst = monsters->nextCreature; monst != NULL; monst = monst->nextCreature) {
		restoreMonster(monst, mapToStairs, mapToPit);
	}
	freeDynamicGrid(mapToStairs);
	freeDynamicGrid(mapToPit);
	
	if (!levels[rogue.depthLevel-1].visited) {
		populateItems(upLoc[0], upLoc[1]);
		populateMonsters();
	}
	
}

// fills 2-member short array "loc" with the coordinates of a random cell with
// no creatures, items or stairs and with either a matching liquid and dungeon type
// or at least one layer of type terrainType.
// A dungeon, liquid type of -1 will match anything.
boolean randomMatchingLocation(short *loc, short dungeonType, short liquidType, short terrainType) {
	short failsafeCount = 0;
	do {
		failsafeCount++;
		loc[0] = rand_range(0, DCOLS - 1);
		loc[1] = rand_range(0, DROWS - 1);
	} while (failsafeCount < 500 && ((terrainType >= 0 && !cellHasTerrainType(loc[0], loc[1], terrainType))
									 || (((dungeonType >= 0 && pmap[loc[0]][loc[1]].layers[DUNGEON] != dungeonType)
										  || (liquidType >= 0 && pmap[loc[0]][loc[1]].layers[LIQUID] != liquidType))
										 && terrainType < 0)
									 || pmap[loc[0]][loc[1]].flags &
									 (HAS_PLAYER | HAS_MONSTER | HAS_DOWN_STAIRS | HAS_UP_STAIRS | HAS_ITEM)
									 || (terrainType < 0 && !(tileCatalog[dungeonType].flags & OBSTRUCTS_ITEMS)
										 && cellHasTerrainFlag(loc[0], loc[1], OBSTRUCTS_ITEMS))));
	if (failsafeCount >= 500) {
		return false;
	}
	return true;
}

void logLevel() {
	
	short i, j;
	char cellChar;
	
	printf("\n\n *** LEVEL %i ***", rogue.depthLevel);
	
	printf("\n");
	printf("    ");
	for (i=0; i<DCOLS; i++) {
		if (i % 10 == 0) {
			printf("%i", i / 10);
		} else {
			printf(" ");
		}
	}
	printf("\n");
	printf("    ");
	for (i=0; i<DCOLS; i++) {
		printf("%i", i % 10);
	}
	printf("\n");
	for( j=0; j<DROWS; j++ ) {
		if (j < 10) {
			printf(" ");
		}
		printf("%i: ", j);
		for( i=0; i<DCOLS; i++ ) {
			if (pmap[i][j].layers[LIQUID]) {
				cellChar = '0';
			} else {
				switch (pmap[i][j].layers[DUNGEON]) {
					case GRANITE:
						cellChar = ' ';
						break;
					case PERM_WALL:
						cellChar = '#';
						break;
					case LEFT_WALL:
					case RIGHT_WALL:
						cellChar = '|';
						break;
					case TOP_WALL:
					case BOTTOM_WALL:
						cellChar = '-';
						break;
					case FLOOR:
						cellChar = '.'; //(tmap[i][j].connected == -1 ? '&' : '0' + tmap[i][j].connected);
						break;
					case DOOR:
						cellChar = '+';
						break;
					case TORCH_WALL:
						cellChar = '*';
						break;
					case UP_STAIRS:
						cellChar = '<';
						break;
					case DOWN_STAIRS:
						cellChar = '>';
						break;
					default: // error
						cellChar = '&';
						break;
				}
			}
			printf("%c", cellChar);
		}
		printf("\n");
	}
	printf("\n");
}

void logBuffer(char array[DCOLS][DROWS]) {
//void logBuffer(short **array) {	
	short i, j;
	
	printf("\n");
	printf("    ");
	for (i=0; i<DCOLS; i++) {
		if (i % 10 == 0) {
			printf("%i", i / 10);
		} else {
			printf(" ");
		}
	}
	printf("\n");
	printf("    ");
	for (i=0; i<DCOLS; i++) {
		printf("%i", i % 10);
	}
	printf("\n");
	for( j=0; j<DROWS; j++ ) {
		if (j < 10) {
			printf(" ");
		}
		printf("%i: ", j);
		for( i=0; i<DCOLS; i++ ) {
			if (array[i][j] == 0) {
				printf(" ");
			} else if (array[i][j] == 1) {
				printf(".");
			} else if (array[i][j] == 2) {
				printf("o");
			} else if (array[i][j] == 3) {
				printf("#");
			} else {
				printf("%i", array[i][j]);
			}
		}
		printf("\n");
	}
	printf("\n");
}
