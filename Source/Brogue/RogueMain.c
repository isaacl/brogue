/*
 *  RogueMain.c
 *  Brogue
 *
 *  Created by Brian Walker on 12/26/08.
 *  Copyright 2008-2009. All rights reserved.
 *
 */

#include "Rogue.h"
#include "IncludeGlobals.h"
#include <math.h>
#include <time.h>

void rogueMain() {
	rogueEvent theEvent;
	
	initializeRogue();
	startLevel(rogue.depthLevel, 1); // descending into level 1
	while(!rogue. gameHasEnded) {
		nextKeyOrMouseEvent(&theEvent);
		if (theEvent.eventType == KEYSTROKE) {
			executeKeystroke(theEvent.param1, theEvent.controlKey); // the keypress and the state of the control key respectively
		} else if (theEvent.eventType == MOUSE_UP) {
			executeMouseClick(&theEvent);
		}
	}
	updateDisplay();
	freeEverything();
}

void initializeRogue() {
	short i, j, k;
	item *theItem;
	
	// grab the random seeds first thing
	rogue.seed = time(NULL);
	
	// for debugging, uncomment the following line and insert desired level sequence ID:
	// rogue.seed = 1255926155;
	
	srandom(rogue.seed);	
	
	levels[0].upStairsLoc[0] = rand_range(2, DCOLS - 2);
	levels[0].upStairsLoc[1] = rand_range(2, DROWS - 2);
	
	for (i=0; i<101; i++) {
		levels[i].levelSeed = random();
		levels[i].monsters = NULL;
		levels[i].items = NULL;
		levels[i].roomStorage = NULL;
		levels[i].visited = false;
		levels[i].numberOfRooms = 0;
		do {
			levels[i].downStairsLoc[0] = rand_range(2, DCOLS - 2);
			levels[i].downStairsLoc[1] = rand_range(2, DROWS - 2);
		} while (distanceBetween(levels[i].upStairsLoc[0], levels[i].upStairsLoc[1],
								 levels[i].downStairsLoc[0], levels[i].downStairsLoc[1]) < DCOLS / 3);
		if (i < 100) {
			levels[i+1].upStairsLoc[0] = levels[i].downStairsLoc[0];
			levels[i+1].upStairsLoc[1] = levels[i].downStairsLoc[1];
		}
	}
	
	rogue.gameHasEnded = false;
	
	for (i=0; i<COLS; i++) {
		for (j=0; j<ROWS; j++) {
			displayBuffer[i][j].character = 0;
			for (k=0; k<3; k++) {
				displayBuffer[i][j].foreColorComponents[k] = 0;
				displayBuffer[i][j].backColorComponents[k] = 0;
			}
			plotCharWithColor(' ', i, j, black, black);
		}
	}
	
	// preload funky characters so the game doesn't pause the first time they are displayed
	plotCharWithColor(FLOOR_CHAR, 0, 0, black, black);
	plotCharWithColor(CHASM_CHAR, 0, 1, black, black);
	plotCharWithColor(TRAP_CHAR, 0, 2, black, black);
	plotCharWithColor(FIRE_CHAR, 0, 3, black, black);
	plotCharWithColor(AMULET_CHAR, 0, 4, black, black);
	plotCharWithColor(RING_CHAR, 0, 5, black, black);
	plotCharWithColor(WEAPON_CHAR, 0, 6, black, black);
	
	updateDisplay();
	pauseForMilliseconds(1);
	
	for (i=0; i<NUMBER_MONSTER_KINDS; i++) {
		monsterCatalog[i].monsterID = i;
	}
	
	shuffleFlavors();
	
	// seed the stacks
	floorItems = (item *) malloc(sizeof(item));
	floorItems->nextItem = NULL;
	packItems = (item *) malloc(sizeof(item));
	packItems->nextItem = NULL;
	monsters = (creature *) malloc(sizeof(creature));
	monsters->nextCreature = NULL;
	lights = (lightSource *) malloc(sizeof(lightSource));
	lights->nextLight = NULL;
	rooms = (room *) malloc(sizeof(room));
	rooms->nextRoom = NULL;
	
	safetyMap = allocDynamicGrid();
	
	// initialize the player
	
	player.info = monsterCatalog[0];
	player.movementSpeed = player.info.movementSpeed;
	player.attackSpeed = player.info.attackSpeed;
	clearStatus(&player);
	player.carriedItem = NULL;
	player.statusLight = NULL;
	player.status.nutrition = player.maxStatus.nutrition = STOMACH_SIZE;
	player.currentHP = player.info.maxHP;
	player.creatureState = MONSTER_ALLY;
	
	rogue.depthLevel = 1;
	rogue.scentTurnNumber = 1000;
	rogue.turnNumber = 0;
	rogue.foodSpawned = 0;
	rogue.gold = 0;
	rogue.disturbed = false;
	rogue.justRested = false;
	rogue.easyMode = false;
	rogue.inWater = false;
	rogue.experienceLevel = 1;
	rogue.experience = 0;
	rogue.maxStrength = 12;
	rogue.currentStrength = 12;
	rogue.weapon = NULL;
	rogue.armor = NULL;
	rogue.ringLeft = NULL;
	rogue.ringRight = NULL;
	rogue.monsterSpawnFuse = rand_range(125, 175);
	rogue.ticksTillUpdateEnvironment = 100;
	
	rogue.clairvoyance = rogue.aggravating = rogue.regenerationBonus
	= rogue.stealthBonus = rogue.transference = 0;
	rogue.lightMultiplier = 1;
	
	theItem = generateItem(FOOD, RATION);
	theItem = addItemToPack(theItem);
	
	theItem = generateItem(WEAPON, DAGGER);
	theItem->enchant1 = theItem->enchant2 = 0;
	theItem->flags &= ~ITEM_CURSED;
	identify(theItem);
	theItem = addItemToPack(theItem);
	equipItem(theItem, false);
	
	theItem = generateItem(WEAPON, DART);
	theItem->enchant1 = theItem->enchant2 = 0;
	theItem->quantity = 15;
	theItem->flags &= ~ITEM_CURSED;
	identify(theItem);
	theItem = addItemToPack(theItem);
	
	theItem = generateItem(ARMOR, LEATHER_ARMOR);
	theItem->enchant1 = 0;
	theItem->flags &= ~ITEM_CURSED;
	identify(theItem);
	theItem = addItemToPack(theItem);
	equipItem(theItem, false);
	
	DEBUG {
		theItem = generateItem(RING, RING_CLAIRVOYANCE);
		theItem->enchant1 = 50;
		theItem->flags &= ~ITEM_CURSED;
		identify(theItem);
		theItem = addItemToPack(theItem);
		
		theItem = generateItem(RING, RING_STEALTH);
		theItem->enchant1 = 10;
		theItem->flags &= ~ITEM_CURSED;
		identify(theItem);
		theItem = addItemToPack(theItem);
		
		theItem = generateItem(POTION, POTION_TELEPATHY);
		theItem->quantity = 2;
		theItem->flags &= ~ITEM_CURSED;
		identify(theItem);
		theItem = addItemToPack(theItem);
		
		theItem = generateItem(STAFF, STAFF_FIRE);
		theItem->enchant1 = 3;
		theItem->charges = 300;
		theItem->flags &= ~ITEM_CURSED;
		identify(theItem);
		theItem = addItemToPack(theItem);
		
		theItem = generateItem(STAFF, STAFF_OBSTRUCTION);
		theItem->enchant1 = 10;
		theItem->charges = 300;
		theItem->flags &= ~ITEM_CURSED;
		identify(theItem);
		theItem = addItemToPack(theItem);
		
		theItem = generateItem(STAFF, STAFF_LIGHTNING);
		theItem->enchant1 = theItem->charges = 30;
		theItem->flags &= ~ITEM_CURSED;
		identify(theItem);
		theItem = addItemToPack(theItem);
		
		theItem = generateItem(STAFF, STAFF_BLINKING);
		theItem->enchant1 = theItem->charges = 30;
		theItem->flags &= ~ITEM_CURSED;
		identify(theItem);
		theItem = addItemToPack(theItem);
		
		theItem = generateItem(STAFF, STAFF_TUNNELING);
		theItem->enchant1 = theItem->charges = 50;
		theItem->flags &= ~ITEM_CURSED;
		identify(theItem);
		theItem = addItemToPack(theItem);
		
		theItem = generateItem(STAFF, STAFF_ENTRANCEMENT);
		theItem->enchant1 = 5;
		theItem->charges = 300;
		theItem->flags &= ~ITEM_CURSED;
		identify(theItem);
		theItem = addItemToPack(theItem);
	}
	clearMessage();
	message("Hello, and welcome to the Dungeons of Doom!  Press <?> for help.", true, false);
	message("The doors to the dungeon slam shut behind you.", false, false); 
}

