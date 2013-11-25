/*
 *  Globals.c
 *  Brogue
 *
 *  Created by Brian Walker on 1/10/09.
 *  Copyright 2008-2009. All rights reserved.
 *
 */

#include "Rogue.h"


tcell tmap[DCOLS][DROWS];					// grids with info about the map
pcell pmap[DCOLS][DROWS];
cellDisplayBuffer displayBuffer[COLS][ROWS];	// used to optimize plotCharWithColor
char buffer[DCOLS][DROWS];				// used in cave generation
short **safetyMap;						// used to help monsters flee
short listOfWallsX[4][DROWS*DCOLS];
short listOfWallsY[4][DROWS*DCOLS];
short numberOfWalls[4];
short nbDirs[8][2] = {{0,-1},{0,1},{-1,0},{1,0},{-1,-1},{-1,1},{1,-1},{1,1}};
short numberOfRooms;
short numberOfWaypoints;
levelData levels[101];
creature player;
playerCharacter rogue;
creature *monsters;
item *floorItems;
item *packItems;
lightSource *lights;
room *rooms;
waypoint waypoints[MAX_WAYPOINTS];
levelProfile *thisLevelProfile;
boolean messageDisplayed;
char displayedMessage[COLS];
char combatText[COLS];

long levelPoints[MAX_EXP_LEVEL] = {
10L,
20L,
40L,
80L,
160L,
320L,
640L,
1300L,
2600L,
5200L,
10000L,
20000L,
40000L,
80000L,
160000L,
320000L,
1000000L,
3333333L,
6666666L,
MAX_EXP,
99900000L
};

//								Red		Green	Blue	RedRand	GreenRand	BlueRand	Rand	Dances?
// basic colors
color white =					{100,	100,	100,	0,		0,			0,			0,		false};
color gray =					{50,	50,		50,		0,		0,			0,			0,		false};
color darkGray =				{30,	30,		30,		0,		0,			0,			0,		false};
color black =					{0,		0,		0,		0,		0,			0,			0,		false};
color yellow =					{100,	100,	0,		0,		0,			0,			0,		false};
color teal =					{30,	100,	100,	0,		0,			0,			0,		false};
color purple =					{100,	0,		100,	0,		0,			0,			0,		false};
color darkPurple =				{50,	0,		50,		0,		0,			0,			0,		false};
color brown =					{60,	40,		0,		0,		0,			0,			0,		false};
color green =					{0,		100,	0,		0,		0,			0,			0,		false};
color darkGreen =				{0,		50,		0,		0,		0,			0,			0,		false};
color orange =					{100,	50,		0,		0,		0,			0,			0,		false};
color blue =					{0,		0,		100,	0,		0,			0,			0,		false};
color darkBlue =				{0,		0,		50,		0,		0,			0,			0,		false};
color lightblue =				{40,	40,		100,	0,		0,			0,			0,		false};
color pink =					{100,	60,		66,		0,		0,			0,			0,		false};
color red  =					{100,	0,		0,		0,		0,			0,			0,		false};
color darkRed =					{50,	0,		0,		0,		0,			0,			0,		false};
color tanColor =				{80,	67,		15,		0,		0,			0,			0,		false};

// tile colors
color undiscoveredColor =		{0,		0,		0,		0,		0,			0,			0,		false};

color wallForeColor =			{7,		7,		7,		0,		0,			0,			0,		false};
color wallBackColor; // adjusted dynamically per depth level
color wallBackColorStart =		{60,	60,		60,		0,		0,			0,			0,		false};
color wallBackColorEnd =		{50,	45,		45,		0,		0,			0,			0,		false};
color graniteBackColor =		{10,	10,		10,		0,		0,			0,			0,		false};
color floorForeColor =			{40,	40,		40,		0,		0,			0,			0,		false};
color floorBackColor =			{2,		2,		10,		0,		0,			0,			0,		false};
color doorForeColor =			{70,	35,		15,		0,		0,			0,			0,		false};
color doorBackColor =			{30,	10,		5,		0,		0,			0,			0,		false};

color deepWaterForeColor =		{5,		5,		40,		0,		0,			10,			10,		true};
color deepWaterBackColor =		{5,		5,		55,		5,		5,			10,			10,		true};
color shallowWaterForeColor =	{40,	40,		90,		0,		0,			10,			10,		true};
color shallowWaterBackColor =	{30,	30,		80,		0,		0,			10,			10,		true};
color mudForeColor =			{15,	10,		5,		0,		0,			0,			0,		false};
color mudBackColor =			{30,	20,		10,		0,		0,			0,			0,		false};
color chasmForeColor =			{7,		7,		15,		0,		0,			0,			0,		false};
color chasmEdgeBackColor =		{5,		5,		25,		0,		0,			0,			0,		false};
color lavaForeColor =			{70,	20,		0,		15,		10,			0,			0,		true};
color lavaBackColor =			{70,	20,		0,		15,		10,			0,			0,		true};
color acidBackColor =			{20,	70,		30,		5,		15,			10,			0,		true};

color lightningColor =			{60,	80,		90,		40,		20,			10,			0,		true};
color fungusLightColor =		{2,		11,		11,		4,		3,			3,			0,		true};
color lavaLightColor =			{47,	13,		0,		10,		7,			0,			0,		true};
color deepWaterLightColor =		{10,	30,		100,	0,		30,			100,		0,		true};

color grassColor =				{30,	80,		30,		0,		0,			0,			0,		false};
color ashForeColor =			{20,	20,		20,		0,		0,			0,			0,		false};
color bonesForeColor =			{80,	80,		50,		5,		5,			15,			5,		false};
color ectoplasmColor =			{45,	20,		55,		25,		0,			25,			5,		false};
color forceFieldColor =			{0,		25,		25,		0,		25,			25,			0,		true};

// monster colors
color goblinColor =				{40,	30,		20,		0,		0,			0,			0,		false};
color jackalColor =				{60,	42,		27,		0,		0,			0,			0,		false};
color ogreColor =				{60,	25,		25,		0,		0,			0,			0,		false};
color eelColor =				{30,	12,		12,		0,		0,			0,			0,		false};
color toadColor =				{40,	65,		30,		0,		0,			0,			0,		false};
color trollColor =				{40,	60,		15,		0,		0,			0,			0,		false};
color centipedeColor =			{75,	25,		85,		0,		0,			0,			0,		false};
color dragonColor =				{20,	80,		15,		0,		0,			0,			0,		false};
color krakenColor =				{100,	55,		55,		0,		0,			0,			0,		false};
color salamanderColor =			{40,	10,		0,		8,		5,			0,			0,		true};
color gorgonColor =				{100,	0,		50,		0,		0,			0,			0,		false};
color pixieColor =				{60,	60,		60,		40,		40,			40,			0,		true};
color darPriestessColor =		{0,		50,		50,		0,		0,			0,			0,		false};
color darMageColor =			{50,	50,		0,		0,		0,			0,			0,		false};

// light colors
color minersLightColor; // set on each level as a weighted average of the two following colors depending on depth
color minersLightEndColor =		{60,	60,		90,	0,		0,			0,			0,		false};
color minersLightStartColor =	{180,	180,	180,	0,		0,			0,			0,		false};
color torchLightColor =			{90,	50,		30,		0,		20,			10,			0,		true};
color magicLightColor =			{60,	60,		100,	40,		40,			0,			0,		true};
color burningLightColor =		{100,	10,		10,		0,		20,			5,			0,		true};
color wispLightColor =			{50,	66,		100,	33,		10,			0,			0,		true};
color ectoplasmLightColor =		{23,	10,		28,		13,		0,			13,			3,		false};
color explosionColor =			{10,	8,		2,		0,		2,			2,			0,		true};
color lichLightColor =			{30,	80,		30,		0,		0,			20,			0,		true};
color forceFieldLightColor =	{10,	10,		10,		0,		50,			50,			0,		true};
color sunLightColor =			{100,	100,	75,		0,		0,			0,			0,		false};

// color multipliers
color colorDim25 =				{25,	25,		25,		25,		25,			25,			25,		false};
color memoryColor =				{15,	15,		60,		10,		10,			20,			0,		false};
color magicMapColor =			{30,	10,		30,		30,		10,			30,			0,		false};
color clairvoyanceColor =		{40,	80,		40,		20,		40,			20,			0,		false};

// blood colors
color humanBloodColor =			{60,	20,		10,		0,		0,			0,			0,		false};
color insectBloodColor =		{10,	60,		20,		0,		0,			0,			0,		false};
color vomitColor =				{60,	50,		5,		0,		0,			0,			0,		false};
color urineColor =				{70,	70,		40,		0,		0,			0,			0,		false};
color methaneColor =			{45,	60,		15,		0,		0,			0,			0,		false};

// gas colors
color poisonGasColor =			{75,	25,		85,		0,		0,			0,			0,		false};
color confusionGasColor =		{50,	50,		50,		40,		40,			40,			0,		true};

// interface colors
color blueBar =					{0,		0,		75,		0,		0,			0,			0,		false};
color redBar =					{75,	0,		0,		0,		0,			0,			0,		false};
color hiliteColor =				{100,	100,	0,		0,		0,			0,			0,		false};

