/*
 *  Include Globals.h
 *  Brogue
 *
 *  Created by Brian Walker on 2/8/09.
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

extern tcell tmap[DCOLS][DROWS];						// grids with info about the map
extern pcell pmap[DCOLS][DROWS];						// grids with info about the map
extern cellDisplayBuffer displayBuffer[COLS][ROWS];
extern short terrainRandomValues[DCOLS][DROWS][8];
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
extern creature *graveyard;
extern item *floorItems;
extern item *packItems;
extern lightSource *lights;
extern room *rooms;
extern waypoint waypoints[MAX_WAYPOINTS];
extern short numberOfWaypoints;
extern levelProfile *thisLevelProfile;
extern char displayedMessage[MESSAGE_LINES][COLS];
extern boolean messageConfirmed[3];
extern char combatText[COLS];
extern long levelPoints[MAX_EXP_LEVEL];
extern short brogueCursorX, brogueCursorY;

// basic colors
extern color white;
extern color red;
extern color blue;
extern color darkRed;
extern color gray;
extern color darkGray;
extern color black;
extern color yellow;
extern color teal;
extern color purple;
extern color brown;
extern color green;
extern color darkGreen;
extern color orange;
extern color darkOrange;
extern color pink;
extern color tanColor;
extern color sunlight;
extern color rainbow;

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
extern color fireForeColor;

// light colors
extern color minersLightColor;
extern color minersLightStartColor;
extern color minersLightEndColor;
extern color torchLightColor;
extern color magicLightColor;
extern color deepWaterLightColor;
extern color redFlashColor;

extern color memoryColor;
extern color memoryOverlay;
extern color magicMapColor;
extern color clairvoyanceColor;

extern color colorDim25;

extern color blueBar;
extern color redBar;
extern color hiliteColor;

extern color playerInShadow;
extern color playerInLight;

// other colors
extern color centipedeColor;
extern color confusionGasColor;
extern color lightningColor;

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

extern short goodItems[NUMBER_GOOD_ITEMS][2];

extern char monsterBehaviorFlagDescriptions[32][COLS];
extern char monsterAbilityFlagDescriptions[32][COLS];
extern char monsterBookkeepingFlagDescriptions[32][COLS];