void startLevel(short oldLevelNumber, short stairDirection) {
	long oldSeed;
	item *theItem, *previousItem;
	lightSource *theLight, *theLight2;
	short loc[2], i, j, x, y;
	boolean isAlreadyAmulet = false;
	creature *monst;
	enum dungeonLayers layer;
	unsigned long timeAway;
	short **mapToStairs;
	boolean connectingStairsDiscovered;
	
	if (stairDirection == 0) { // fallen
		connectingStairsDiscovered = (pmap[rogue.downLoc[0]][rogue.downLoc[1]].flags & DISCOVERED ? true : false);
	}
	
	if (stairDirection != 0 && oldLevelNumber != rogue.depthLevel) {
		mapToStairs = allocDynamicGrid();
		calculateDistances(mapToStairs, player.xLoc, player.yLoc, 0, &player);
		for (monst = monsters->nextCreature; monst != NULL; monst = monst->nextCreature) {
			x = monst->xLoc;
			y = monst->yLoc;
			if ((monst->creatureState == MONSTER_TRACKING_SCENT || monst->creatureState == MONSTER_ALLY)
				&& !(monst->info.flags & MONST_WILL_NOT_USE_STAIRS)
				&& !(cellHasTerrainFlag(x, y, OBSTRUCTS_PASSABILITY))
				&& !monst->status.entranced
				&& !monst->status.paralyzed
				&& mapToStairs[monst->xLoc][monst->yLoc] < 30000) {
				monst->status.entersLevelIn = mapToStairs[monst->xLoc][monst->yLoc] * monst->movementSpeed / 100;
				monst->bookkeepingFlags |= (stairDirection == 1 ? MONST_APPROACHING_DOWNSTAIRS : MONST_APPROACHING_UPSTAIRS);
			}
		}
		freeDynamicGrid(mapToStairs);
	}
	
	// save or clear old level rooms and lights
	for (monst = monsters->nextCreature; monst != NULL; monst = monst->nextCreature) {
		if (monst->mapToMe) {
			freeDynamicGrid(monst->mapToMe);
			monst->mapToMe = NULL;
		}
	}
	levels[oldLevelNumber-1].monsters = monsters->nextCreature;
	levels[oldLevelNumber-1].items = floorItems->nextItem;
	theLight = lights->nextLight;
	lights->nextLight = NULL;
	while (theLight) {
		theLight2 = theLight->nextLight;
		free(theLight);
		theLight = theLight2;
	}
	
	levels[oldLevelNumber - 1].numberOfRooms = numberOfRooms;
	levels[oldLevelNumber - 1].roomStorage = rooms->nextRoom;
	rooms->nextRoom = NULL;
	
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			for (layer = 0; layer < NUMBER_TERRAIN_LAYERS; layer++) {
				levels[oldLevelNumber - 1].mapStorage[i][j].layers[layer] = pmap[i][j].layers[layer];
			}
			levels[oldLevelNumber - 1].mapStorage[i][j].volume = pmap[i][j].volume;
			levels[oldLevelNumber - 1].mapStorage[i][j].flags = (pmap[i][j].flags & PERMANENT_TILE_FLAGS);
		}
	}
	
	levels[oldLevelNumber - 1].awaySince = rogue.turnNumber;
	
	//	Prepare the new level
	
	player.statusLight = NULL;
	
	if (!levels[rogue.depthLevel - 1].visited) { // level has not already been visited
		// generate new level
		oldSeed = random();
		srandom(levels[rogue.depthLevel - 1].levelSeed);
		thisLevelProfile = &(levelProfileCatalog[rand_range(0, NUMBER_LEVEL_PROFILES - 1)]);
		digDungeon();
		initializeLevel(rogue.depthLevel, stairDirection);
		if (rogue.depthLevel >= AMULET_LEVEL && !numberOfMatchingPackItems(AMULET, 0, 0, false)
			&& levels[rogue.depthLevel-1].visited == false) {
			for (theItem = floorItems->nextItem; theItem != NULL; theItem = theItem->nextItem) {
				if (theItem->category & AMULET) {
					isAlreadyAmulet = true;
					break;
				}
			}
			if (!isAlreadyAmulet) {
				placeItem(generateItem(AMULET, 0), 0, 0);
			}
		}
		srandom(oldSeed);
		
		//logLevel();
		
	} else { // level has already been visited
		
		// restore level
		timeAway = rogue.turnNumber - levels[rogue.depthLevel - 1].awaySince;
		numberOfRooms = levels[rogue.depthLevel - 1].numberOfRooms;
		rooms->nextRoom = levels[rogue.depthLevel - 1].roomStorage;
		
		for (i=0; i<DCOLS; i++) {
			for (j=0; j<DROWS; j++) {
				for (layer = 0; layer < NUMBER_TERRAIN_LAYERS; layer++) {
					pmap[i][j].layers[layer] = levels[rogue.depthLevel - 1].mapStorage[i][j].layers[layer];
				}
				pmap[i][j].volume = levels[rogue.depthLevel - 1].mapStorage[i][j].volume;
				pmap[i][j].flags = (levels[rogue.depthLevel - 1].mapStorage[i][j].flags & PERMANENT_TILE_FLAGS);
			}
		}
		
		rogue.downLoc[0] = levels[rogue.depthLevel - 1].downStairsLoc[0];
		rogue.downLoc[1] = levels[rogue.depthLevel - 1].downStairsLoc[1];
		rogue.upLoc[0] = levels[rogue.depthLevel - 1].upStairsLoc[0];
		rogue.upLoc[1] = levels[rogue.depthLevel - 1].upStairsLoc[1];
		
		if (stairDirection == 1) { // heading downward
			player.xLoc = rogue.upLoc[0];
			player.yLoc = rogue.upLoc[1];
			pmap[player.xLoc][player.yLoc].flags |= HAS_PLAYER;
		} else if (stairDirection == -1) { // heading upward
			player.xLoc = rogue.downLoc[0];
			player.yLoc = rogue.downLoc[1];
			pmap[player.xLoc][player.yLoc].flags |= HAS_PLAYER;
		}
		
		monsters->nextCreature = levels[rogue.depthLevel - 1].monsters;
		floorItems->nextItem = levels[rogue.depthLevel - 1].items;
		
		if (numberOfMatchingPackItems(AMULET, 0, 0, false)) {
			isAlreadyAmulet = true;
		}
		
		for (theItem = floorItems->nextItem; theItem != NULL; theItem = theItem->nextItem) {
			if (theItem->category & AMULET) {
				if (isAlreadyAmulet) {
					
					pmap[theItem->xLoc][theItem->yLoc].flags &= ~(HAS_ITEM | ITEM_DETECTED);
					
					for (previousItem = floorItems; previousItem->nextItem != theItem; previousItem = previousItem->nextItem);
					previousItem->nextItem = theItem->nextItem;
					
					deleteItem(theItem);
					
					continue;
				}
				isAlreadyAmulet = true;
			}
			restoreItem(theItem);
		}
		
		mapToStairs = allocDynamicGrid();
		calculateDistances(mapToStairs, player.xLoc, player.yLoc, 0, &player);
		for (monst = monsters->nextCreature; monst != NULL; monst = monst->nextCreature) {
			restoreMonster(monst, mapToStairs);
		}
		freeDynamicGrid(mapToStairs);
		
		for (i = 0; i < timeAway && i < 100; i++) {
			updateEnvironment();
		}
	}
	
	setUpWaypoints();
	
	levels[rogue.depthLevel-1].visited = true;
	
	if (stairDirection == 0) { // fell into the level
		
		getQualifyingLocNear(loc, player.xLoc, player.yLoc, DCOLS,
							 (OBSTRUCTS_PASSABILITY | LAVA_INSTA_DEATH | TRAP_DESCENT | IS_DF_TRAP),
							 (HAS_MONSTER | HAS_ITEM | HAS_UP_STAIRS | HAS_DOWN_STAIRS), false);
		player.xLoc = loc[0];
		player.yLoc = loc[1];
		pmap[player.xLoc][player.yLoc].flags |= HAS_PLAYER;
		
		if (connectingStairsDiscovered) {
			pmap[rogue.upLoc[0]][rogue.upLoc[1]].flags |= DISCOVERED;
		}
	}
	
	// updateStatBar(DEPTH_STAT);
	
	rogue.minersLight = newLight(&lightCatalog[MINERS_LIGHT], 0, 0, &player);
	rogue.minersLight->lightRadius.lowerBound = rogue.minersLight->lightRadius.upperBound
	= 2 + (DCOLS - 1) * pow(0.85, rogue.depthLevel) * rogue.lightMultiplier;
	minersLightColor = minersLightStartColor;
	applyColorAverage(&minersLightColor, &minersLightEndColor, min(100, max(0, rogue.depthLevel * 7)));
	
	wallBackColor = wallBackColorStart;
	applyColorAverage(&wallBackColor, &wallBackColorEnd, min(100, max(0, rogue.depthLevel * 100 / 26)));
	//wallBackColor.red = 50 + min(26, rogue.depthLevel); // walls turn redder as you descend
	
	if (player.status.burning) {
		player.statusLight = newLight(&lightCatalog[BURNING_CREATURE_LIGHT], 0, 0, &player);
	}
	
	updateRingBonuses(); // also tinkers with minerslight
	
	updateVision();
	displayLevel();
	refreshSideBar(NULL);
}