color playerInLight =			{100,	100,	50,		0,		0,			0,			0,		false};
color playerInShadow =			{75,	75,		100,	0,		0,			0,			0,		false};


floorTileType tileCatalog[NUMBER_TILETYPES] = {
//	char		fore color				back color		priority	ignit	fireType	discovType	promoteType		promoteChance	glowLight		flags																							flavorText

// dungeon layer (this layer must have all of fore color, back color and char)
{	' ',		&black,					&black,					10,	0,	DF_PLAIN_FIRE,	0,			0,				0,				NO_LIGHT,		0,																								""},
{WALL_CHAR,		&gray,					&graniteBackColor,		0,	0,	DF_PLAIN_FIRE,	0,			0,				0,				NO_LIGHT,		(OBSTRUCTS_EVERYTHING),																			""},
{FLOOR_CHAR,	&floorForeColor,		&floorBackColor,		9,	0,	DF_PLAIN_FIRE,	0,			0,				0,				NO_LIGHT,		0,																								""},
{WALL_CHAR,		&wallForeColor,			&wallBackColor,			0,	0,	DF_PLAIN_FIRE,	0,			0,				0,				NO_LIGHT,		(OBSTRUCTS_EVERYTHING),																			""},
{WALL_CHAR,		&wallForeColor,			&wallBackColor,			0,	0,	DF_PLAIN_FIRE,	0,			0,				0,				NO_LIGHT,		(OBSTRUCTS_EVERYTHING),																			""},
{WALL_CHAR,		&wallForeColor,			&wallBackColor,			0,	0,	DF_PLAIN_FIRE,	0,			0,				0,				NO_LIGHT,		(OBSTRUCTS_EVERYTHING),																			""},
{WALL_CHAR,		&wallForeColor,			&wallBackColor,			0,	0,	DF_PLAIN_FIRE,	0,			0,				0,				NO_LIGHT,		(OBSTRUCTS_EVERYTHING),																			""},
{WALL_CHAR,		&wallForeColor,			&wallBackColor,			0,	0,	DF_PLAIN_FIRE,	0,			0,				0,				NO_LIGHT,		(OBSTRUCTS_EVERYTHING),																			""},
{DOOR_CHAR,		&doorForeColor,			&doorBackColor,			2,	50,	DF_EMBERS,		0,			0,				0,				NO_LIGHT,		(OBSTRUCTS_VISION | OBSTRUCTS_ITEMS | OBSTRUCTS_SCENT | OBSTRUCTS_GAS | IS_FLAMMABLE | VANISHES_UPON_PROMOTION),	"you pass through the doorway."},
{WALL_CHAR,		&wallForeColor,			&wallBackColor,			0,	100,DF_EMBERS,		DF_SHOW_DOOR,0,				0,				NO_LIGHT,		(OBSTRUCTS_EVERYTHING | IS_SECRET | IS_FLAMMABLE),												""},
{DOWN_CHAR,		&yellow,				&floorBackColor,		3,	0,	DF_PLAIN_FIRE,	0,			0,				0,				NO_LIGHT,		(PERMITS_DESCENT | OBSTRUCTS_ITEMS | OBSTRUCTS_SURFACE_EFFECTS),								"stairs spiral downward into the depths."},
{UP_CHAR,		&yellow,				&floorBackColor,		3,	0,	DF_PLAIN_FIRE,	0,			0,				0,				NO_LIGHT,		(PERMITS_ASCENT | OBSTRUCTS_ITEMS | OBSTRUCTS_SURFACE_EFFECTS),									"stairs spiral upward."},
{UP_CHAR,		&blue,					&floorBackColor,		3,	0,	DF_PLAIN_FIRE,	0,			0,				0,				NO_LIGHT,		(PERMITS_ASCENT | OBSTRUCTS_ITEMS | OBSTRUCTS_SURFACE_EFFECTS),									"the gilded doors leading out of the dungeon are sealed by an invisible force."},
{WALL_CHAR,		&torchLightColor,		&wallBackColor,			0,	0,	DF_PLAIN_FIRE,	0,			0,				0,				TORCH_LIGHT,	(OBSTRUCTS_EVERYTHING | GLOWS),																	""},
// traps (part of dungeon layer):
{FLOOR_CHAR,	&floorForeColor,		&floorBackColor,		9,	0,	DF_POISON_GAS_CLOUD, DF_SHOW_POISON_GAS_TRAP, 0, 0,			NO_LIGHT,		(IS_SECRET | IS_DF_TRAP | OBSTRUCTS_ITEMS | UNKNOWN_TO_MONSTERS),								""},
{TRAP_CHAR,		&centipedeColor,		&floorBackColor,		3,	0,	DF_POISON_GAS_CLOUD, 0,		0,				0,				NO_LIGHT,		(IS_DF_TRAP | OBSTRUCTS_ITEMS | UNKNOWN_TO_MONSTERS),											"there is a hidden pressure plate in the floor above a reserve of poisonous gas."},
{FLOOR_CHAR,	&floorForeColor,		&floorBackColor,		9,	0,	DF_POISON_GAS_CLOUD, DF_SHOW_TRAPDOOR_HALO, 0,	0,			NO_LIGHT,		(IS_SECRET | TRAP_DESCENT),																		"you plunge through a hidden trap door!"},
{CHASM_CHAR,	&chasmForeColor,		&black,					3,	0,	DF_POISON_GAS_CLOUD, 0,		0,				0,				NO_LIGHT,		(TRAP_DESCENT),																					"you plunge through a hidden trap door!"},
{FLOOR_CHAR,	&floorForeColor,		&floorBackColor,		9,	0,	DF_PARALYSIS_GAS_CLOUD,	DF_SHOW_PARALYSIS_GAS_TRAP, 0, 0,	NO_LIGHT,		(IS_SECRET | IS_DF_TRAP | OBSTRUCTS_ITEMS | UNKNOWN_TO_MONSTERS),								""},
{TRAP_CHAR,		&pink,					&floorBackColor,		3,	0,	DF_PARALYSIS_GAS_CLOUD,	0,	0,				0,				NO_LIGHT,		(IS_DF_TRAP | OBSTRUCTS_ITEMS | UNKNOWN_TO_MONSTERS),											"there is a hidden pressure plate in the floor above a reserve of paralyzing gas."},
{FLOOR_CHAR,	&floorForeColor,		&floorBackColor,		9,	0,	DF_CONFUSION_GAS_TRAP_CLOUD,DF_SHOW_CONFUSION_GAS_TRAP, 0,0,NO_LIGHT,		(IS_SECRET | IS_DF_TRAP | OBSTRUCTS_ITEMS | UNKNOWN_TO_MONSTERS),								""},
{TRAP_CHAR,		&confusionGasColor,		&floorBackColor,		3,	0,	DF_CONFUSION_GAS_TRAP_CLOUD,0,	0,			0,				NO_LIGHT,		(IS_DF_TRAP | OBSTRUCTS_ITEMS | UNKNOWN_TO_MONSTERS),											"A hidden pressure plate accompanies a reserve of psychotropic gas."},
{FLOOR_CHAR,	&floorForeColor,		&floorBackColor,		9,	0,	DF_FLAMETHROWER,	DF_SHOW_FLAMETHROWER_TRAP, 0,	0,		NO_LIGHT,		(IS_SECRET | IS_DF_TRAP | OBSTRUCTS_ITEMS | UNKNOWN_TO_MONSTERS),								""},
{TRAP_CHAR,		&lavaForeColor,			&floorBackColor,		3,	0,	DF_FLAMETHROWER,	0,		0,				0,				NO_LIGHT,		(IS_DF_TRAP | OBSTRUCTS_ITEMS | UNKNOWN_TO_MONSTERS),											"A hidden pressure plate is connected to a crude flamethrower mechanism."},

// liquid layer
{WATER_CHAR,	&deepWaterForeColor,	&deepWaterBackColor,	4,	0,	DF_STEAM_ACCUMULATION,	0,	0,				0,				NO_LIGHT,		(EXTINGUISHES_FIRE | IS_DEEP_WATER | OBSTRUCTS_SCENT | ALLOWS_SUBMERGING),						"the current tugs you in different directions."},
{FLOOR_CHAR,	&shallowWaterForeColor,	&shallowWaterBackColor,	4,	0,	DF_STEAM_ACCUMULATION,	0,	0,				0,				NO_LIGHT,		(EXTINGUISHES_FIRE | ALLOWS_SUBMERGING),														"the water is cold and reaches your knees."},
{WATER_CHAR,	&mudForeColor,			&mudBackColor,			4,	0,	DF_PLAIN_FIRE,	0,			DF_METHANE_GAS_PUFF, 1,			NO_LIGHT,		(ALLOWS_SUBMERGING),																			"you are knee-deep in thick, foul-smelling mud."},
{CHASM_CHAR,	&chasmForeColor,		&black,					4,	0,	DF_PLAIN_FIRE,	0,			0,				0,				NO_LIGHT,		(EXTINGUISHES_FIRE | OBSTRUCTS_SCENT | TRAP_DESCENT),											"you plunge downward into the chasm!"},
{FLOOR_CHAR,	&white,					&chasmEdgeBackColor,	8,	0,	DF_PLAIN_FIRE,	0,			0,				0,				NO_LIGHT,		0,																								"chilly winds blow upward from the stygian depths."},
{WATER_CHAR,	&lavaForeColor,			&lavaBackColor,			4,	0,	DF_OBSIDION,	0,			0,				0,				LAVA_LIGHT,		(OBSTRUCTS_SCENT | GLOWS | LAVA_INSTA_DEATH | ALLOWS_SUBMERGING),								"searing heat rises from the lava."},
{FLOOR_CHAR,	&floorForeColor,		&floorBackColor,		8,	0,	DF_PLAIN_FIRE,	0,			0,				0,				SUN_LIGHT,		(GLOWS),																						"sunlight streams through cracks in the ceiling."},

// surface layer
{GRASS_CHAR,	&grassColor,			0,						6,	15,	DF_PLAIN_FIRE,	0,			0,				0,				NO_LIGHT,		(IS_FLAMMABLE),																					"grass-like fungus crunches underfoot."},
{GRASS_CHAR,	&teal,					0,						6,	10,	DF_PLAIN_FIRE,	0,			0,				0,				FUNGUS_LIGHT,	(GLOWS | IS_FLAMMABLE),																			"luminescent fungus casts a pale, eerie glow."},
{FLOOR_CHAR,	&humanBloodColor,		0,						8,	0,	DF_PLAIN_FIRE,	0,			0,				0,				NO_LIGHT,		0,																								"the floor is splattered with blood."},
{FLOOR_CHAR,	&insectBloodColor,		0,						8,	0,	DF_PLAIN_FIRE,	0,			0,				0,				NO_LIGHT,		0,																								"the floor is splattered with greenish blood."},
{FLOOR_CHAR,	&acidBackColor,			0,						8,	0,	DF_PLAIN_FIRE,	0,			0,				0,				NO_LIGHT,		0,																								"the floor is splattered with acid."},
{FLOOR_CHAR,	&vomitColor,			0,						8,	0,	DF_PLAIN_FIRE,	0,			0,				0,				NO_LIGHT,		0,																								"the floor is caked with vomit."},
{FLOOR_CHAR,	&urineColor,			0,						8,	0,	DF_PLAIN_FIRE,	0,			0,				1,				NO_LIGHT,		(VANISHES_UPON_PROMOTION),																		"a puddle of urine covers the ground."},
{ASH_CHAR,		&ashForeColor,			0,						8,	0,	DF_PLAIN_FIRE,	0,			0,				0,				NO_LIGHT,		0,																								"charcoal and ash crunches underfoot."},
{FLOOR_CHAR,	&ashForeColor,			0,						8,	0,	DF_PLAIN_FIRE,	0,			0,				0,				NO_LIGHT,		0,																								"the ground has fused into obsidion."},
{FLOOR_CHAR,	&shallowWaterBackColor,	0,						8,	20,	0,				0,			0,				1,				NO_LIGHT,		(IS_FLAMMABLE | VANISHES_UPON_PROMOTION | IS_NOT_REMEMBERED),									"a puddle of water covers the ground."},
{BONES_CHAR,	&bonesForeColor,		0,						7,	0,	DF_PLAIN_FIRE,	0,			0,				0,				NO_LIGHT,		0,																								"unidentifiable bones, yellowed with age, litter the ground."},
{BONES_CHAR,	&gray,					0,						7,	0,	DF_PLAIN_FIRE,	0,			0,				0,				NO_LIGHT,		0,																								"rocky rubble covers the ground."},
{FLOOR_CHAR,	&ectoplasmColor,		0,						7,	0,	DF_PLAIN_FIRE,	0,			0,				0,				ECTOPLASM_LIGHT,(GLOWS),																						"a thick, glowing substance has congealed on the ground."},
{ASH_CHAR,		&lavaForeColor,			0,						7,	0,	DF_PLAIN_FIRE,	0,			DF_ASH,			3,				EMBER_LIGHT,	(GLOWS | VANISHES_UPON_PROMOTION | IS_NOT_REMEMBERED),											"sputtering embers cover the ground."},
{WEB_CHAR,		&white,					0,						2,	100,DF_PLAIN_FIRE,	0,			0,				0,				NO_LIGHT,		(ENTANGLES | IS_FLAMMABLE),																		"thick, sticky spiderwebs fill the area."},
{FOLIAGE_CHAR,	&grassColor,			0,						3,	15,	DF_PLAIN_FIRE,	0,			DF_TRAMPLED_FOLIAGE,	0,		NO_LIGHT,		(OBSTRUCTS_VISION | OBSTRUCTS_SCENT | PROMOTES_ON_STEP | VANISHES_UPON_PROMOTION | IS_FLAMMABLE),"dense foliage fills the area, thriving on the sunlight that trickles in."},
{GRASS_CHAR,	&grassColor,			0,						6,	15,	DF_PLAIN_FIRE,	0,			DF_FOLIAGE_REGROW,		1,		NO_LIGHT,		(VANISHES_UPON_PROMOTION | IS_FLAMMABLE),														"the foliage in this area has been recently trampled."},
{WALL_CHAR,		&forceFieldColor,		&forceFieldColor,		0,	0,	0,				0,			0,				2,				FORCEFIELD_LIGHT, (OBSTRUCTS_PASSABILITY | OBSTRUCTS_GAS | VANISHES_UPON_PROMOTION | IS_NOT_REMEMBERED | GLOWS), ""},
{CHAIN_TOP_LEFT,&gray,					0,						2,	0,	0,				0,			0,				0,				NO_LIGHT,		0,																								"a thick iron manacle is anchored to the ceiling."},
{CHAIN_BOTTOM_RIGHT, &gray,				0,						2,	0,	0,				0,			0,				0,				NO_LIGHT,		0,																								"a thick iron manacle is anchored to the floor."},
{CHAIN_TOP_RIGHT, &gray,				0,						2,	0,	0,				0,			0,				0,				NO_LIGHT,		0,																								"a thick iron manacle is anchored to the ceiling."},
{CHAIN_BOTTOM_LEFT, &gray,				0,						2,	0,	0,				0,			0,				0,				NO_LIGHT,		0,																								"a thick iron manacle is anchored to the floor."},
{CHAIN_TOP,		&gray,					0,						2,	0,	0,				0,			0,				0,				NO_LIGHT,		0,																								"a thick iron manacle is anchored to the wall."},
{CHAIN_BOTTOM,	&gray,					0,						2,	0,	0,				0,			0,				0,				NO_LIGHT,		0,																								"a thick iron manacle is anchored to the wall."},
{CHAIN_LEFT,	&gray,					0,						2,	0,	0,				0,			0,				0,				NO_LIGHT,		0,																								"a thick iron manacle is anchored to the wall."},
{CHAIN_RIGHT,	&gray,					0,						2,	0,	0,				0,			0,				0,				NO_LIGHT,		0,																								"a thick iron manacle is anchored to the wall."},

// fire tiles
{FIRE_CHAR,		&lavaForeColor,			0,						1,	0,	0,				0,			DF_EMBERS,		5,				FIRE_LIGHT,		(IS_FIRE | GLOWS | VANISHES_UPON_PROMOTION | IS_NOT_REMEMBERED),								"flames billow upwards."},
{FIRE_CHAR,		&lavaForeColor,			0,						1,	0,	0,				0,			DF_OBSIDION,	2,				FIRE_LIGHT,		(IS_FIRE | GLOWS | VANISHES_UPON_PROMOTION | IS_NOT_REMEMBERED),								"oily dragonfire eats at the floor."},
{FIRE_CHAR,		&lavaForeColor,			0,						1,	0,	NOTHING,		0,			NOTHING,		80,				FIRE_LIGHT,		(IS_FIRE | GLOWS | VANISHES_UPON_PROMOTION | IS_NOT_REMEMBERED),								"flammable gas fills the air with flame."},
{FIRE_CHAR,		&yellow,				0,						1,	0,	0,				0,			DF_GAS_FIRE,	100,			EXPLOSION_LIGHT,(IS_FIRE | GLOWS | IS_NOT_REMEMBERED | CAUSES_EXPLOSIVE_DAMAGE),								"the force of the explosion slams into you."},

// gas layer
{	' ',		0,						&poisonGasColor,		3,	100,DF_GAS_FIRE,	0,			0,				0,				NO_LIGHT,		(IS_VOLUMETRIC | IS_FLAMMABLE | GAS_DISSIPATES | CAUSES_DAMAGE | IS_NOT_REMEMBERED),			"you can feel the purple gas eating at your flesh."},
{	' ',		0,						&confusionGasColor,		3,	100,DF_GAS_FIRE,	0,			0,				0,				CONFUSION_GAS_LIGHT,(IS_VOLUMETRIC | IS_FLAMMABLE | GAS_DISSIPATES_QUICKLY | CAUSES_CONFUSION | GLOWS | IS_NOT_REMEMBERED),	"the rainbow-colored gas tickles your brain."},
{	' ',		0,						&vomitColor,			3,	100,DF_GAS_FIRE,	0,			0,				0,				NO_LIGHT,		(IS_VOLUMETRIC | IS_FLAMMABLE | GAS_DISSIPATES_QUICKLY | CAUSES_NAUSEA | IS_NOT_REMEMBERED),	"the stench of rotting flesh is overpowering."},
{	' ',		0,						&pink,					3,	100,DF_GAS_FIRE,	0,			0,				0,				NO_LIGHT,		(IS_VOLUMETRIC | IS_FLAMMABLE | GAS_DISSIPATES_QUICKLY | CAUSES_PARALYSIS | IS_NOT_REMEMBERED),	"the pale gas causes your muscles to stiffen."},
{	' ',		0,						&methaneColor,			3,	100,DF_EXPLOSION_FIRE,0,		0,				0,				NO_LIGHT,		(IS_VOLUMETRIC | IS_FLAMMABLE | IS_NOT_REMEMBERED),												"the smell of flammable swamp gas fills the air."},
{	' ',		0,						&white,					3,	0,	DF_GAS_FIRE,	0,			0,				0,				NO_LIGHT,		(IS_VOLUMETRIC | CAUSES_DAMAGE | GAS_DISSIPATES_QUICKLY | IS_NOT_REMEMBERED),					"scalding steam fills the air!"},

};

