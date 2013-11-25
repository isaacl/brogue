/*
 *  Include Globals.h
 *  Brogue
 *
 *  Created by Brian Walker on 2/8/09.
 *  Copyright 2008-2009. All rights reserved.
 *
 */

extern tcell tmap[DCOLS][DROWS];						// grids with info about the map
extern pcell pmap[DCOLS][DROWS];						// grids with info about the map
extern cellDisplayBuffer displayBuffer[COLS][ROWS];
extern char buffer[DCOLS][DROWS];						// used in cave generation
short **safetyMap;										// used to help monsters flee

extern short listOfWallsX[4][DROWS*DCOLS];
extern short listOfWallsY[4][DROWS*DCOLS];
extern short numberOfWalls[4];
extern short nbDirs[8][2];
extern short numberOfRooms;
extern levelData levels[101];
extern creature player;
extern playerCharacter rogue;
extern creature *monsters;
extern item *floorItems;
extern item *packItems;
extern lightSource *lights;
extern room *rooms;
extern waypoint waypoints[MAX_WAYPOINTS];
extern short numberOfWaypoints;
extern levelProfile *thisLevelProfile;
extern boolean messageDisplayed;
extern char displayedMessage[COLS];
extern char combatText[COLS];
extern long levelPoints[MAX_EXP_LEVEL];

// basic colors
extern color white;
extern color red;
extern color darkRed;
extern color gray;
extern color darkGray;
extern color black;
extern color yellow;
extern color teal;
extern color purple;
extern color brown;
extern color green;
extern color orange;
extern color tanColor;
extern color sunlight;

// tile colors
extern color undiscoveredColor;

extern color wallForeColor;
extern color wallBackColorStart;
extern color wallBackColorEnd;
extern color wallBackColor;
extern color graniteBackColor;
extern color floorForeColor;
extern color floorBackColor;
extern color doorForeColor;
extern color doorBackColor;

extern color deepWaterForeColor;
extern color deepWaterBackColor;
extern color shallowWaterForeColor;
extern color shallowWaterBackColor;
extern color mudForeColor;
extern color mudBackColor;
extern color chasmForeColor;
extern color chasmEdgeBackColor;
extern color lavaForeColor;

// light colors
extern color minersLightColor;
extern color minersLightStartColor;
extern color minersLightEndColor;
extern color torchLightColor;
extern color magicLightColor;
extern color deepWaterLightColor;

extern color memoryColor;
extern color magicMapColor;
extern color clairvoyanceColor;

extern color colorDim25;

extern color blueBar;
extern color redBar;
extern color hiliteColor;

extern color playerInShadow;
extern color playerInLight;

extern floorTileType tileCatalog[NUMBER_TILETYPES];

extern dungeonFeature dungeonFeatureCatalog[NUMBER_DUNGEON_FEATURES];

extern lightSource lightCatalog[NUMBER_LIGHT_KINDS];

extern creatureType monsterCatalog[NUMBER_MONSTER_KINDS];
extern monsterWords monsterText[NUMBER_MONSTER_KINDS];
extern hordeType hordeCatalog[NUMBER_HORDES];
extern levelProfile levelProfileCatalog[NUMBER_LEVEL_PROFILES];

color *boltColors[NUMBER_BOLT_KINDS];

// ITEMS
extern char itemTitles[NUMBER_SCROLL_KINDS][30];
extern char titlePhonemes[NUMBER_TITLE_PHONEMES][30];
extern char itemColors[NUMBER_ITEM_COLORS][30];
extern char itemWoods[NUMBER_ITEM_WOODS][30];
extern char itemMetals[NUMBER_ITEM_METALS][30];
extern char itemGems[NUMBER_ITEM_GEMS][30];
extern itemTable foodTable[NUMBER_FOOD_KINDS];
extern itemTable weaponTable[NUMBER_WEAPON_KINDS];
extern itemTable armorTable[NUMBER_ARMOR_KINDS];
extern itemTable scrollTable[NUMBER_SCROLL_KINDS];
extern itemTable potionTable[NUMBER_POTION_KINDS];
extern itemTable wandTable[NUMBER_WAND_KINDS];
extern itemTable staffTable[NUMBER_STAFF_KINDS];
extern itemTable ringTable[NUMBER_RING_KINDS];