void freeEverything() {
	short i;
	creature *monst, *monst2;
	item *theItem, *theItem2;
	
	freeDynamicGrid(safetyMap);
	
	for (i=0; i<101; i++) {
		for (monst = levels[1].monsters; monst != NULL; monst = monst2) {
			monst2 = monst->nextCreature;
			if (monst->mapToMe) {
				freeDynamicGrid(monst->mapToMe);
			}
			free(monst);
		}
		levels[1].monsters = NULL;
		for (theItem = levels[1].items; theItem != NULL; theItem = theItem2) {
			theItem2 = theItem->nextItem;
			free(theItem);
		}
		levels[1].items = NULL;
	}
}

void gameOver(char *killedBy, boolean useCustomPhrasing) {
	//short i, j;
	char buf[COLS];
	rogueHighScoresEntry theEntry;
	cellDisplayBuffer dbuf[COLS][ROWS];
	
	message("You die...", true, true);
	
	DEBUG {
		message("...but then you get better.", true, false);
		player.currentHP = player.info.maxHP;
		if (player.status.nutrition < 10) {
			player.status.nutrition = STOMACH_SIZE;
		}
		return;
	}
	
	copyDisplayBuffer(dbuf, displayBuffer);
	funkyFade(dbuf, &black, 0, 30, player.xLoc + STAT_BAR_WIDTH, player.yLoc + 1, false);
	/*for (i=0; i<COLS; i++) {
		for (j=0; j<ROWS; j++) {
			plotCharWithColor(' ', i, j, black, black);
		}
	}*/
	
	if (useCustomPhrasing) {
		sprintf(buf, "%s on level %i%s.", killedBy, rogue.depthLevel,
				(numberOfMatchingPackItems(AMULET, 0, 0, false) > 0 ? ", amulet in hand" : ""));
	} else {
		sprintf(buf, "Killed by a%s %s on level %i%s.", (isVowel(killedBy[0]) ? "n" : ""), killedBy,
				rogue.depthLevel, (numberOfMatchingPackItems(AMULET, 0, 0, false) > 0 ? ", amulet in hand" : ""));
	}
	
	theEntry.score = rogue.gold;
	if (rogue.easyMode) {
		theEntry.score /= 10;
	}
	strcpy(theEntry.description, buf);
	
	printString(buf, ((COLS - strlen(buf)) / 2), (ROWS / 2), &gray, &black, 0);
	displayMoreSign();
	waitForAcknowledgment();
	
	printHighScores(saveHighScore(theEntry));
	
	rogue.gameHasEnded = true;
}