// note that features in the gas layer use the startprob as volume, ignore probdecr, and spawn in only a single point.
// Intercepts and slopes are in units of 0.01.
dungeonFeature dungeonFeatureCatalog[NUMBER_DUNGEON_FEATURES] = {
// tileType			layer		startProb		probDecr.	ovDngn	subsequentDF	foundation	>Depth	<Depth	freq	minIncp	minSlope	maxNumber
{}, // nothing
{LUMINESCENT_FUNGUS,SURFACE,	{50, 70, 2},	{4, 10, 2},	false,	0,				FLOOR,		7,		100,	15,		-300,	70,			14},
{GRASS,				SURFACE,	{50, 80, 2},	{3, 7, 2},	false,	0,				FLOOR,		0,		10,		0,		1000,	-60,		10},
{BONES,				SURFACE,	{70, 80, 2},	{20, 25, 2},false,	0,				FLOOR,		12,		100,	30,		0,		0,			4},
{RUBBLE,			SURFACE,	{40, 50, 2},	{20, 25, 2},false,	0,				FLOOR,		0,		100,	30,		0,		0,			4},
{FOLIAGE,			SURFACE,	{50, 80, 2},	{7, 8, 1},	false,	0,				FLOOR,		0,		0,		15,		500,	-167,		10}, // disabled because it sucks

// torches
{TORCH_WALL,		DUNGEON,	{0,0,1},		{0,0,1},	true,	0,				TOP_WALL,	6,		100,	5,		-200,	70,			12},

// traps
{GAS_TRAP_POISON_HIDDEN,DUNGEON,{0,0,1},		{0,0,1},	true,	0,				FLOOR,		5,		100,	30,		100,	0,			5},
{GAS_TRAP_PARALYSIS_HIDDEN,DUNGEON,{0,0,1},		{0,0,1},	true,	0,				FLOOR,		7,		100,	30,		100,	0,			5},
{TRAP_DOOR_HIDDEN,	DUNGEON,	{0,0,1},		{0,0,1},	true,	0,				FLOOR,		9,		100,	30,		100,	0,			5},
{GAS_TRAP_CONFUSION_HIDDEN,DUNGEON,{0,0,1},		{0,0,1},	true,	0,				FLOOR,		11,		100,	30,		100,	0,			5},
{FLAMETHROWER_HIDDEN,DUNGEON,	{0,0,1},		{0,0,1},	true,	0,				FLOOR,		15,		100,	30,		100,	0,			5},

// misc. liquids
{MUD,				LIQUID,		{65,80,1},		{4,6,1},	false,	0,				FLOOR,		1,		100,	30,		0,		0,			5},
{SUNLIGHT_POOL,		LIQUID,		{50, 80, 2},	{5, 6, 1},	false,	0,				FLOOR,		0,		5,		15,		500,	-150,		10},

// Dungeon features spawned during gameplay:

// revealed secrets
{DOOR,				DUNGEON,	{0,0,1},		{0,0,1},	true},
{GAS_TRAP_POISON,	DUNGEON,	{0,0,1},		{0,0,1},	true},
{GAS_TRAP_PARALYSIS,DUNGEON,	{0,0,1},		{0,0,1},	true},
{CHASM_EDGE,		LIQUID,		{100,100,1},	{100,100,1},false,	DF_SHOW_TRAPDOOR},
{TRAP_DOOR,			LIQUID,		{0,0,1},		{0,0,1},	true},
{GAS_TRAP_CONFUSION,DUNGEON,	{0,0,1},		{0,0,1},	true},
{FLAMETHROWER,		DUNGEON,	{0,0,1},		{0,0,1},	true},

// bloods
// Start probability is actually a percentage for bloods. Base prob is 15 + (damage * 2/3), and then take the percent of that.
// If it's a gas, we multiply the base by an additional 100. Thus to get a starting gas volume of a typical poison potion (1000)
// with a blow of 10 damage, use a starting probability of 48.
{HUMAN_BLOOD,		SURFACE,	{100,100,1},	{25,25,1},	false},
{GREEN_BLOOD,		SURFACE,	{100,100,1},	{25,25,1},	false},
{ACID_SPLATTER,		SURFACE,	{100,100,1},	{25,25,1},	false},
{ASH,				SURFACE,	{50,50,1},		{25,25,1},	false},
{ECTOPLASM,			SURFACE,	{100,100,1},	{25,25,1},	false},
{RUBBLE,			SURFACE,	{33,33,1},		{25,25,1},	false},
{ROT_GAS,			GAS,		{12,12,1},		{0,0,0},	false},

// monster effects
{VOMIT,				SURFACE,	{30,30,1},		{10,10,1},	false},

// misc
{ROT_GAS,			GAS,		{10,20,1},		{0,0,0},	false},
{STEAM,				GAS,		{300,350,1},	{0,0,0},	false},
{STEAM,				GAS,		{10,20,1},		{0,0,0},	false},
{METHANE_GAS,		GAS,		{1,4,1},		{0,0,0},	false},
{EMBERS,			SURFACE,	{0,0,1},		{0,0,0},	false},
{URINE,				SURFACE,	{60,70,1},		{25,25,1},	false},
{PUDDLE,			SURFACE,	{10,15,1},		{25,25,1},	false},
{ASH,				SURFACE,	{0,0,1},		{0,0,0},	false},
{ECTOPLASM,			SURFACE,	{0,0,1},		{0,0,1},	false},
{FORCEFIELD,		SURFACE,	{100,100,1},	{50,50,1},	false},
{TRAMPLED_FOLIAGE,	SURFACE,	{0,0,1},		{0,0,1},	false},
{FOLIAGE,			SURFACE,	{0,0,1},		{0,0,1},	false},

// fire
{PLAIN_FIRE,		SURFACE,	{0,0,1},		{0,0,0},	false},
{GAS_FIRE,			SURFACE,	{0,0,1},		{0,0,0},	false},
{GAS_EXPLOSION,		SURFACE,	{50,75,1},		{10,25,1},	false},
{PLAIN_FIRE,		SURFACE,	{100,100,1},	{25,50,1},	false},
{EMBERS,			SURFACE,	{0,0,1},		{0,0,0},	false},
{OBSIDION,			SURFACE,	{0,0,1},		{0,0,0},	false},

// gas trap/potion effects
{POISON_GAS,		GAS,		{1000,1000,1},	{0,0,0},	false},
{PARALYSIS_GAS,		GAS,		{1000,1000,1},	{0,0,0},	false},
{CONFUSION_GAS,		GAS,		{1000,1000,1},	{0,0,0},	false},
{CONFUSION_GAS,		GAS,		{300,300,1},	{0,0,0},	false},
{METHANE_GAS,		GAS,		{10000,10000,1},{0,0,0},	false},
};


lightSource lightCatalog[NUMBER_LIGHT_KINDS] = {
{},																		// NO_LIGHT
{&minersLightColor,		{0, 0, 1},			10000,	35,		true},		// miners light
{&burningLightColor,	{8, 8, 1},			500,	60,		false},		// burning creature light
{&magicLightColor,		{DCOLS, DCOLS, 1},	200,	50,		true},		// staff of lighting light
{&wispLightColor,		{5, 9, 1},			500,	0,		false},		// will-o'-the-wisp light
{&lavaForeColor,		{5, 6, 1},			500,	0,		false},		// salamander glow
{&pink,					{6, 6, 1},			500,	0,		true},		// imp light
{&pixieColor,			{4, 6, 1},			500,	50,		false},		// pixie light
{&lichLightColor,		{15, 15, 1},		100,	0,		false},		// lich light

// glowing terrain:
{&torchLightColor,		{10, 10, 1},		500,	50,		false},		// torch
{&lavaLightColor,		{3, 3, 1},			125,	50,		false},		// lava
{&sunLightColor,		{2, 2, 1},			100,	25,		true},		// sunlight
{&fungusLightColor,		{3, 3, 1},			100,	50,		false},		// luminescent fungus
{&ectoplasmColor,		{2, 2, 1},			100,	50,		false},		// ectoplasm
{&lavaLightColor,		{2, 2, 1},			200,	50,		false},		// embers
{&lavaLightColor,		{5, 10, 1},			300,	0,		false},		// fire
{&explosionColor,		{DCOLS,DCOLS,1},	10000,	100,	false},		// explosions
{&confusionGasColor,	{3, 3, 1},			100,	100,	false},		// confusion gas
{&forceFieldLightColor,	{2, 2, 1},			200,	50,		false},		// forcefield

};