unsigned long itemValue(item *theItem) {
	switch (theItem->category) {
		case FOOD:
			return foodTable[theItem->kind].marketValue * theItem->quantity;
			break;
		case WEAPON:
			return weaponTable[theItem->kind].marketValue * theItem->quantity
			* (1 + 0.15 * (theItem->enchant1 + theItem->enchant2 + (theItem->flags & ITEM_PROTECTED ? 1 : 0)));
			break;
		case ARMOR:
			return armorTable[theItem->kind].marketValue * theItem->quantity
			* (1 + 0.15 * (theItem->enchant1 + (theItem->flags & ITEM_PROTECTED ? 1 : 0)));
			break;
		case SCROLL:
			return scrollTable[theItem->kind].marketValue * theItem->quantity;
			break;
		case POTION:
			return potionTable[theItem->kind].marketValue * theItem->quantity;
			break;
		case STAFF:
			return staffTable[theItem->kind].marketValue * theItem->quantity
			* (1 + 0.15 * (theItem->enchant1 - 1));
			break;
		case WAND:
			return wandTable[theItem->kind].marketValue * theItem->quantity;
			break;
		case RING:
			return ringTable[theItem->kind].marketValue * theItem->quantity
			* (1 + 0.15 * (theItem->enchant1 - 1));
			break;
		case AMULET:
			return 10000;
			break;
		case GEM:
			return 5000 * theItem->quantity;
			break;
		default:
			return 0;
			break;
	}
}