// Defines all creatures, which include monsters and the player:
creatureType monsterCatalog[NUMBER_MONSTER_KINDS] = {
//	name			ch		color			exp		HP		def		acc		damage			reg	sight	scent	move	attack	blood			light	DFChance DFType		behaviorF, abilityF
{0,	"you",	PLAYER_CHAR,	&playerInLight,	0,		15,		0,		75,		{1, 2, 1},		0,	DCOLS,	0,		100,	100,	DF_HUMAN_BLOOD,	0,		0,		0},

{0, "rat",			'r',	&gray,			1,		5,		0,		80,		{1, 2, 1},		20,	20,		30,		100,	100,	DF_HUMAN_BLOOD,	0,		1,		DF_URINE},
{0, "kobold",		'k',	&goblinColor,	2,		5,		0,		80,		{1, 3, 1},		20,	30,		30,		100,	100,	DF_HUMAN_BLOOD,	0,		0,		0},
{0,	"jackal",		'j',	&jackalColor,	3,		7,		0,		70,		{1, 4, 1},		20,	50,		50,		50,		100,	DF_HUMAN_BLOOD,	0,		1,		DF_URINE},
{0,	"eel",			'e',	&eelColor,		30,		18,		20,		100,	{2, 9, 2},		5,	DCOLS,	20,		50,		100,	NOTHING,		0,		0,		0,
		(MONST_RESTRICTED_TO_LIQUID | MONST_IMMUNE_TO_WATER | MONST_SUBMERGES | MONST_FLITS | MONST_NEVER_SLEEPS)},
{0,	"monkey",		'm',	&ogreColor,		3,		12,		17,		100,	{1, 3, 1},		20,	DCOLS,	100,	100,	100,	DF_HUMAN_BLOOD,	0,		1,		DF_URINE,
		(0), (MA_HIT_STEAL_FLEE)},
{0, "goblin",		'g',	&goblinColor,	5,		15,		10,		70,		{2, 6, 1},		20,	30,		20,		100,	100,	DF_HUMAN_BLOOD,	0,		0,		0},
{0, "goblin totem",	TOTEM_CHAR,	&orange,	15,	30,		0,		0,		{0, 0, 0},		0,	DCOLS,		200,	100,	300,	DF_RUBBLE_BLOOD,IMP_LIGHT,0,	0,
		(MONST_IMMUNE_TO_WEBS | MONST_NEVER_SLEEPS | MONST_INTRINSIC_LIGHT | MONST_IMMOBILE | MONST_INANIMATE | MONST_ALWAYS_HUNTING), (MA_CAST_HASTE | MA_CAST_SPARK)},
{0, "toad",			't',	&toadColor,		5,		18,		0,		90,		{1, 4, 1},		10,	15,		15,		100,	100,	DF_GREEN_BLOOD,	0,		0,		0,
		(0), (MA_HIT_HALLUCINATE)},
{0, "vampire bat",	'v',	&gray,			10,		18,		25,		100,	{2, 6, 1},		20,	DCOLS,	50,		50,		100,	DF_HUMAN_BLOOD,	0,		0,		0,
		(MONST_FLIES | MONST_FLITS)},
{0, "arrow turret", TURRET_CHAR,&black,		10,		30,		0,		70,		{2, 6, 1},		0,	DCOLS,	50,		100,	250,	NOTHING,		0,		0,		0,
		(MONST_TURRET), (MA_ATTACKS_FROM_DISTANCE)},
{0, "acid mound",	'a',	&acidBackColor,	12,		15,		10,		70,		{1, 3, 1},		5,	15,		15,		100,	100,	DF_ACID_BLOOD,	0,		0,		0,
		(MONST_DEFEND_DEGRADE_WEAPON), (MA_HIT_DEGRADE_ARMOR)},
{0, "centipede",	'c',	&centipedeColor,10,		20,		20,		80,		{1, 5, 1},		20,	20,		50,		100,	100,	DF_GREEN_BLOOD,	0,		0,		0,
		(0), (MA_HIT_SAP_STRENGTH)},
{0,	"ogre",			'O',	&ogreColor,		50,		55,		80,		125,	{4, 18, 4},		20,	30,		30,		100,	200,	DF_HUMAN_BLOOD,	0,		0,		0,			(0)},
{0,	"bog monster",	'b',	&krakenColor,	50,		55,		80,		500,	{3, 5, 1},		3,	30,		30,		200,	100,	NOTHING,		0,		0,		0,
		(MONST_RESTRICTED_TO_LIQUID | MONST_SUBMERGES | MONST_FLITS | MONST_FLEES_NEAR_DEATH), (MA_SEIZES)},
{0, "ogre totem",	TOTEM_CHAR,	&green,		100,	70,		0,		0,		{0, 0, 0},		0,	DCOLS,	200,	100,	400,	DF_RUBBLE_BLOOD,LICH_LIGHT,0,	0,
		(MONST_IMMUNE_TO_WEBS | MONST_NEVER_SLEEPS | MONST_INTRINSIC_LIGHT | MONST_IMMOBILE | MONST_INANIMATE | MONST_ALWAYS_HUNTING), (MA_CAST_HEAL | MA_CAST_SLOW)},
{0, "spider",		's',	&white,			60,		20,		90,		90,		{1, 10, 1},		20,	50,		20,		100,	100,	DF_GREEN_BLOOD,	0,		0,		0,
		(MONST_IMMUNE_TO_WEBS | MONST_CAST_SPELLS_SLOWLY), (MA_SHOOTS_WEBS)},
{0, "spark turret", TURRET_CHAR, &lightningColor, 70,80,	0,		100,	{0, 0, 0},		0,	DCOLS,	50,		100,	150,	NOTHING,		0,		0,		0,
		(MONST_TURRET), (MA_CAST_SPARK)},
{0,	"will-o-the-wisp",'w',	&wispLightColor,40,		10,		120,	100,	{3,	10, 2},		5,	90,		15,		100,	100,	DF_ASH_BLOOD,	WISP_LIGHT,	0,	0,
		(MONST_IMMUNE_TO_FIRE | MONST_FLIES | MONST_FLITS | MONST_NEVER_SLEEPS | MONST_FIERY | MONST_INTRINSIC_LIGHT)},
{0, "zombie",		'z',	&vomitColor,	30,		55,		0,		100,	{4, 15, 1},		0,	50,		200,	200,	200,	DF_ROT_GAS_BLOOD,0,		100,	DF_ROT_GAS_PUFF, (0)},
{0, "troll",		'T',	&trollColor,	150,	65,		90,		125,	{5, 20, 3},		1,	DCOLS,	20,		100,	100,	DF_HUMAN_BLOOD,	0,		0,		0,			(0)},
{0,	"ogre shaman",	'O',	&green,			120,	45,		70,		100,	{4, 10, 1},		20,	DCOLS,	30,		100,	200,	DF_HUMAN_BLOOD,	0,		0,		0,
		(MONST_MAINTAINS_DISTANCE | MONST_CAST_SPELLS_SLOWLY), (MA_CAST_HASTE | MA_CAST_SPARK | MA_CAST_SUMMON)},
{0, "naga",			'N',	&trollColor,	150,	75,		100,	150,	{5, 13, 4},		10,	DCOLS,	100,	100,	100,	DF_GREEN_BLOOD,	0,		100,	DF_PUDDLE,
		(MONST_IMMUNE_TO_WATER | MONST_SUBMERGES | MONST_NEVER_SLEEPS)},
{0, "salamander",	'S',	&salamanderColor,150,	60,		110,	150,	{5, 15, 3},		10,	DCOLS,	100,	100,	100,	DF_ASH_BLOOD,	SALAMANDER_LIGHT, 100, DF_SALAMANDER_FLAME,
		(MONST_IMMUNE_TO_FIRE | MONST_SUBMERGES | MONST_NEVER_SLEEPS | MONST_FIERY | MONST_INTRINSIC_LIGHT)},
{0, "dar blademaster",'d',	&purple,		175,	35,		120,	160,	{2, 12, 2},		20,	DCOLS,	100,	100,	100,	DF_HUMAN_BLOOD,	0,		0,		0,
		(MONST_CARRY_ITEM_25), (MA_CAST_BLINK)},
{0, "dar priestess", 'd',	&darPriestessColor,150,	20,		80,		100,	{2, 5, 1},		20,	DCOLS,	100,	100,	100,	DF_HUMAN_BLOOD, 0,		0,		0,
		(MONST_MAINTAINS_DISTANCE | MONST_CARRY_ITEM_25), (MA_CAST_HEAL | MA_CAST_SPARK | MA_CAST_HASTE | MA_CAST_CANCEL)},
{0, "dar battlemage",'d',	&darMageColor,	150,	20,		80,		100,	{1, 3, 1},		20,	DCOLS,	100,	100,	100,	DF_HUMAN_BLOOD,	0,		0,		0,
		(MONST_MAINTAINS_DISTANCE | MONST_CARRY_ITEM_25), (MA_CAST_FIRE | MA_CAST_SLOW)},
{0,	"centaur",		'C',	&tanColor,		175,	35,		50,		175,	{2, 8, 2},		20,	DCOLS,	30,		50,		200,	DF_HUMAN_BLOOD,	0,		0,		0,
		(MONST_MAINTAINS_DISTANCE | MONST_CAST_SPELLS_SLOWLY), (MA_ATTACKS_FROM_DISTANCE)},
{0, "acid turret", TURRET_CHAR,	&darkGreen,	120,	35,		0,		250,	{1, 2, 1},		0,	DCOLS,	50,		100,	250,	NOTHING,		0,		0,		0,
		(MONST_TURRET), (MA_ATTACKS_FROM_DISTANCE | MA_HIT_DEGRADE_ARMOR)},
{0,	"kraken",		'K',	&krakenColor,	200,	120,	0,		150,	{10, 25, 3},	1,	DCOLS,	20,		50,		100,	NOTHING,		0,		0,		0,
		(MONST_RESTRICTED_TO_LIQUID | MONST_IMMUNE_TO_WATER | MONST_SUBMERGES | MONST_FLITS | MONST_NEVER_SLEEPS | MONST_FLEES_NEAR_DEATH)},
{0,	"lich",			'L',	&white,			250,	35,		100,	175,	{1, 7, 1},		0,	DCOLS,	100,	100,	100,	DF_ASH_BLOOD,	LICH_LIGHT,	0,	0,
		(MONST_MAINTAINS_DISTANCE | MONST_CARRY_ITEM_25 | MONST_INTRINSIC_LIGHT), (MA_CAST_SUMMON | MA_CAST_FIRE)},
{0, "pixie",		'p',	&pixieColor,	150,	10,		120,	100,	{1, 3, 1},		20,	100,	100,	50,		100,	DF_GREEN_BLOOD,	PIXIE_LIGHT, 0,	0,
		(MONST_MAINTAINS_DISTANCE | MONST_INTRINSIC_LIGHT | MONST_FLIES | MONST_FLITS), (MA_CAST_SPARK | MA_CAST_SLOW | MA_CAST_CANCEL)},
{0,	"phantom",		'P',	&ectoplasmColor,180,	35,		100,	160,	{8, 22, 4},		0,	30,		30,		50,		200,	DF_ECTOPLASM_BLOOD,	0,	2,		DF_ECTOPLASM_DROPLET,
		(MONST_INVISIBLE | MONST_FLITS | MONST_FLIES | MONST_IMMUNE_TO_WEBS)},
{0, "dart turret", TURRET_CHAR,	&darkPurple,150,	40,		0,		130,	{1, 2, 1},		0,	DCOLS,	50,		100,	250,	NOTHING,		0,		0,		0,
		(MONST_TURRET), (MA_ATTACKS_FROM_DISTANCE | MA_HIT_SAP_STRENGTH)},
{0, "imp",			'i',	&pink,			150,	35,		110,	225,	{3, 10, 2},		10,	10,		15,		50,		100,	DF_GREEN_BLOOD,	IMP_LIGHT,	0,	0,
		(MONST_TELEPORTS | MONST_INTRINSIC_LIGHT), (MA_HIT_STEAL_FLEE)},
{0,	"fury",			'f',	&darkRed,		50,		19,		110,	200,	{4, 13, 4},		20,	40,		30,		50,		100,	DF_HUMAN_BLOOD,	0,		0,		0,
		(MONST_NEVER_SLEEPS | MONST_FLIES)},
{0, "revenant",		'R',	&ectoplasmColor,300,	20,		0,		200,	{5, 30, 5},		0,	DCOLS,	20,		100,	100,	DF_ECTOPLASM_BLOOD,	0,	0,		0,
		(MONST_IMMUNE_TO_WEAPONS)},
{0, "tentacle horror",'H',	&centipedeColor,2000,	120,	120,	225,	{10, 50, 10},	1,	DCOLS,	50,		100,	100,	NOTHING,		0,		0,		0,			(0)},
{0, "golem",		'G',	&gray,			1000,	400,	100,	225,	{4, 8, 1},		0,	DCOLS,	200,	100,	100,	DF_RUBBLE_BLOOD,0,		0,		0,			(0)},
{0, "dragon",		'D',	&dragonColor,	5000,	150,	140,	250,	{15, 60, 5},	20,	DCOLS,	120,	100,	100,	DF_GREEN_BLOOD,	0,		0,		0,
		(MONST_IMMUNE_TO_FIRE | MONST_CARRY_ITEM_100 | MONST_CAST_SPELLS_SLOWLY), (MA_BREATHES_FIRE)}
};

monsterWords monsterText[NUMBER_MONSTER_KINDS] = {
{"you",				"you",	{"hit", 0}},
{"rat",				"it",	{"scratches", "bites", 0}},
{"kobold",			"it",	{"clubs", "bashes", 0}},
{"jackal",			"it",	{"claws", "bites", "mauls", 0}},
{"eel",				"it",	{"shocks", "bites", 0}},
{"monkey",			"it",	{"tweaks", "bites", "punches", 0}},
{"goblin",			"it",	{"slashes", "cuts", "stabs", 0}},
{"goblin totem",	"it",	{"hits", 0}},
{"toad",			"it",	{"slimes", "slams", 0}},
{"vampire bat",		"it",	{"nips", "bites", 0}},
{"arrow turret",	"it",	{"shoots", 0}},
{"acid mound",		"it",	{"slimes", "douses", "drenches", 0}},
{"centipede",		"it",	{"pricks", "stings", 0}},
{"ogre",			"he",	{"cudgels", "clubs", "batters", 0}},
{"bog monster",		"it",	{"squeezes", "strangles", "crushes", 0}},
{"ogre totem",		"it",	{"hits", 0}},
{"spider",			"it",	{"bites", "stings", 0}},
{"spark turret",	"it",	{"shocks", 0}},
{"will-o-the-wisp",	"it",	{"scorches", "burns", 0}},
{"zombie",			"it",	{"hits", "bites", 0}},
{"troll",			"he",	{"cudgels", "clubs", "bludgeons", "pummels", "batters"}},
{"ogre shaman",		"he",	{"cudgels", "clubs", 0}},
{"naga",			"she",	{"claws", "bites", "tail-whips", 0}},
{"salamander",		"it",	{"claws", "whips", "lashes", 0}},
{"dar blademaster",	"he",	{"grazes", "cuts", "slices", "slashes", "stabs"}},
{"dar priestess",	"she",	{"cuts", "slices", 0}},
{"dar battlemage",	"she",	{"cuts", 0}},
{"centaur",			"he",	{"shoots", 0}},
{"acid turret",		"he",	{"douses", "drenches", 0}},
{"kraken",			"it",	{"slaps", "smites", "batters", 0}},
{"lich",			"it",	{"touches", 0}},
{"pixie",			"she",	{"pokes", 0}},
{"phantom",			"it",	{"hits", 0}},
{"dart turret",		"it",	{"pricks", 0}},
{"imp",				"it",	{"slices", "cuts", 0}},
{"fury",			"she",	{"drubs", "fustigates", "castigates", 0}},
{"revenant",		"it",	{"hits", 0}},
{"tentacle horror",	"it",	{"slaps", "batters", "crushes", 0}},
{"golem",			"it",	{"backhands", "punches", "kicks", 0}},
{"dragon",			"it",	{"claws", "bites", 0}},
};