void victory() {
	char buf[DCOLS];
	item *theItem;
	short i, gemCount = 0;
	unsigned long totalValue = 0;
	rogueHighScoresEntry theEntry;
	boolean qualified;
	cellDisplayBuffer dbuf[COLS][ROWS];
	
	clearMessage();
	message("You are bathed in sunlight as you throw open the heavy doors.", true, false);
	
	copyDisplayBuffer(dbuf, displayBuffer);
	funkyFade(dbuf, &white, 0, 100, player.xLoc + STAT_BAR_WIDTH, player.yLoc + 1, false);
	displayMoreSign();
	waitForAcknowledgment();
	printString("Congratulations; you have escaped from the Dungeons of Doom!     ", STAT_BAR_WIDTH, 0, &black, &white, 0);
	displayMoreSign();
	waitForAcknowledgment();
	
	clearDisplayBuffer(dbuf);
	
	strcpy(displayedMessage, "You sell your treasures and live out your days in fame and glory.");
	
	//	for (i=0; i<COLS; i++) {
	//		for (j=0; j<ROWS; j++) {
	//			plotCharWithColor(' ', i, j, black, black);
	//		}
	//	}
	
	printString(displayedMessage, STAT_BAR_WIDTH, 0, &white, &black, dbuf);
	
	printString("Gold", STAT_BAR_WIDTH + 2, 2, &white, &black, dbuf);
	sprintf(buf, "%i", rogue.gold);
	printString(buf, STAT_BAR_WIDTH + 60, 2, &yellow, &black, dbuf);
	totalValue += rogue.gold;
	//updateDisplay();
	
	for (i = 2, theItem = packItems->nextItem; theItem != NULL; theItem = theItem->nextItem, i++) {
		if (theItem->category & GEM) {
			gemCount += theItem->quantity;
		}
		identify(theItem);
		nameOfItem(theItem, buf, true, true);
		upperCase(buf);
		printString(buf, STAT_BAR_WIDTH + 2, min(ROWS-1, i + 1), &white, &black, dbuf);
		sprintf(buf, "%i", max(0, itemValue(theItem)));
		printString(buf, STAT_BAR_WIDTH + 60, min(ROWS-1, i + 1), &yellow, &black, dbuf);
		totalValue += max(0, itemValue(theItem));
	}
	i++;
	printString("TOTAL:", STAT_BAR_WIDTH + 2, min(ROWS-1, i + 1), &teal, &black, dbuf);
	sprintf(buf, "%i", totalValue);
	printString(buf, STAT_BAR_WIDTH + 60, min(ROWS-1, i + 1), &teal, &black, dbuf);
	
	funkyFade(dbuf, &white, 0, 15, COLS/2, ROWS/2, true);	
	
	if (gemCount == 0) {
		strcpy(theEntry.description, "Escaped the Dungeons of Doom!");
	} else if (gemCount == 1) {
		strcpy(theEntry.description, "Escaped the Dungeons of Doom with a lumenstone!");
	} else {
		sprintf(theEntry.description, "Escaped the Dungeons of Doom with %i lumenstones!", gemCount);
	}
	
	theEntry.score = totalValue;
	
	if (rogue.easyMode) {
		theEntry.score /= 10;
	}
	
	if (!DEBUGGING) {
		qualified = saveHighScore(theEntry);
	} else {
		qualified = false;
	}
	
	displayMoreSign();
	waitForAcknowledgment();
	
	printHighScores(qualified);
	
	rogue.gameHasEnded = true;
}