hordeType hordeCatalog[NUMBER_HORDES] = {
// leader		#members	member list								member numbers					minL	maxL	freq	spawnsIn	flags
{MK_RAT,			0,		{0},									{{0}},							1,		5,		10},
{MK_KOBOLD,			0,		{0},									{{0}},							1,		6,		10},
{MK_JACKAL,			0,		{0},									{{0}},							1,		7,		10},
{MK_EEL,			0,		{0},									{{0}},							2,		17,		10,		DEEP_WATER},
{MK_MONKEY,			0,		{0},									{{0}},							2,		9,		10},
{MK_GOBLIN,			0,		{0},									{{0}},							3,		10,		10},
{MK_TOAD,			0,		{0},									{{0}},							4,		11,		10},
{MK_GOBLIN_TOTEM,	1,		{MK_GOBLIN},							{{2,4,1}},						5,		13,		10,		0,			NO_PERIODIC_SPAWN},
{MK_VAMPIRE_BAT,	1,		{MK_VAMPIRE_BAT},						{{0,2,1}},						5,		12,		10},
{MK_ARROW_TURRET,	0,		{0},									{{0}},							5,		13,		10,		TOP_WALL,	NO_PERIODIC_SPAWN},
{MK_MONKEY,			0,		{0},									{{0}},							5,		12,		10},
{MK_MONKEY,			1,		{MK_MONKEY},							{{2,4,1}},						5,		13,		3},
{MK_ACID_MOUND,		0,		{0},									{{0}},							6,		13,		10},
{MK_GOBLIN,			3,		{MK_GOBLIN, MK_KOBOLD, MK_JACKAL},		{{1, 3, 1}, {1,2,1}, {1,2,1}},	6,		12,		4},
{MK_CENTIPEDE,		0,		{0},									{{0}},							7,		14,		10},
{MK_BOG_MONSTER,	0,		{0},									{{0}},							7,		14,		8,		MUD},
{MK_OGRE,			0,		{0},									{{0}},							8,		13,		10},
{MK_EEL,			1,		{MK_EEL},								{{2, 4, 1}},					8,		22,		7,		DEEP_WATER},
{MK_ACID_MOUND,		1,		{MK_ACID_MOUND},						{{2, 4, 1}},					9,		13,		3},
{MK_SPIDER,			0,		{0},									{{0}},							9,		16,		10},
{MK_SPARK_TURRET,	0,		{0},									{{0}},							11,		18,		10,		TOP_WALL,	NO_PERIODIC_SPAWN},
{MK_DAR_BLADEMASTER,1,		{MK_DAR_BLADEMASTER},					{{0, 1, 1}},					10,		14,		10},
{MK_WILL_O_THE_WISP,0,		{0},									{{0}},							10,		17,		10},
{MK_ZOMBIE,			0,		{0},									{{0}},							11,		18,		10},
{MK_TROLL,			0,		{0},									{{0}},							12,		19,		10},
{MK_OGRE_TOTEM,		1,		{MK_OGRE},								{{2,4,1}},						12,		19,		6,		0,			NO_PERIODIC_SPAWN},
{MK_BOG_MONSTER,	1,		{MK_BOG_MONSTER},						{{2,4,1}},						12,		26,		10,		MUD},
{MK_NAGA,			0,		{0},									{{0}},							13,		20,		10,		DEEP_WATER},
{MK_SALAMANDER,		0,		{0},									{{0}},							13,		20,		10,		LAVA},
{MK_OGRE_SHAMAN,	1,		{MK_OGRE},								{{1, 3, 1}},					14,		20,		10},
{MK_CENTAUR,		1,		{MK_CENTAUR},							{{1, 1, 1}},					14,		21,		10},
{MK_ACID_TURRET,	0,		{0},									{{0}},							15,		22,		10,		TOP_WALL,	NO_PERIODIC_SPAWN},
{MK_PIXIE,			0,		{0},									{{0}},							14,		21,		8},
{MK_DAR_BLADEMASTER,2,		{MK_DAR_BLADEMASTER, MK_DAR_PRIESTESS},	{{0, 1, 1}, {0, 1, 1}},			15,		17,		10},
{MK_KRAKEN,			0,		{0},									{{0}},							15,		30,		10,		DEEP_WATER},
{MK_PHANTOM,		0,		{0},									{{0}},							16,		23,		10},
{MK_DART_TURRET,	0,		{0},									{{0}},							17,		24,		10,		TOP_WALL,	NO_PERIODIC_SPAWN},
{MK_IMP,			0,		{0},									{{0}},							17,		24,		10},
{MK_DAR_BLADEMASTER,3,		{MK_DAR_BLADEMASTER, MK_DAR_PRIESTESS, MK_DAR_BATTLEMAGE},{{1,2,1},{1,1,1},{1,1,1}},18,25,10},
{MK_FURY,			1,		{MK_FURY},								{{2, 4, 1}},					18,		26,		8},
{MK_REVENANT,		0,		{0},									{{0}},							19,		27,		10},
{MK_GOLEM,			0,		{0},									{{0}},							21,		30,		10},
{MK_TENTACLE_HORROR,0,		{0},									{{0}},							22,		40,		10},
{MK_LICH,			0,		{0},									{{0}},							23,		45,		10},
{MK_DRAGON,			0,		{0},									{{0}},							24,		50,		7},
{MK_DRAGON,			1,		{MK_DRAGON},							{{1,1,1}},						27,		50,		3},
{MK_GOLEM,			3,		{MK_GOLEM, MK_DAR_PRIESTESS, MK_DAR_BATTLEMAGE}, {{1, 2, 1}, {0,1,1},{0,1,1}},27,100,	8},
{MK_GOLEM,			1,		{MK_GOLEM},								{{5, 10, 2}},					30,		100,	2},
{MK_TENTACLE_HORROR,2,		{MK_TENTACLE_HORROR, MK_REVENANT},		{{1, 3, 1}, {2, 4, 1}},			32,		100,	2},
{MK_DRAGON,			1,		{MK_DRAGON},							{{3, 5, 1}},					34,		100,	2},

// summons
{MK_OGRE_SHAMAN,	1,		{MK_OGRE},								{{1, 3, 1}},					0,		0,		10,		0,			HORDE_IS_SUMMONED},
{MK_LICH,			1,		{MK_PHANTOM},							{{2, 3, 1}},					0,		0,		10,		0,			HORDE_IS_SUMMONED},
{MK_LICH,			1,		{MK_FURY},								{{2, 3, 1}},					0,		0,		10,		0,			HORDE_IS_SUMMONED},

// captives
{MK_MONKEY,			1,		{MK_KOBOLD},							{{1, 2, 1}},					1,		5,		1,		0,			HORDE_LEADER_CAPTIVE},
{MK_GOBLIN,			1,		{MK_GOBLIN},							{{1, 2, 1}},					3,		7,		1,		0,			HORDE_LEADER_CAPTIVE},
{MK_OGRE,			1,		{MK_GOBLIN},							{{3, 5, 1}},					4,		10,		1,		0,			HORDE_LEADER_CAPTIVE},
{MK_OGRE,			1,		{MK_OGRE},								{{1, 2, 1}},					8,		15,		2,		0,			HORDE_LEADER_CAPTIVE},
{MK_TROLL,			1,		{MK_TROLL},								{{1, 2, 1}},					12,		19,		1,		0,			HORDE_LEADER_CAPTIVE},
{MK_CENTAUR,		1,		{MK_TROLL},								{{1, 2, 1}},					12,		19,		1,		0,			HORDE_LEADER_CAPTIVE},
{MK_TROLL,			2,		{MK_OGRE, MK_OGRE_SHAMAN},				{{2, 3, 1}, {0, 1, 1}},			14,		19,		1,		0,			HORDE_LEADER_CAPTIVE},
{MK_DAR_BLADEMASTER,1,		{MK_TROLL},								{{1, 2, 1}},					12,		19,		1,		0,			HORDE_LEADER_CAPTIVE},
{MK_NAGA,			1,		{MK_SALAMANDER},						{{1, 2, 1}},					13,		20,		1,		0,			HORDE_LEADER_CAPTIVE},
{MK_SALAMANDER,		1,		{MK_NAGA},								{{1, 2, 1}},					13,		20,		1,		0,			HORDE_LEADER_CAPTIVE},
{MK_TROLL,			1,		{MK_SALAMANDER},						{{1, 2, 1}},					13,		19,		1,		0,			HORDE_LEADER_CAPTIVE},
{MK_IMP,			1,		{MK_FURY},								{{2, 4, 1}},					18,		26,		1,		0,			HORDE_LEADER_CAPTIVE},
{MK_PIXIE,			1,		{MK_IMP, MK_PHANTOM},					{{1, 2, 1}, {1, 2, 1}},			14,		21,		1,		0,			HORDE_LEADER_CAPTIVE},
{MK_DAR_BLADEMASTER,1,		{MK_FURY},								{{2, 4, 1}},					18,		26,		1,		0,			HORDE_LEADER_CAPTIVE},
{MK_DAR_BLADEMASTER,1,		{MK_IMP},								{{2, 3, 1}},					18,		26,		1,		0,			HORDE_LEADER_CAPTIVE},
{MK_DAR_PRIESTESS,	1,		{MK_FURY},								{{2, 4, 1}},					18,		26,		1,		0,			HORDE_LEADER_CAPTIVE},
{MK_DAR_BATTLEMAGE,	1,		{MK_IMP},								{{2, 3, 1}},					18,		26,		1,		0,			HORDE_LEADER_CAPTIVE},
{MK_TENTACLE_HORROR,3,		{MK_DAR_BLADEMASTER, MK_DAR_PRIESTESS, MK_DAR_BATTLEMAGE},{{1,2,1},{1,1,1},{1,1,1}},18,25,1,	0,			HORDE_LEADER_CAPTIVE},
{MK_GOLEM,			3,		{MK_DAR_BLADEMASTER, MK_DAR_PRIESTESS, MK_DAR_BATTLEMAGE},{{1,2,1},{1,1,1},{1,1,1}},18,25,1,	0,			HORDE_LEADER_CAPTIVE},
};

// LEVELS

levelProfile levelProfileCatalog[NUMBER_LEVEL_PROFILES] = {
//cave?		cross?	corrid?	door?	maxRms	maxLoops
{33,		100,	80,		60,		99,		30},
};

// ITEMS

char itemTitles[NUMBER_SCROLL_KINDS][30];

char titlePhonemes[NUMBER_TITLE_PHONEMES][30] = {
"glorp",
"snarg",
"gana",
"flin",
"herba",
"pora",
"nuglo",
"greep",
"nur",
"lofa",
"poder",
"nidge",
"pus",
"wooz",
"flem",
"bloto",
"porta"
};

char itemColors[NUMBER_ITEM_COLORS][30] = {
"crimson",
"scarlet",
"orange",
"yellow",
"green",
"blue",
"indigo",
"violet",
"puce",
"mauve",
"burgundy",
"turquoise",
"aquamarine",
"gray",
"pink",
"white",
"lavender",
"tan",
"brown",
"cyan",
"black"
};

char itemWoods[NUMBER_ITEM_WOODS][30] = {
"teak",
"oak",
"redwood",
"rowan",
"willow",
"mahogany",
"pine",
"maple",
"bamboo",
"ironwood",
"pearwood",
"birch",
"cherry",
"eucalyptus",
"walnut",
"cedar",
"rosewood",
"yew"
};

char itemMetals[NUMBER_ITEM_METALS][30] = {
"bronze",
"steel",
"brass",
"pewter",
"nickel",
"copper",
"aluminum",
"tungsten",
"titanium",
};

char itemGems[NUMBER_ITEM_GEMS][30] = {
"diamond",
"opal",
"garnet",
"ruby",
"amethyst",
"topaz",
"onyx",
"tourmaline",
"sapphire",
"obsidion",
"malachite",
"aquamarine",
"emerald",
"jade",
"alexandrite",
"agate",
"bloodstone",
"jasper"
};

//typedef struct itemTable {
//	char *name;
//	char *flavor;
//	short frequency;
//	short marketValue;
//	short number;
//	randomRange range;
//} itemTable;

itemTable foodTable[NUMBER_FOOD_KINDS] = {
{"ration of food",	"", "", 3, 25,	1800, {0,0,0}, true, false},
{"mango",			"", "", 1, 15,	1550, {0,0,0}, true, false}
};

itemTable weaponTable[NUMBER_WEAPON_KINDS] = {
{"dagger",				"", "", 1, 100,		0,	{2,	5,	1}, true, false},	// avg dmg 3.5
{"short sword",			"", "", 2, 300,		12, {2,	8,	2}, true, false},	// avg dmg 5
{"mace",				"", "", 3, 500,		14, {3,	12, 2}, true, false},	// avg dmg 7.5
{"long sword",			"", "", 2, 700,		17, {3,	20, 3}, true, false},	// avg dmg 11.5
{"two-handed sword",	"", "", 2, 1200,	21, {5,	30, 4}, true, false},	// avg dmg 17.5
{"dart",				"", "",	1, 15,		0,	{1,	3,	1},	true, false},	// avg dmg 2
{"shuriken",			"", "",	2, 25,		0,	{2,	5,	1}, true, false},	// avg dmg 3.5
{"javelin",				"", "",	2, 40,		15,	{3, 11, 3},	true, false}	// avg dmg 7
};

itemTable armorTable[NUMBER_ARMOR_KINDS] = {
{"leather armor",	"", "", 1, 250,		0,	{30,30,0}, true, false},
{"scale mail",		"", "", 2, 350,		11, {40,40,0}, true, false},
{"chain mail",		"", "", 2, 500,		14, {50,50,0}, true, false},
{"banded mail",		"", "", 2, 800,		16, {70,70,0}, true, false},
{"splint mail",		"", "", 2, 1000,	18, {90,90,0}, true, false},
{"plate mail",		"", "", 2, 1300,	21, {120,120,0}, true, false}
};

itemTable scrollTable[NUMBER_SCROLL_KINDS] = {
{"identify",			itemTitles[0], "",	30,	100,	0,{0,0,0}, false, false},
{"teleportation",		itemTitles[1], "",	10,	500,	0,{0,0,0}, false, false},
{"remove curse",		itemTitles[2], "",	15,	150,	0,{0,0,0}, false, false},
{"enchanting",			itemTitles[3], "",	20,	525,	0,{0,0,0}, false, false},
{"protect armor",		itemTitles[4], "",	10,	400,	0,{0,0,0}, false, false},
{"protect weapon",		itemTitles[5], "",	10,	375,	0,{0,0,0}, false, false},
{"magic mapping",		itemTitles[6], "",	12,	500,	0,{0,0,0}, false, false},
{"aggravate monsters",	itemTitles[7], "",	15,	50,		0,{0,0,0}, false, false},
{"vorpalize weapon",	itemTitles[8], "",	2,	1000,	0,{0,0,0}, false, false},
{"summon monsters",		itemTitles[9], "",	10,	175,	0,{0,0,0}, false, false},
{"cause fear",			itemTitles[10], "",	10,	500,	0,{0,0,0}, false, false},
{"sanctuary",			itemTitles[11], "",	5,	850,	0,{0,0,0}, false, false}
};

itemTable potionTable[NUMBER_POTION_KINDS] = {
{"healing",				itemColors[0], "",	30,	150,	0,{0,0,0}, false, false},
{"extra healing",		itemColors[1], "",	15,	750,	0,{0,0,0}, false, false},
{"hallucination",		itemColors[2], "",	10,	500,	0,{0,0,0}, false, false},
{"incineration",		itemColors[3], "",	15,	500,	0,{0,0,0}, false, false},
{"gain level",			itemColors[4], "",	4,	850,	0,{0,0,0}, false, false},
{"gain strength",		itemColors[5], "",	25,	400,	0,{0,0,0}, false, false},
{"restore strength",	itemColors[6], "",	20,	200,	0,{0,0,0}, false, false},
{"weakness",			itemColors[7], "",	15,	50,		0,{0,0,0}, false, false},
{"poisonous gas",		itemColors[8], "",	15,	200,	0,{0,0,0}, false, false},
{"paralysis",			itemColors[9], "",	10, 250,	0,{0,0,0}, false, false},
{"telepathy",			itemColors[10], "",	20,	350,	0,{0,0,0}, false, false},
{"levitation",			itemColors[11], "",	15,	250,	0,{0,0,0}, false, false},
{"detect magic",		itemColors[12], "",	20,	300,	0,{0,0,0}, false, false},
{"confusion",			itemColors[13], "",	15,	450,	0,{0,0,0}, false, false},
{"slowness",			itemColors[14], "",	15,	50,		0,{0,0,0}, false, false},
{"speed",				itemColors[15], "",	10,	500,	0,{0,0,0}, false, false},
{"fire immunity",		itemColors[16], "",	15,	500,	0,{0,0,0}, false, false}
};

itemTable wandTable[NUMBER_WAND_KINDS] = {
{"teleportation",	itemMetals[0], "",	1,	800,	0,{2,5,1}, false, false},
{"slow monster",	itemMetals[1], "",	1,	800,	0,{2,5,1}, false, false},
{"haste monster",	itemMetals[2], "",	1,	10,		0,{4,8,1}, false, false},
{"polymorph",		itemMetals[3], "",	1,	700,	0,{3,5,1}, false, false},
{"invisibility",	itemMetals[4], "",	1,	10,		0,{3,5,1}, false, false},
{"cancellation",	itemMetals[5], "",	1,	550,	0,{4,6,1}, false, false}
};

itemTable staffTable[NUMBER_STAFF_KINDS] = {
{"lightning",		itemWoods[0], "",	3,	1300,	0,{2,4,1}, false, false},
{"firebolt",		itemWoods[1], "",	3,	1300,	0,{2,4,1}, false, false},
{"tunneling",		itemWoods[2], "",	2,	900,	0,{2,4,1}, false, false},
{"blinking",		itemWoods[3], "",	2,	1100,	0,{2,4,1}, false, false},
{"entrancement",	itemWoods[4], "",	1,	1100,	0,{2,4,1}, false, false},
{"healing",			itemWoods[5], "",	2,	1100,	0,{2,4,1}, false, false},
{"obstruction",		itemWoods[6], "",	2,	1000,	0,{2,4,1}, false, false},
};

color *boltColors[NUMBER_BOLT_KINDS] = {
&blue,				// teleport other
&green,				// slow
&orange,			// haste
&purple,			// polymorph
&darkBlue,			// invisibility
&pink,				// cancellation
&lightningColor,	// lightning
&lavaForeColor,		// fire
&brown,				// tunneling
&white,				// blinking
&yellow,			// light
&darkRed,			// healing
&forceFieldColor,	// obstruction
};

itemTable ringTable[NUMBER_RING_KINDS] = {
{"clairvoyance",	itemGems[0], "",	1,	900,	0,{1,3,1}, false, false},
{"stealth",			itemGems[1], "",	1,	800,	0,{1,3,1}, false, false},
{"regeneration",	itemGems[2], "",	1,	750,	0,{1,3,1}, false, false},
{"transference",	itemGems[3], "",	1,	750,	0,{1,3,1}, false, false},
{"perception",		itemGems[4], "",	1,	600,	0,{1,3,1}, false, false},
{"awareness",		itemGems[5], "",	1,	700,	0,{1,3,1}, false, false}
};