void enableEasyMode() {
	if (rogue.easyMode) {
		message("Alas, all hope of salvation is lost. You shed scalding tears at your plight.", true, false);
		return;
	}
	message("A dark presence surrounds you, whispering promises of stolen power.", true, false);
	if (confirm("Succumb to demonic temptation? (Y/N)")) {
		message("An ancient and terrible evil burrows into your willing flesh!", true, true);
		player.info.displayChar = '&';
		rogue.easyMode = true;
		refreshDungeonCell(player.xLoc, player.yLoc);
		refreshSideBar(NULL);
		message("Wracked by spasms, your body contorts into an ALL-POWERFUL AMPERSAND!!!", true, false);
		message("You have a feeling you will take half as much damage from now on.", true, false);
	} else {
		message("The evil dissipates, hissing, from the air around you.", true, false);
	}
}

// Get a random int between lowerBound and upperBound, inclusive, with uniform probability distribution
long rand_range(long lowerBound, long upperBound) {
	if (upperBound <= lowerBound) {
		return lowerBound;
	}
	return ((random() % (upperBound - lowerBound + 1)) + lowerBound);
}

short randClump(randomRange theRange) {
	return randClumpedRange(theRange.lowerBound, theRange.upperBound, theRange.clumpFactor);
}

// Get a random int between lowerBound and upperBound, inclusive, with probability distribution
// affected by clumpFactor.
short randClumpedRange(short lowerBound, short upperBound, short clumpFactor) {
	if (upperBound <= lowerBound) {
		return lowerBound;
	}
	if (clumpFactor <= 1) {
		return rand_range(lowerBound, upperBound);
	}
	
	short i, total = 0, numSides = (upperBound - lowerBound) / clumpFactor;
	
	for(i=0; i < (upperBound - lowerBound) % clumpFactor; i++) {
		total += rand_range(0, numSides + 1);
	}
	
	for(; i< clumpFactor; i++) {
		total += rand_range(0, numSides);
	}
	
	return (total + lowerBound);
}

// Get a random int between lowerBound and upperBound, inclusive
boolean rand_percent(short percent) {
	return ((random() % 100) < max(0, min(100, percent)));
}