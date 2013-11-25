//
//  RogueMain.h
//  Brogue
//
//  Created by Brian Walker on 12/26/08.
//  Copyright 2010. All rights reserved.
//  
//  This file is part of Brogue.
//
//  Brogue is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  Brogue is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with Brogue.  If not, see <http://www.gnu.org/licenses/>.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "PlatformDefines.h"

// unicode: comment this line to revert to ASCII

#define USE_UNICODE

// debug macros -- define DEBUGGING as 1 to enable debugging.

#define DEBUGGING			0
#define DEBUG				if (DEBUGGING)
#define MONSTERS_ENABLED	(!DEBUGGING || 1)
#define ITEMS_ENABLED		(!DEBUGGING || 1)

#define D_BULLET_TIME		(DEBUGGING && 0)
#define D_SAFETY_VISION		(DEBUGGING && 0)
#define D_WORMHOLING		(DEBUGGING && 1)


#define boolean			char

#define false			0
#define true			1

// Allows unicode characters:
#define uchar			unsigned short

// Size of each tile (largely a relic):
#define	VERT_PX			18
#define	HORIZ_PX		11

#define MESSAGE_LINES	3

// Size of the entire terminal window. These need to be hard-coded here and in Viewport.h
#define COLS			100			// was 80
#define ROWS			(29 + MESSAGE_LINES)

// Size of the portion of the terminal window devoted to displaying the dungeon:
#define DCOLS			80
#define DROWS			(ROWS - MESSAGE_LINES - 1)	// n lines at the top for messages;
									// one line at the bottom for flavor text.

#define STAT_BAR_WIDTH	20			// number of characters in the stats bar to the left of the map

#define LOS_SLOPE_GRANULARITY 32768 // how finely we divide up the squares when calculating slope;
									// higher numbers mean fewer artifacts but more memory and processing

#define VISIBILITY_THRESHOLD 50		// how bright cumulative light has to be before the cell is marked visible

#define AMULET_LEVEL	26

// display characters:

#ifdef USE_UNICODE

#define FLOOR_CHAR		0x00b7
#define WATER_CHAR		'~'
#define CHASM_CHAR		0x2237
#define TRAP_CHAR		0x25c7
#define FIRE_CHAR		0x22CF
#define GRASS_CHAR		'"'
#define BRIDGE_CHAR		'='
#define DOWN_CHAR		'>'
#define UP_CHAR			'<'
#define WALL_CHAR		'#'
#define DOOR_CHAR		'+'
#define ASH_CHAR		'\''
#define BONES_CHAR		','
#define WEB_CHAR		':'
//#define FOLIAGE_CHAR	0x03A8 // lower-case psi
#define FOLIAGE_CHAR	0x2648 // Aries symbol
//#define FOLIAGE_CHAR	'&'

#define T_FOLIAGE_CHAR	'"'		// 0x2034 // 0x2037

#define PLAYER_CHAR		'@'

#define AMULET_CHAR		0x2640
#define FOOD_CHAR		':'
#define SCROLL_CHAR		0x266A//'?'		// 0x039E
#define RING_CHAR		0xffee
#define POTION_CHAR		'!'
#define ARMOR_CHAR		'['
#define WEAPON_CHAR		0x2191
#define STAFF_CHAR		'\\'
#define WAND_CHAR		'~'
#define GOLD_CHAR		'*'
#define GEM_CHAR		0x25cf
#define TOTEM_CHAR		0x26b2
#define TURRET_CHAR		0x25cf

#define CHAIN_TOP_LEFT		'\\'
#define CHAIN_BOTTOM_RIGHT	'\\'
#define CHAIN_TOP_RIGHT		'/'
#define CHAIN_BOTTOM_LEFT	'/'
#define CHAIN_TOP			'|'
#define CHAIN_BOTTOM		'|'
#define CHAIN_LEFT			'-'
#define CHAIN_RIGHT			'-'

#define BAD_MAGIC_CHAR	0x29F2
#define GOOD_MAGIC_CHAR	0x29F3

#else

#define FLOOR_CHAR		'.'
#define WATER_CHAR		'~'
#define CHASM_CHAR		':'
#define TRAP_CHAR		'%'
#define FIRE_CHAR		'^'
#define GRASS_CHAR		'"'
#define BRIDGE_CHAR		'='
#define DOWN_CHAR		'>'
#define UP_CHAR			'<'
#define WALL_CHAR		'#'
#define DOOR_CHAR		'+'
#define ASH_CHAR		'\''
#define BONES_CHAR		','
#define WEB_CHAR		':'
#define FOLIAGE_CHAR	'&'

#define T_FOLIAGE_CHAR	'"'

#define PLAYER_CHAR		'@'

#define AMULET_CHAR		','
#define FOOD_CHAR		':'
#define SCROLL_CHAR		'?'
#define RING_CHAR		'='
#define POTION_CHAR		'!'
#define ARMOR_CHAR		'['
#define WEAPON_CHAR		'('
#define STAFF_CHAR		'\\'
#define WAND_CHAR		'~'
#define GOLD_CHAR		'*'
#define GEM_CHAR		'+'
#define TOTEM_CHAR		'0'
#define TURRET_CHAR		'*'

#define CHAIN_TOP_LEFT		'\\'
#define CHAIN_BOTTOM_RIGHT	'\\'
#define CHAIN_TOP_RIGHT		'/'
#define CHAIN_BOTTOM_LEFT	'/'
#define CHAIN_TOP			'|'
#define CHAIN_BOTTOM		'|'
#define CHAIN_LEFT			'-'
#define CHAIN_RIGHT			'-'

#define BAD_MAGIC_CHAR	'+'
#define GOOD_MAGIC_CHAR	'$'

#endif

enum eventTypes {
	KEYSTROKE,
	MOUSE_UP,
	MOUSE_DOWN, // unused
	MOUSE_ENTERED_CELL,
	REPEAT_PREVIOUS_EVENT // unused
};

typedef struct rogueEvent {
	enum eventTypes eventType;
	unsigned short int param1;
	char param2;
	boolean controlKey;
	boolean shiftKey;
} rogueEvent;

typedef struct rogueHighScoresEntry {
	signed long score;
	char date[DCOLS];
	char description[DCOLS];
} rogueHighScoresEntry;

//typedef struct distanceQueue {
//	short *qX, *qY, *qVal;
//	short qLen, qMaxLen;
//	short qMinVal, qMinCount;
//} distanceQueue;

enum directions {
	// Cardinal directions; must be 0-3:
	UP				= 0,
	DOWN			= 1,
	LEFT			= 2,
	RIGHT			= 3,
	// Secondary directions; must be 4-7:
	UPLEFT			= 4,
	DOWNLEFT		= 5,
	UPRIGHT			= 6,
	DOWNRIGHT		= 7
};

enum tileType {
	NOTHING = 0,
	GRANITE,
	FLOOR,
	TOP_WALL,
	BOTTOM_WALL,
	LEFT_WALL,
	RIGHT_WALL,
	PERM_WALL, // e.g. corners of rooms: any tiles that should show up as '#' but cannot be cut into a door.
	DOOR,
	OPEN_DOOR,
	SECRET_DOOR,
	DOWN_STAIRS,
	UP_STAIRS,
	DUNGEON_EXIT,
	TORCH_WALL, // wall lit with a torch
	CRYSTAL_WALL,
	GAS_TRAP_POISON_HIDDEN,
	GAS_TRAP_POISON,
	TRAP_DOOR_HIDDEN,
	TRAP_DOOR,
	GAS_TRAP_PARALYSIS_HIDDEN,
	GAS_TRAP_PARALYSIS,
	GAS_TRAP_CONFUSION_HIDDEN,
	GAS_TRAP_CONFUSION,
	FLAMETHROWER_HIDDEN,
	FLAMETHROWER,
	FLOOD_TRAP_HIDDEN,
	FLOOD_TRAP,
	DEEP_WATER,
	SHALLOW_WATER,
	MUD,
	CHASM,
	CHASM_EDGE,
	LAVA,
	SUNLIGHT_POOL,
	DARKNESS_PATCH,
	ACTIVE_BRIMSTONE,
	INERT_BRIMSTONE,
	OBSIDIAN,
	BRIDGE,
	BRIDGE_EDGE,
	FLOOD_WATER_DEEP,
	FLOOD_WATER_SHALLOW,
	GRASS,
	LUMINESCENT_FUNGUS,
	LICHEN,
	HUMAN_BLOOD,
	GREEN_BLOOD,
	PURPLE_BLOOD,
	ACID_SPLATTER,
	VOMIT,
	URINE,
	ASH,
	PUDDLE,
	BONES,
	RUBBLE,
	ECTOPLASM,
	EMBERS,
	SPIDERWEB,
	FOLIAGE,
	TRAMPLED_FOLIAGE,
	FUNGUS_FOREST,
	TRAMPLED_FUNGUS_FOREST,
	FORCEFIELD,
	MANACLE_TL,
	MANACLE_BR,
	MANACLE_TR,
	MANACLE_BL,
	MANACLE_T,
	MANACLE_B,
	MANACLE_L,
	MANACLE_R,
	PLAIN_FIRE,
	BRIMSTONE_FIRE,
	DRAGON_FIRE,
	GAS_FIRE,
	GAS_EXPLOSION,
	POISON_GAS,
	CONFUSION_GAS,
	ROT_GAS,
	PARALYSIS_GAS,
	METHANE_GAS,
	STEAM,
	NUMBER_TILETYPES,
	UNFILLED_LAKE = 120	// used to mark lakes not yet assigned a liquid type
};

enum lightType {
	NO_LIGHT,
	MINERS_LIGHT,
	BURNING_CREATURE_LIGHT,
	MAGIC_LIGHT,
	WISP_LIGHT,
	SALAMANDER_LIGHT,
	IMP_LIGHT,
	PIXIE_LIGHT,
	LICH_LIGHT,
	SPARK_TURRET_LIGHT,
	BOLT_LIGHT_SOURCE,
	
	TORCH_LIGHT,
	LAVA_LIGHT,
	SUN_LIGHT,
	NEGATIVE_LIGHT,
	FUNGUS_LIGHT,
	FUNGUS_FOREST_LIGHT,
	ECTOPLASM_LIGHT,
	EMBER_LIGHT,
	FIRE_LIGHT,
	BRIMSTONE_FIRE_LIGHT,
	EXPLOSION_LIGHT,
	CONFUSION_GAS_LIGHT,
	FORCEFIELD_LIGHT,
	CRYSTAL_WALL_LIGHT,
	NUMBER_LIGHT_KINDS
};

// Item categories
enum itemCategory {
	FOOD		= 1 << 0,
	WEAPON		= 1 << 1,
	ARMOR		= 1 << 2,
	POTION		= 1 << 3,
	SCROLL		= 1 << 4,
	STAFF		= 1 << 5,
	WAND		= 1 << 6,
	RING		= 1 << 7,
	GOLD		= 1 << 8,
	AMULET		= 1 << 9,
	GEM			= 1 << 10,
	ALL_ITEMS	= (FOOD|POTION|WEAPON|ARMOR|STAFF|WAND|SCROLL|RING|GOLD|AMULET|GEM)
};

enum foodKind {
	RATION,
	FRUIT,
	NUMBER_FOOD_KINDS
};

enum potionKind {
	POTION_HEALING,
	POTION_EXTRA_HEALING,
	POTION_GAIN_LEVEL,
	POTION_TELEPATHY,
	POTION_LEVITATION,
	POTION_DETECT_MAGIC,
	POTION_HASTE_SELF,
	POTION_FIRE_IMMUNITY,
	POTION_GAIN_STRENGTH,
	POTION_RESTORE_STRENGTH,
	POTION_WEAKNESS,
	POTION_POISON,
	POTION_PARALYSIS,
	POTION_HALLUCINATION,
	POTION_CONFUSION,
	POTION_INCINERATION,
	POTION_LICHEN,
	NUMBER_POTION_KINDS
};

enum weaponKind {
	DAGGER,
	SHORT_SWORD,
	MACE,
	LONG_SWORD,
	TWO_HANDED_SWORD,
	DART,
	SHURIKEN,
	JAVELIN,
	NUMBER_WEAPON_KINDS
};

enum weaponEnchants {
	W_POISON,
	W_QUIETUS,
	W_PARALYSIS,
	W_NEGATION,
	W_SLOWING,
	W_CONFUSION,
	W_SLAYING,
	W_MERCY,
	NUMBER_GOOD_WEAPON_ENCHANT_KINDS = W_MERCY,
	W_PLENTY,
	NUMBER_WEAPON_ENCHANT_KINDS
};

enum armorKind {
	LEATHER_ARMOR,
	SCALE_MAIL,
	CHAIN_MAIL,
	BANDED_MAIL,
	SPLINT_MAIL,
	PLATE_MAIL,
	NUMBER_ARMOR_KINDS
};

enum armorEnchants {
	A_TOUGHNESS,
	A_WISDOM,
	A_ABSORPTION,
	A_REPRISAL,
	A_IMMUNITY,
	A_BURDEN,
	NUMBER_GOOD_ARMOR_ENCHANT_KINDS = A_BURDEN,
	A_VULNERABILITY,
	NUMBER_ARMOR_ENCHANT_KINDS,
};

enum wandKind {
	WAND_TELEPORT,
	WAND_BECKONING,
	WAND_SLOW,
	WAND_PLENTY,
	WAND_POLYMORPH,
	WAND_INVISIBILITY,
	WAND_CANCELLATION,
	NUMBER_WAND_KINDS
};

enum staffKind {
	STAFF_LIGHTNING,
	STAFF_FIRE,
	STAFF_POISON,
	STAFF_TUNNELING,
	STAFF_BLINKING,
	STAFF_ENTRANCEMENT,
	STAFF_HEALING,
	STAFF_HASTE,
	STAFF_OBSTRUCTION,
	STAFF_DISCORD,
	NUMBER_STAFF_KINDS
};

// these must be wand bolts, in order, and then staff bolts, in order:
enum boltType {
	BOLT_TELEPORT,
	BOLT_BECKONING,
	BOLT_SLOW,
	BOLT_PLENTY,
	BOLT_POLYMORPH,
	BOLT_INVISIBILITY,
	BOLT_CANCELLATION,
	BOLT_LIGHTNING,
	BOLT_FIRE,
	BOLT_POISON,
	BOLT_TUNNELING,
	BOLT_BLINKING,
	BOLT_ENTRANCEMENT,
	BOLT_HEALING,
	BOLT_HASTE,
	BOLT_OBSTRUCTION,
	BOLT_DISCORD,
	NUMBER_BOLT_KINDS
};

enum ringKind {
	RING_CLAIRVOYANCE,
	RING_STEALTH,
	RING_REGENERATION,
	RING_TRANSFERENCE,
	RING_PERCEPTION,
	RING_AWARENESS,
	RING_REFLECTION,
	NUMBER_RING_KINDS
};

enum scrollKind {
	SCROLL_IDENTIFY,
	SCROLL_TELEPORT,
	SCROLL_REMOVE_CURSE,
	SCROLL_ENCHANT_ITEM,
	SCROLL_PROTECT_ARMOR,
	SCROLL_PROTECT_WEAPON,
	SCROLL_MAGIC_MAPPING,
	SCROLL_AGGRAVATE_MONSTER,
	SCROLL_SUMMON_MONSTER,
	SCROLL_DARKNESS,
	SCROLL_CAUSE_FEAR,
	SCROLL_SANCTUARY,
	NUMBER_SCROLL_KINDS
};

#define MAX_PACK_ITEMS				26

enum monsterTypes {
	MK_YOU,
	MK_RAT,
	MK_KOBOLD,
	MK_JACKAL,
	MK_EEL,
	MK_MONKEY,
	MK_BLOAT,
	MK_GOBLIN,
	MK_GOBLIN_CONJURER,
	MK_GOBLIN_TOTEM,
	MK_SPECTRAL_BLADE,
	MK_PINK_JELLY,
	MK_TOAD,
	MK_VAMPIRE_BAT,
	MK_ARROW_TURRET,
	MK_ACID_MOUND,
	MK_CENTIPEDE,
	MK_OGRE,
	MK_BOG_MONSTER,
	MK_OGRE_TOTEM,
	MK_SPIDER,
	MK_SPARK_TURRET,
	MK_WILL_O_THE_WISP,
	MK_WRAITH,
	MK_ZOMBIE,
	MK_TROLL,
	MK_OGRE_SHAMAN,
	MK_NAGA,
	MK_SALAMANDER,
	MK_DAR_BLADEMASTER,
	MK_DAR_PRIESTESS,
	MK_DAR_BATTLEMAGE,
	MK_CENTAUR,
	MK_ACID_TURRET,
	MK_KRAKEN,
	MK_LICH,
	MK_PIXIE,
	MK_PHANTOM,
	MK_DART_TURRET,
	MK_IMP,
	MK_FURY,
	MK_REVENANT,
	MK_TENTACLE_HORROR,
	MK_GOLEM,
	MK_DRAGON,
	NUMBER_MONSTER_KINDS
};

#define	NUMBER_HORDES				79

// flavors

#define NUMBER_ITEM_COLORS			21
#define NUMBER_TITLE_PHONEMES		17
#define NUMBER_ITEM_WOODS			18
#define NUMBER_POTION_DESCRIPTIONS	18
#define NUMBER_ITEM_METALS			9
#define NUMBER_ITEM_GEMS			18
#define NUMBER_GOOD_ITEMS			13

// Dungeon flags
enum tileFlags {
	DISCOVERED					= 1 << 0,
	VISIBLE						= 1 << 1,	// cell has sufficient light and is in field of view, ready to draw.
	HAS_PLAYER					= 1 << 2,
	HAS_MONSTER					= 1 << 3,
	HAS_ITEM					= 1 << 4,
	IN_FIELD_OF_VIEW			= 1 << 5,	// player has unobstructed line of sight whether or not there is enough light
	WAS_VISIBLE					= 1 << 6,
	HAS_DOWN_STAIRS				= 1 << 7,
	HAS_UP_STAIRS				= 1 << 8,
	IS_IN_SHADOW				= 1 << 9,	// so that a player gains an automatic stealth bonus
	MAGIC_MAPPED				= 1 << 10,
	ITEM_DETECTED				= 1 << 11,
	CLAIRVOYANT_VISIBLE			= 1 << 12,
	WAS_CLAIRVOYANT_VISIBLE		= 1 << 13,
	CLAIRVOYANT_DARKENED		= 1 << 14,	// magical blindness from a cursed ring of clairvoyance
	CAUGHT_FIRE_THIS_TURN		= 1 << 15,	// so that fire does not spread asymmetrically
	PRESSURE_PLATE_DEPRESSED	= 1 << 16,	// so that traps do not trigger repeatedly while you stand on them
	DOORWAY						= 1 << 17,	// so that waypoint paths don't cross open doorways
	STABLE_MEMORY				= 1 << 18,	// redraws will simply be pulled from the memory array, not recalculated
	PLAYER_STEPPED_HERE			= 1 << 19,	// keep track of where the player has stepped as he knows no traps are there
	TERRAIN_COLORS_DANCING		= 1 << 20,
	IN_LOOP						= 1 << 21,	// this cell is part of a terrain loop
	
	PERMANENT_TILE_FLAGS = (DISCOVERED | MAGIC_MAPPED | ITEM_DETECTED | DOORWAY | PRESSURE_PLATE_DEPRESSED
							| STABLE_MEMORY | PLAYER_STEPPED_HERE | TERRAIN_COLORS_DANCING | IN_LOOP),
};

#define TURNS_FOR_FULL_REGEN				300
#define STOMACH_SIZE						2150
#define HUNGER_THRESHOLD					300
#define WEAK_THRESHOLD						150
#define FAINT_THRESHOLD						50
#define MAX_EXP_LEVEL						21
#define MAX_EXP								10000000L

#define ROOM_MIN_WIDTH						4
#define ROOM_MAX_WIDTH						20
#define ROOM_MIN_HEIGHT						3
#define ROOM_MAX_HEIGHT						7
#define HORIZONTAL_CORRIDOR_MIN_LENGTH		5
#define HORIZONTAL_CORRIDOR_MAX_LENGTH		15
#define VERTICAL_CORRIDOR_MIN_LENGTH		2
#define VERTICAL_CORRIDOR_MAX_LENGTH		10
#define CROSS_ROOM_MIN_WIDTH				3
#define CROSS_ROOM_MAX_WIDTH				12
#define CROSS_ROOM_MIN_HEIGHT				2
#define CROSS_ROOM_MAX_HEIGHT				5
#define MIN_SCALED_ROOM_DIMENSION			2

#define CORRIDOR_WIDTH						1

#define MAX_WAYPOINTS						200
#define WAYPOINT_SIGHT_RADIUS				10

typedef struct levelSpecProfile {
	short roomMinWidth;
	short roomMaxWidth;
	short roomMinHeight;
	short roomMaxHeight;
	short horCorrMinLength;
	short horCorrMaxLength;
	short vertCorrMinLength;
	short vertCorrMaxLength;
	short crossRoomMinWidth;
	short crossRoomMaxWidth;
	short crossRoomMinHeight;
	short crossRoomMaxHeight;
	short secretDoorChance;
	short numberOfTraps;
} levelSpecProfile;

// Making these larger means cave generation will take more trials; set them too high and the program will hang.
#define CAVE_MIN_WIDTH						50
#define CAVE_MIN_HEIGHT						20

// Keyboard commands:
#define UP_KEY				'k'
#define DOWN_KEY			'j'
#define LEFT_KEY			'h'
#define RIGHT_KEY			'l'
#define UP_ARROW			63232
#define LEFT_ARROW			63234
#define DOWN_ARROW			63233
#define RIGHT_ARROW			63235
#define UPLEFT_KEY			'y'
#define UPRIGHT_KEY			'u'
#define DOWNLEFT_KEY		'b'
#define DOWNRIGHT_KEY		'n'
#define DESCEND_KEY			'>'
#define ASCEND_KEY			'<'
#define REST_KEY			'z'
#define AUTO_REST_KEY		'Z'
#define SEARCH_KEY			's'
#define INVENTORY_KEY		'i'
#define ACKNOWLEDGE_KEY		' '
#define EQUIP_KEY			'e'
#define UNEQUIP_KEY			'r'
#define APPLY_KEY			'a'
#define THROW_KEY			't'
#define DROP_KEY			'd'
#define CALL_KEY			'c'
#define FIGHT_KEY			'f'
#define FIGHT_TO_DEATH_KEY	'F'
#define HELP_KEY			'?'
#define DISCOVERIES_KEY		'D'
#define REPEAT_TRAVEL_KEY	RETURN_KEY
#define EXAMINE_KEY			'x'
#define EXPLORE_KEY			'X'
#define AUTOPLAY_KEY		'A'
#define SEED_KEY			'~'
#define EASY_MODE_KEY		'&'
#define ESCAPE_KEY			'\033'
#define RETURN_KEY			'\015'
#define ENTER_KEY			'\012'
#define DELETE_KEY			'\177'
#define TAB_KEY				'\t'
#define PERIOD_KEY			'.'
#define NUMPAD_0			48
#define NUMPAD_1			49
#define NUMPAD_2			50
#define NUMPAD_3			51
#define NUMPAD_4			52
#define NUMPAD_5			53
#define NUMPAD_6			54
#define NUMPAD_7			55
#define NUMPAD_8			56
#define NUMPAD_9			57

#define min(x, y)		(((x) < (y)) ? (x) : (y))
#define max(x, y)		(((x) > (y)) ? (x) : (y))

#define terrainFlags(x, y)					(tileCatalog[pmap[x][y].layers[DUNGEON]].flags \
											| tileCatalog[pmap[x][y].layers[LIQUID]].flags \
											| tileCatalog[pmap[x][y].layers[SURFACE]].flags \
											| tileCatalog[pmap[x][y].layers[GAS]].flags)
#define cellHasTerrainFlag(x, y, flagMask)	((flagMask) & terrainFlags((x), (y)) ? true : false)
#define cellHasTerrainType(x, y, terrain)	((pmap[x][y].layers[DUNGEON] == (terrain) \
											|| pmap[x][y].layers[LIQUID] == (terrain) \
											|| pmap[x][y].layers[SURFACE] == (terrain) \
											|| pmap[x][y].layers[GAS] == (terrain)) ? true : false)
#define coordinatesAreInMap(x, y) ((x) >= 0 && (x) < DCOLS && (y) >= 0 && (y) < DROWS)

#define CYCLE_MONSTERS_AND_PLAYERS(x)	for ((x) = &player; (x) != NULL; (x) = ((x) == &player ? monsters->nextCreature : (x)->nextCreature))

#define ASSERT(x)		if (!(x)) { printf("\nassert failure"); 1/(0);}

// structs

enum dungeonLayers {
	DUNGEON = 0,		// dungeon-level tile	(e.g. walls)
	LIQUID,				// liquid-level tile	(e.g. lava)
	GAS,				// gas-level tile		(e.g. fire, smoke, swamp gas)
	SURFACE,			// surface-level tile	(e.g. grass)
	NUMBER_TERRAIN_LAYERS
};

// keeps track of graphics so we only redraw if the cell has changed:
typedef struct cellDisplayBuffer {
	uchar character;
	char foreColorComponents[3];
	char backColorComponents[3];
	char opacity;
	boolean needsUpdate;
} cellDisplayBuffer;

typedef struct pcell {								// permanent cell; have to remember this stuff to save levels
	enum tileType layers[NUMBER_TERRAIN_LAYERS];	// terrain
	unsigned long flags;							// non-terrain cell flags
	unsigned short volume;							// quantity of volumetric medium in cell
	cellDisplayBuffer rememberedAppearance;			// how the player remembers the cell to look
	enum itemCategory rememberedItemCategory;		// what category of item the player remembers lying there
	enum tileType rememberedTerrain;				// what the player remembers as the terrain (i.e. highest priority terrain upon last seeing)
} pcell;

typedef struct tcell {			// transient cell; stuff we don't need to remember between levels
	unsigned short scent;		// scent value of the cell
	short light[3];				// RGB components of lighting
	short oldLight[3];			// compare with subsequent lighting to determine whether to refresh cell
	char connected;				// used in dungeon generation to keep track of connected regions
} tcell;

typedef struct randomRange {
	short lowerBound;
	short upperBound;
	short clumpFactor;
} randomRange;

typedef struct color {
	// base RGB components:
	short red;
	short green;
	short blue;
	// random RGB components to add to base components:
	short redRand;
	short greenRand;
	short blueRand;
	// random scalar to add to all components:
	short rand;
	// Flag: this color "dances" with every refresh:
	boolean colorDances;
} color;

typedef struct door {
	short x;
	short y;
	enum directions direction;
} door;

typedef struct room {
	short roomX;
	short roomY;
	short width;
	short height;
	short roomX2;
	short roomY2;
	short width2;
	short height2;
	short numberOfSiblings;
	struct door doors[20];
	struct room *siblingRooms[20];
	struct room *nextRoom;
	short pathNumber; // used to calculate the distance (in rooms) between rooms
} room;

typedef struct waypoint {
	short x;
	short y;
	short pointsTo[2];
	short connectionCount;
	short connection[10][2];
} waypoint;

enum itemFlags {
	ITEM_IDENTIFIED			= 1	<< 0,
	ITEM_EQUIPPED			= 1 << 1,
	ITEM_CURSED				= 1 << 2,
	ITEM_PROTECTED			= 1 << 3,
	ITEM_NO_PICKUP			= 1 << 4,
	ITEM_RUNIC				= 1 << 5,
	ITEM_RUNIC_HINTED		= 1 << 6,
	ITEM_RUNIC_IDENTIFIED	= 1 << 7,
	ITEM_CAN_BE_IDENTIFIED	= 1 << 8,
	ITEM_PREPLACED			= 1 << 9,
	ITEM_FLAMMABLE			= 1 << 10,
	ITEM_MAGIC_DETECTED		= 1 << 11,
	ITEM_NAMED				= 1 << 12,
	ITEM_MAX_CHARGES_KNOWN	= 1 << 13,
};

typedef struct item {
	unsigned short category;
	short kind;
	unsigned long flags;
	randomRange damage;
	short armor;
	short charges;
	short enchant1;
	short enchant2;
	enum monsterTypes vorpalEnemy;
	short strengthRequired;
	unsigned short quiverNumber;
	uchar displayChar;
	color *foreColor;
	color *inventoryColor;
	short quantity;
	char inventoryLetter;
	char inscription[DCOLS];
	short xLoc;
	short yLoc;
	struct item *nextItem;
} item;

typedef struct itemTable {
	char *name;
	char *flavor;
	char callTitle[30];
	short frequency;
	short marketValue;
	short strengthRequired;
	randomRange range;
	boolean identified;
	boolean called;
} itemTable;

enum dungeonFeatureTypes {
	DF_GRANITE_COLUMN = 1,
	DF_CRYSTAL_WALL,
	//DF_new,
	//DF_GRANITE_OUTCROP,
	DF_LUMINESCENT_FUNGUS,
	DF_GRASS,
	DF_BONES,
	DF_RUBBLE,
	DF_FOLIAGE,
	DF_FUNGUS_FOREST,
	DF_WALL_TORCH_TOP,
	DF_POISON_GAS_TRAP,
	DF_PARALYSIS_GAS_TRAP,
	DF_TRAPDOOR,
	DF_CONFUSION_GAS_TRAP,
	DF_FLAMETHROWER_TRAP,
	DF_FLOOD_TRAP,
	DF_MUD,
	DF_SUNLIGHT,
	DF_DARKNESS,
	
	DF_SHOW_DOOR,
	DF_SHOW_POISON_GAS_TRAP,
	DF_SHOW_PARALYSIS_GAS_TRAP,
	DF_SHOW_TRAPDOOR_HALO,
	DF_SHOW_TRAPDOOR,
	DF_SHOW_CONFUSION_GAS_TRAP,
	DF_SHOW_FLAMETHROWER_TRAP,
	DF_SHOW_FLOOD_TRAP,
	
	DF_HUMAN_BLOOD,
	DF_GREEN_BLOOD,
	DF_PURPLE_BLOOD,
	DF_ACID_BLOOD,
	DF_ASH_BLOOD,
	DF_ECTOPLASM_BLOOD,
	DF_RUBBLE_BLOOD,
	DF_ROT_GAS_BLOOD,
	
	DF_VOMIT,
	DF_BLOAT_DEATH,
	
	DF_ROT_GAS_PUFF,
	DF_STEAM_PUFF,
	DF_STEAM_ACCUMULATION,
	DF_METHANE_GAS_PUFF,
	DF_SALAMANDER_FLAME,
	DF_URINE,
	DF_PUDDLE,
	DF_ASH,
	DF_ECTOPLASM_DROPLET,
	DF_FORCEFIELD,
	DF_LICHEN_GROW,
	DF_LICHEN_PLANTED,
	
	DF_TRAMPLED_FOLIAGE,
	DF_FOLIAGE_REGROW,
	DF_TRAMPLED_FUNGUS_FOREST,
	DF_FUNGUS_FOREST_REGROW,
	
	DF_ACTIVE_BRIMSTONE,
	DF_INERT_BRIMSTONE,
	
	DF_OPEN_DOOR,
	DF_CLOSED_DOOR,
	
	DF_PLAIN_FIRE,
	DF_GAS_FIRE,
	DF_EXPLOSION_FIRE,
	DF_BRIMSTONE_FIRE,
	DF_BRIDGE_FIRE,
	DF_FLAMETHROWER,
	DF_EMBERS,
	DF_OBSIDIAN,
	DF_FLOOD,
	DF_FLOOD_2,
	DF_FLOOD_DRAIN,
	DF_POISON_GAS_CLOUD,
	DF_PARALYSIS_GAS_CLOUD,
	DF_CONFUSION_GAS_CLOUD,
	DF_CONFUSION_GAS_TRAP_CLOUD,
	DF_METHANE_GAS_ARMAGEDDON,
	NUMBER_DUNGEON_FEATURES,
};

typedef struct lightSource {
	color *lightColor;
	randomRange lightRadius;
	short maxIntensity;
	short radialFadeToPercent;
	boolean passThroughCreatures; // generally no, but miner light does
	
	struct lightSource *nextLight;
	struct creature *followsCreature; // NULL means it's stationary; otherwise tracks the creature and dies when the creature dies
	short xLoc, yLoc;
} lightSource;

// Dungeon features, spawned with the spawnSurfaceEffect() method from Architect.c:
typedef struct dungeonFeature {
	// tile info:
	enum tileType tile;
	enum dungeonLayers layer;
	
	// spawning pattern:
	randomRange startProbability;
	randomRange probabilityDecrement;
	boolean restrictedToPropTerrain;
	enum tileType propagationTerrain;
	enum dungeonFeatureTypes subsequentDF;
	
	enum tileType requiredDungeonFoundationType;
	
	// prevalence:
	short minDepth;
	short maxDepth;
	short frequency;
	short minNumberIntercept;
	short minNumberSlope; // actually slope * 100
	short maxNumber;
} dungeonFeature;

typedef struct floorTileType {
	// appearance:
	uchar displayChar;
	struct color *foreColor;
	struct color *backColor;
	// draw priority (lower number means higher priority):
	short drawPriority;
	// settings related to fire:
	char chanceToIgnite;					// doubles as chance to extinguish once ignited, if IS_FIRE set
	enum dungeonFeatureTypes fireType;		// doubles as terrain type remaining after extinguished, if IS_FIRE is set
	enum dungeonFeatureTypes discoverType;	// spawn this DF when successfully searched if IS_SECRET is set
	enum dungeonFeatureTypes promoteType;	// creates this dungeon spawn type
	short promoteChance;					// percent chance per turn to spawn the promotion type; will also vanish upon doing so if VANISHES_UPON_PROMOTION is set
	short glowLight;						// if it glows, this is the ID of the light type
	unsigned long flags;
	char description[COLS];
	char flavorText[COLS];
} floorTileType;

enum terrainFlags {
	OBSTRUCTS_PASSABILITY			= 1 << 0,		// cannot be walked through
	OBSTRUCTS_VISION				= 1 << 1,		// blocks line of sight
	OBSTRUCTS_ITEMS					= 1 << 2,		// items can't be on this tile
	OBSTRUCTS_SCENT					= 1 << 3,		// blocks the permeation of scent
	OBSTRUCTS_SURFACE_EFFECTS		= 1 << 4,		// grass, blood, etc. cannot exist on this tile
	OBSTRUCTS_GAS					= 1 << 5,		// blocks the permeation of gas
	PERMITS_ASCENT					= 1 << 6,		// functions as an up-staircase
	PERMITS_DESCENT					= 1 << 7,		// functions as a down-staircase
	SPONTANEOUSLY_IGNITES			= 1 << 8,		// monsters avoid unless chasing player or immune to fire
	TRAP_DESCENT					= 1 << 9,		// automatically drops the player a depth level and does some damage (2d6)
	PROMOTES_ON_STEP				= 1 << 10,		// promotes when a non-levitating creature steps on the tile
	GLOWS							= 1 << 11,		// intrinsic light
	LAVA_INSTA_DEATH				= 1 << 12,		// kills any non-levitating non-fire-immune creature instantly
	CAUSES_POISON					= 1 << 13,		// any non-levitating creature gets 10 poison
	IS_FLAMMABLE					= 1 << 14,		// terrain can catch fire
	IS_FIRE							= 1 << 15,		// terrain is a type of fire; ignites neighboring flammable cells
	EXTINGUISHES_FIRE				= 1 << 16,		// extinguishes burning terrain or creatures
	ENTANGLES						= 1 << 17,		// entangles players and monsters like a spiderweb
	IS_DEEP_WATER					= 1 << 18,		// steals items 50% of the time and moves them around randomly
	ALLOWS_SUBMERGING				= 1 << 19,		// allows submersible monsters to submerge in this terrain
	IS_SECRET						= 1 << 20,		// successful search or being stepped on while visible transforms it into discoverType
	// unused						= 1 << 21,		// 
	GAS_DISSIPATES					= 1 << 22,		// does not just hang in the air forever
	GAS_DISSIPATES_QUICKLY			= 1 << 23,		// dissipates quickly
	CAUSES_DAMAGE					= 1 << 24,		// anything on the tile takes max(1-2, 10%) damage per turn
	CAUSES_NAUSEA					= 1 << 25,		// any creature on the tile becomes nauseous
	CAUSES_PARALYSIS				= 1 << 26,		// anything caught on this tile is paralyzed
	CAUSES_CONFUSION				= 1 << 27,		// causes creatures on this tile to become confused
	IS_DF_TRAP						= 1 << 28,		// spews gas of type specified in fireType with volume 1000 when stepped on
	STAND_IN_TILE					= 1 << 29,		// earthbound creatures will be said to stand "in" the tile, not on it
	VANISHES_UPON_PROMOTION			= 1 << 30,		// vanishes when randomly creating promotion dungeon feature;
													// can be overwritten anyway depending on priorities and layers of dungeon feature
	CAUSES_EXPLOSIVE_DAMAGE			= 1 << 31,		// is an explosion; deals 15-20 or 50% damage instantly but not again for five turns
	
	PATHING_BLOCKER					= (OBSTRUCTS_PASSABILITY | TRAP_DESCENT | IS_DF_TRAP | LAVA_INSTA_DEATH | IS_DEEP_WATER | IS_FIRE | SPONTANEOUSLY_IGNITES),
	WAYPOINT_BLOCKER				= (OBSTRUCTS_PASSABILITY | TRAP_DESCENT | IS_DF_TRAP | LAVA_INSTA_DEATH | IS_DEEP_WATER | SPONTANEOUSLY_IGNITES),
	MOVES_ITEMS						= (IS_DEEP_WATER | LAVA_INSTA_DEATH),
	CAN_BE_BRIDGED					= (TRAP_DESCENT), // | ALLOWS_SUBMERGING | LAVA_INSTA_DEATH),
	OBSTRUCTS_EVERYTHING			= (OBSTRUCTS_PASSABILITY | OBSTRUCTS_VISION | OBSTRUCTS_ITEMS | OBSTRUCTS_GAS
									   | OBSTRUCTS_SCENT | OBSTRUCTS_SURFACE_EFFECTS),
};

typedef struct statusEffects {
	short telepathic;			// can locate monsters for this long
	short hallucinating;
	short blind;
	short levitating;			// levitating for this long
	short slowed;				// speed is halved for this long
	short hasted;				// speed is doubled for this long
	short confused;				// movement and aiming is random
	short burning;				// takes damage per turn for this long or until entering certain liquids
	short paralyzed;			// creature gets no turns
	short poisoned;				// creature loses 1hp per turn and does not regenerate
	short stuck;				// creature can do anything but move
	short nauseous;				// every so often the creature will vomit instead of moving
	short discordant;			// creature is enemies with all other creatures
	short immuneToFire;
	short explosionImmunity;	// so that repeat explosions only have an effect once every five turns at the most
	short nutrition;			// opposite of hunger; monsters do not use this unless allied
	short entersLevelIn;		// this many ticks until it follows the player up/down the stairs
	short magicalFear;			// monster will flee
	short entranced;			// monster will mimic player movements
	short darkness;				// player's miner's light is diminished
} statusEffects;

enum hordeFlags {
	HORDE_DIES_ON_LEADER_DEATH		= 1 << 1,	// if the leader dies, the horde will die instead of electing new leader
	HORDE_IS_SUMMONED				= 1 << 2,	// minions summoned when any creature is the same species as the leader and casts summon
	HORDE_LEADER_CAPTIVE			= 1 << 3,	// the leader is in chains and the followers are guards
	NO_PERIODIC_SPAWN				= 1 << 4,	// can spawn only when the level begins -- not afterwards
};

#define Fl(N)	(1 << (N))

enum monsterBehaviorFlags {
	MONST_INVISIBLE					= Fl(0),	// monster is invisible
	MONST_INANIMATE					= Fl(1),	// monster has abbreviated stat bar display and is immune to many things
	MONST_IMMOBILE					= Fl(2),	// monster won't move or perform melee attacks
	MONST_CARRY_ITEM_100			= Fl(3),	// monster carries an item 100% of the time
	MONST_CARRY_ITEM_25				= Fl(4),	// monster carries an item 25% of the time
	MONST_ALWAYS_HUNTING			= Fl(5),	// monster is never asleep or in wandering mode
	MONST_FLEES_NEAR_DEATH			= Fl(6),	// monster flees when under 25% health and re-engages when over 75%
	MONST_ATTACKABLE_THRU_WALLS		= Fl(7),	// can be attacked when embedded in a wall
	MONST_DEFEND_DEGRADE_WEAPON		= Fl(8),	// hitting the monster damages the weapon
	MONST_IMMUNE_TO_WEAPONS			= Fl(9),	// weapons ineffective
	MONST_FLIES						= Fl(10),	// permanent levitation
	MONST_FLITS						= Fl(11),	// moves randomly a third of the time
	MONST_IMMUNE_TO_FIRE			= Fl(12),	// won't burn, won't die in lava
	MONST_CAST_SPELLS_SLOWLY		= Fl(13),	// takes twice the attack duration to cast a spell
	MONST_IMMUNE_TO_WEBS			= Fl(14),	// monster passes freely through webs
	MONST_REFLECT_4					= Fl(15),	// monster reflects projectiles as though wearing a level 4 ring of reflection
	MONST_NEVER_SLEEPS				= Fl(16),	// monster is always awake
	MONST_FIERY						= Fl(17),	// monster carries an aura of flame (but no automatic fire light)
	MONST_INTRINSIC_LIGHT			= Fl(18),	// monster carries an automatic light of the specified kind
	MONST_IMMUNE_TO_WATER			= Fl(19),	// monster moves at full speed in deep water and (if player) doesn't drop items
	MONST_RESTRICTED_TO_LIQUID		= Fl(20),	// monster can move only on tiles that allow submersion
	MONST_SUBMERGES					= Fl(21),	// monster can submerge in appropriate terrain
	MONST_MAINTAINS_DISTANCE		= Fl(22),	// monster tries to keep a distance of 3 tiles between it and player
	MONST_TELEPORTS					= Fl(23),	// monster can teleport when fleeing
	MONST_WILL_NOT_USE_STAIRS		= Fl(24),	// monster won't chase the player between levels
	
	CANCELLABLE_TRAITS				= (MONST_INVISIBLE | MONST_DEFEND_DEGRADE_WEAPON | MONST_IMMUNE_TO_WEAPONS | MONST_FLIES
									   | MONST_FLITS | MONST_IMMUNE_TO_FIRE | MONST_IMMUNE_TO_WEBS | MONST_FIERY
									   | MONST_IMMUNE_TO_WATER | MONST_SUBMERGES | MONST_TELEPORTS | MONST_MAINTAINS_DISTANCE),
	MONST_TURRET					= (MONST_IMMUNE_TO_WEBS | MONST_NEVER_SLEEPS | MONST_IMMOBILE | MONST_INANIMATE |
									   MONST_ALWAYS_HUNTING | MONST_ATTACKABLE_THRU_WALLS | MONST_WILL_NOT_USE_STAIRS),
};

enum monsterAbilityFlags {
	MA_HIT_HALLUCINATE				= 1 << 0,	// monster can hit to cause hallucinations
	MA_HIT_STEAL_FLEE				= 1 << 1,	// monster can steal an item and then run away
	MA_HIT_SAP_STRENGTH				= 1 << 2,	// monster hits to weaken
	MA_HIT_DEGRADE_ARMOR			= 1 << 3,	// monster damages armor
	MA_CAST_HEAL					= 1 << 4,
	MA_CAST_HASTE					= 1 << 5,
	MA_CAST_SUMMON					= 1 << 6,	// requires that there be one or more summon hordes with this monster type as the leader
	MA_CAST_BLINK					= 1 << 7,
	MA_CAST_CANCEL					= 1 << 8,
	MA_CAST_SPARK					= 1 << 9,
	MA_CAST_FIRE					= 1 << 10,
	MA_CAST_SLOW					= 1 << 11,
	MA_CAST_DISCORD					= 1 << 12,
	MA_BREATHES_FIRE				= 1 << 13,	// shoots dragonfire at player from a distance
	MA_SHOOTS_WEBS					= 1 << 14,	// monster shoots webs at the player
	MA_ATTACKS_FROM_DISTANCE		= 1 << 15,	// monster shoots from a distance for its attack
	MA_SEIZES						= 1 << 16,	// monster seizes enemies before attacking and cannot attack flying enemies
	MA_POISONS						= 1 << 17,	// monster's damage is dealt in the form of poison, and it flees a poisoned player
	MA_DF_ON_DEATH					= 1 << 18,	// monster spawns its DF when it dies
	MA_CLONE_SELF_ON_DEFEND			= 1 << 19,	// monster splits in two when struck
	MA_KAMIKAZE						= 1 << 20,	// monster dies instead of attacking
	
	MAGIC_ATTACK					= (MA_CAST_HEAL | MA_CAST_HASTE | MA_CAST_BLINK | MA_CAST_CANCEL | MA_CAST_SPARK | MA_CAST_FIRE | MA_CAST_SUMMON
									   | MA_CAST_SLOW | MA_CAST_DISCORD | MA_BREATHES_FIRE | MA_SHOOTS_WEBS | MA_ATTACKS_FROM_DISTANCE),
	SPECIAL_HIT						= (MA_HIT_HALLUCINATE | MA_HIT_STEAL_FLEE | MA_HIT_SAP_STRENGTH | MA_HIT_DEGRADE_ARMOR | MA_POISONS),
};

enum monsterBookkeepingFlags {
	MONST_WAS_VISIBLE				= 1 << 0,	// monster was visible to player last turn
	MONST_ADDED_TO_STATS_BAR		= 1 << 1,	// monster has been polled for the stats bar
	MONST_PREPLACED					= 1 << 2,	// monster dropped onto the level and requires post-processing
	MONST_APPROACHING_UPSTAIRS		= 1 << 3,	// following the player up the stairs
	MONST_APPROACHING_DOWNSTAIRS	= 1 << 4,	// following the player down the stairs
	MONST_APPROACHING_PIT			= 1 << 5,	// following the player down a pit
	MONST_LEADER					= 1 << 6,	// monster is the leader of a horde
	MONST_FOLLOWER					= 1 << 7,	// monster is a member of a horde
	MONST_CAPTIVE					= 1 << 8,	// monster is all tied up
	MONST_SEIZED					= 1 << 9,	// monster is being held
	MONST_SEIZING					= 1 << 10,	// monster is holding another creature immobile
	MONST_SUBMERGED					= 1 << 11,	// monster is currently submerged and hence invisible until it attacks
	MONST_JUST_SUMMONED				= 1 << 12,	// used to mark summons so they can be post-processed
	MONST_WILL_FLASH				= 1 << 13,	// this monster will flash as soon as control is returned to the player
	MONST_BOUND_TO_LEADER			= 1 << 14,	// monster will die if the leader dies or becomes separated from the leader
};

// Defines all creatures, which include monsters and the player:
typedef struct creatureType {
	enum monsterTypes monsterID; // index number for the monsterCatalog
	char monsterName[COLS];
	uchar displayChar;
	struct color *foreColor;
	short expForKilling;
	short maxHP;
	short defense;
	short accuracy;
	randomRange damage;
	long turnsBetweenRegen;		// turns to wait before regaining 1 HP
	short sightRadius;
	short scentThreshold;
//	short soundThreshold;
	short movementSpeed;
	short attackSpeed;
	enum dungeonFeatureTypes bloodType;
	enum lightType intrinsicLightType;
	short DFChance;						// percent chance to spawn the dungeon feature per awake turn
	enum dungeonFeatureTypes DFType;	// kind of dungeon feature
//	enum monsterFlags
	unsigned long flags;
	unsigned long abilityFlags;
} creatureType;

typedef struct monsterWords {
	char flavorText[COLS*5];
	char pronoun[10]; // subjective
	char attack[5][30];
	char DFMessage[DCOLS];
	char summonMessage[DCOLS];
} monsterWords;

//#define PLAYER_BASE_MOVEMENT_SPEED		100
//#define PLAYER_BASE_ATTACK_SPEED		100

enum creatureStates {
	MONSTER_SLEEPING,
	MONSTER_TRACKING_SCENT,
	MONSTER_WANDERING,
	MONSTER_FLEEING,
	MONSTER_ALLY,
};

enum creatureModes {
	MODE_NORMAL,
	MODE_PERM_FLEEING
};

typedef struct hordeType {
	enum monsterTypes leaderType;
	
	// membership information
	short numberOfMemberTypes;
	enum monsterTypes memberType[5];
	randomRange memberCount[5];
	
	// spawning information
	short minLevel;
	short maxLevel;
	short frequency;
	enum tileType spawnsIn;
	
	enum hordeFlags flags;
} hordeType;

typedef struct creature {
	creatureType info;
	short xLoc;
	short yLoc;
	short currentHP;
	long turnsUntilRegen;
	short regenPerTurn;					// number of HP to regenerate every single turn
	enum creatureStates creatureState;	// current behavioral state
	enum creatureModes creatureMode;	// current behavioral mode (higher-level than state)
	short destination[2][2];			// the waypoints the monster is walking towards
	short comingFrom[2];				// the location the monster is walking from when wandering to avoid going back and forth
	short **mapToMe;					// if a pack leader, this is a periodically updated pathing map to get to the leader
	short **safetyMap;					// fleeing monsters store their own safety map when out of player FOV to avoid omniscience
	room *comingFromRoom;				// room the monster is coming from and will try to avoid returning immediately to
	short ticksUntilTurn;				// how long before the creature gets its next move
	short movementSpeed;
	short attackSpeed;
	short turnsSpentStationary;			// how many (subjective) turns it's been since the creature moved between tiles
	short flashStrength;				// monster will flash soon; this indicates the percent strength of flash
	color flashColor;					// the color that the monster will flash
	statusEffects status;
	statusEffects maxStatus;			// used to set the max point on the status bars
	unsigned long bookkeepingFlags;
	struct creature *leader;			// only if monster is a follower
	struct creature *nextCreature;
	struct item *carriedItem;			// only used for monsters
	struct lightSource *statusLight;	// for things like burning
	struct lightSource *intrinsicLight; // if the creature naturally glows
} creature;

// these are basically global variables pertaining to the game state and player's unique variables:
typedef struct playerCharacter {
	short depthLevel;					// which dungeon level are we on
	long foodSpawned;					// amount of nutrition units spawned so far this game
	boolean disturbed;					// player should stop auto-acting
	boolean gainedLevel;				// player gained at least one level this turn
	boolean gameHasEnded;				// stop everything and go to death screen
	boolean highScoreSaved;				// so that it saves the high score only once
	boolean blockCombatText;			// busy auto-fighting
	boolean autoPlayingLevel;			// seriously, don't interrupt
	boolean automationActive;			// cut some corners during redraws to speed things up
	boolean justRested;					// previous turn was a rest -- used in stealth
	boolean cautiousMode;				// used to prevent careless deaths caused by holding down a key
	boolean receivedLevitationWarning;	// only warn you once when you're hovering dangerously over liquid
	boolean updatedSafetyMapThisTurn;
	boolean easyMode;					// enables easy mode
	boolean inWater;					// helps with the blue water filter effect
	boolean heardCombatThisTurn;		// so you get only one "you hear combat in the distance" per turn
	boolean creaturesWillFlashThisTurn;		// there are creatures out there that need to flash before the turn ends
	unsigned long seed;					// the master seed for generating the entire dungeon
	long gold;							// how much gold we have
	short experienceLevel;				// what experience level the player is
	unsigned long experience;			// number of experience points
	short maxStrength;
	short currentStrength;
	unsigned short monsterSpawnFuse;	// how much longer till a random monster spawns
	item *weapon;
	item *armor;
	item *ringLeft;
	item *ringRight;
	lightSource *minersLight;
	short minersLightRadius;
	short ticksTillUpdateEnvironment;	// so that some periodic things happen in objective time
	unsigned short scentTurnNumber;		// helps make scent-casting work
	unsigned long turnNumber;
	signed long milliseconds;			// milliseconds since launch, to decide whether to engage cautious mode
	
	short upLoc[2];						// upstairs location this level
	short downLoc[2];					// downstairs location this level
	
	short lastTravelLoc[2];				// used for the return key functionality
	
	short luckyLevels[3];				// guaranteed to have generated at least n good items by the nth lucky level
	short goodItemsGenerated;			// how many generated so far
	
	short **mapToShore;					// how many steps to get back to shore
	
	// ring bonuses:
	short clairvoyance;
	short stealthBonus;
	short regenerationBonus;
	short lightMultiplier;
	short aggravating;
	short awarenessBonus;
	short transference;
	short reflectionBonus;
} playerCharacter;

// Probably need to ditch this crap:
typedef struct levelProfile {
	short caveLevelChance;
	short crossRoomChance;
	short corridorChance;
	short doorChance;
	short maxNumberOfRooms;
	short maxNumberOfLoops;
} levelProfile;

// Stores the necessary info about a level so it can be regenerated:
typedef struct levelData {
	boolean visited;
	short numberOfRooms;
	room *roomStorage;
	pcell mapStorage[DCOLS][DROWS];
	struct item *items;
	struct creature *monsters;
	long levelSeed;
	short upStairsLoc[2];
	short downStairsLoc[2];
	short playerExitedVia[2];
	unsigned long awaySince;
} levelData;

#define NUMBER_LEVEL_PROFILES 1

#define PDS_FORBIDDEN   -1
#define PDS_OBSTRUCTION -2
#define PDS_CELL(map, x, y) ((map)->links + ((x) + DCOLS * (y)))

typedef struct pdsLink pdsLink;
typedef struct pdsMap pdsMap;


#if defined __cplusplus
extern "C" {
#endif
	
	void rogueMain();
	void initializeRogue();
	void gameOver(char *killedBy, boolean useCustomPhrasing);
	void victory();
	void enableEasyMode();
	int rand_range(int lowerBound, int upperBound);
	long seedRandomGenerator(long seed);
	short randClumpedRange(short lowerBound, short upperBound, short clumpFactor);
	short randClump(randomRange theRange);
	boolean rand_percent(short percent);
	void shuffleList(short *list, short listLength);
	short unflag(unsigned long flag);
	void considerCautiousMode();
	void refreshScreen();
	void displayLevel();
	void shuffleTerrainColors(short percentOfCells, boolean refreshCells);
	void getCellAppearance(short x, short y, uchar *returnChar, color *returnForeColor, color *returnBackColor);
	void logLevel();
	void logBuffer(char array[DCOLS][DROWS]);
	//void logBuffer(short **array);
	boolean search(short searchStrength);
	boolean useStairs(short stairDirection);
	void digDungeon();
	boolean buildABridge();
	void updateMapToShore();
	void generateCave();
	boolean levelIsConnectedWithBlockingMap(char blockingMap[DCOLS][DROWS]);
	boolean checkLakePassability(short lakeX, short lakeY);
	void liquidType(short *deep, short *shallow, short *shallowWidth);
	void fillLake(short x, short y, short liquid, short scanWidth, char wreathMap[DCOLS][DROWS]);
	void fillSpawnMap(enum dungeonLayers layer, enum tileType surfaceTileType, char spawnMap[DCOLS][DROWS], boolean refresh);
	void spawnDungeonFeature(short x, short y, dungeonFeature *feat, boolean refreshCell);
	void spawnSurfaceEffect(short x, short y, enum dungeonLayers layer, enum tileType surfaceTileType,
							enum tileType propagationTerrain, boolean requirePropTerrain, short startProbability,
							short probabilityDecrement, char spawnMap[DCOLS][DROWS], boolean refreshCell);
	void restoreMonster(creature *monst, short **mapToStairs, short **mapToPit);
	void restoreItem(item *theItem);
	void cellularAutomata(short minBlobWidth, short minBlobHeight,
						  short maxBlobWidth, short maxBlobHeight, short percentSeeded,
						  char birthParameters[9], char survivalParameters[9]);
	short markBlobCellAndIterate(short xLoc, short yLoc, short blobNumber);
	boolean checkRoom(short roomX, short roomY, short roomWidth, short roomHeight);
	room *attemptRoom(short doorCandidateX, short doorCandidateY, short direction,
					  boolean isCorridor, boolean isCross, short numAttempts);
	void setUpWaypoints();
	void zeroOutGrid(char grid[DCOLS][DROWS]);
	void createWreath(short shallowLiquid, short wreathWidth, char wreathMap[DCOLS][DROWS]);
	short oppositeDirection(short theDir);
	void connectRooms(room *fromRoom, room *toRoom, short x, short y, short direction);
	room *allocateRoom(short roomX, short roomY, short width, short height,
					   short roomX2, short roomY2, short width2, short height2);
	room *roomContainingCell(short x, short y);
	short distanceBetweenRooms(room *fromRoom, room *toRoom);
	
	void carveRectangle(short roomX, short roomY, short roomWidth, short roomHeight);
	void markRectangle(short roomX, short roomY, short roomWidth, short roomHeight);
	void addWallToList(short direction, short xLoc, short yLoc);
	void removeWallFromList(short direction, short xLoc, short yLoc);
	void plotChar(uchar inputChar,
				  short xLoc, short yLoc,
				  short backRed, short backGreen, short backBlue,
				  short foreRed, short foreGreen, short foreBlue);
	void pausingTimerStartsNow();
	boolean pauseForMilliseconds(short milliseconds);
	void nextKeyOrMouseEvent(rogueEvent *returnEvent, boolean colorsDance);
	short getHighScoresList(rogueHighScoresEntry returnList[20]);
	boolean saveHighScore(rogueHighScoresEntry theEntry);
	char nextKeyPress();
	void refreshSideBar(creature *focusMonst);
	void printHelpScreen();
	void printDiscoveriesScreen();
	void printHighScores(boolean hiliteMostRecent);
	void showWaypoints();
	void displaySafetyMap();
	void printSeed();
	short printMonsterInfo(creature *monst, short y, boolean dim);
	void printMonsterDetails(creature *monst, cellDisplayBuffer rbuf[COLS][ROWS]);
	void funkyFade(cellDisplayBuffer displayBuf[COLS][ROWS], color *colorStart, color *colorEnd, short stepCount, short x, short y, boolean invert);
	void waitForAcknowledgment();
	boolean confirm(char *prompt);
	void refreshDungeonCell(short x, short y);
	void applyColorMultiplier(color *baseColor, color *multiplierColor);
	void applyColorAverage(color *baseColor, color *newColor, short augmentWeight);
	void applyColorAugment(color *baseColor, color *augmentingColor, short augmentWeight);
	void desaturate(color *baseColor, short weight);
	void randomizeColor(color *baseColor, short randomizePercent);
	void colorBlendCell(short x, short y, color *hiliteColor, short hiliteStrength);
	void hiliteCell(short x, short y, color *hiliteColor, short hiliteStrength);
	void colorMultiplierFromDungeonLight(short x, short y, color *editColor);
	void plotCharWithColor(uchar inputChar, short xLoc, short yLoc, color cellForeColor, color cellBackColor);
	void plotCharToBuffer(uchar inputChar, short x, short y, color *foreColor, color *backColor, cellDisplayBuffer dbuf[COLS][ROWS]);
	void commitDraws();
	void copyDisplayBuffer(cellDisplayBuffer toBuf[COLS][ROWS], cellDisplayBuffer fromBuf[COLS][ROWS]);
	void clearDisplayBuffer(cellDisplayBuffer dbuf[COLS][ROWS]);
	color colorFromComponents(char rgb[3]);
	void overlayDisplayBuffer(cellDisplayBuffer overBuf[COLS][ROWS], cellDisplayBuffer previousBuf[COLS][ROWS]);
	void flashForeground(short *x, short *y, color **flashColor, short *flashStrength, short count, short frames);
	void flash(color *theColor, short frames, short x, short y);
	void lightFlash(color *theColor, unsigned long reqTerrainFlags, unsigned long reqTileFlags, short frames, short maxRadius, short x, short y);
	void printString(char *theString, short x, short y, color *foreColor, color*backColor, cellDisplayBuffer dbuf[COLS][ROWS]);
	short printStringWithWrapping(char *theString, short x, short y, short width, color *foreColor,
								  color*backColor, cellDisplayBuffer dbuf[COLS][ROWS]);
	boolean getInputTextString(char *inputText, char *prompt, short maxLength);
	boolean pauseBrogue(short milliseconds);
	void nextBrogueEvent(rogueEvent *returnEvent, boolean colorsDance);
	void executeMouseClick(rogueEvent *theEvent);
	void executeKeystroke(unsigned short keystroke, boolean controlKey, boolean shiftKey);
	void initializeLevel(short oldLevelNumber, short stairDirection);
	void startLevel (short oldLevelNumber, short stairDirection);
	void updateMinersLightRadius();
	void emptyGraveyard();
	void freeEverything();
	boolean randomMatchingLocation(short *loc, short dungeonType, short liquidType, short terrainType);
	enum dungeonLayers highestPriorityLayer(short x, short y, boolean skipGas);
	char *tileFlavor(short x, short y);
	char *tileText(short x, short y);
	void describeLocation(char buf[DCOLS], short x, short y);
	void printLocationDescription(short x, short y);
	void playerRuns(short direction);
	void exposeCreatureToFire(creature *monst);
	void updateFlavorText();
	void applyInstantTileEffectsToCreature(creature *monst);
	void vomit(creature *monst);
	void freeCaptive(creature *monst);
	boolean playerMoves(short direction);
	void calculateDistances(short **distanceMap, short destinationX, short destinationY, unsigned long blockingTerrainFlags, creature *traveler);
	short pathingDistance(short x1, short y1, short x2, short y2, unsigned long blockingTerrainFlags);
	short nextStep(short **distanceMap, short x, short y);
	void travel(short x, short y, boolean autoConfirm);
	boolean explore(short frameDelay);
	void examineMode();
	boolean isDisturbed(short x, short y);
	void discover(short x, short y);
	short randValidDirectionFrom(creature *monst, short x, short y, boolean respectAvoidancePreferences);
	boolean exposeTileToFire(short x, short y, boolean alwaysIgnite);
	boolean cellCanHoldGas(short x, short y);
	void updateEnvironment();
	void updateSafetyMap();
	void extinguishFireOnCreature(creature *monst);
	void autoRest();
	void startFighting(enum directions dir, boolean tillDeath);
	void autoFight(boolean tillDeath);
	void playerTurnEnded();
	void resetTurnNumber();
	void displayMonsterFlashes(boolean flashingEnabled);
	void temporaryMessage(char *msg1, boolean requireAcknowledgment);
	void message(char *message, boolean primaryMessage, boolean requireAcknowledgment);
	void displayMoreSign();
	void upperCase(char *theChar);
	void updateMessageDisplay();
	void deleteMessages();
	void confirmMessages();
	void stripShiftFromMovementKeystroke(unsigned short *keystroke);
	
	void updateFieldOfViewDisplay(boolean updateDancingTerrain);
	void updateFieldOfView(short xLoc, short yLoc, short radius, boolean paintScent,
						   boolean passThroughCreatures, boolean setFieldOfView, short theColor[3], short fadeToPercent);
	void betweenOctant1andN(short *x, short *y, short x0, short y0, short n);
	
	void getFOVMask(boolean grid[DCOLS][DROWS], short xLoc, short yLoc, short maxRadius,
					unsigned long forbiddenTerrain,	unsigned long forbiddenFlags, boolean cautiousOnWalls);
	void scanOctantFOV(boolean grid[DCOLS][DROWS], short xLoc, short yLoc, short octant, short maxRadius,
					   short columnsRightFromOrigin, long startSlope, long endSlope, unsigned long forbiddenTerrain,
					   unsigned long forbiddenFlags, boolean cautiousOnWalls);
	
	creature *generateMonster(short monsterID, boolean itemPossible);
	short chooseMonster(short forLevel);
	creature *spawnHorde(short hordeID, short x, short y, long forbiddenFlags);
	void fadeInMonster(creature *monst);
	boolean monstersAreTeammates(creature *monst1, creature *monst2);
	boolean monstersAreEnemies(creature *monst1, creature *monst2);
	short pickHordeType(enum monsterTypes summonerType, long forbiddenFlags, short depth);
	creature *cloneMonster(creature *monst);
	short **allocDynamicGrid();
	void freeDynamicGrid(short **array);
	void copyDynamicGrid(short **to, short **from);
	void populateMonsters();
	void updateMonsterState(creature *monst);
	void decrementMonsterStatus(creature *monst);
	boolean specifiedPathBetween(short x1, short y1, short x2, short y2,
								 unsigned long blockingTerrain, unsigned long blockingFlags);
	boolean openPathBetween(short x1, short y1, short x2, short y2);
	creature *monsterAtLoc(short x, short y);
	void unAlly(creature *monst);
	void monstersTurn(creature *monst);
	void spawnPeriodicHorde();
	void clearStatus(creature *monst);
	void monsterShoots(creature *attacker, short targetLoc[2], uchar projChar, color *projColor);
	void shootWeb(creature *breather, short targetLoc[2], short kindOfWeb);
	void magicWeaponHit(creature *defender, item *theItem, boolean backstabbed);
	void teleport(creature *monst);
	void chooseNewWanderDestination(creature *monst);
	boolean isPassableOrSecretDoor(short x, short y);
	boolean moveMonster(creature *monst, short dx, short dy);
	boolean monsterAvoids(creature *monst, short x, short y);
	short distanceBetween(short x1, short y1, short x2, short y2);
	void wakeUp(creature *monst);
	boolean canSeeMonster(creature *monst);
	void monsterName(char *buf, creature *monst, boolean includeArticle);
	short hitProbability(creature *attacker, creature *defender);
	boolean attackHit(creature *attacker, creature *defender);
	void applyArmorRunicEffect(char returnString[DCOLS], creature *attacker, short *damage, boolean melee);
	boolean attack(creature *attacker, creature *defender);
	boolean inflictDamage(creature *attacker, creature *defender, short damage, color *flashColor);
	void killCreature(creature *decedent);
	void addExperience(unsigned long exp);
	void addScentToCell(short x, short y, short distance);
	void populateItems(short upstairsX, short upstairsY);
	item *placeItem(item *theItem, short x, short y);
	void pickUpItemAt(short x, short y);
	item *addItemToPack(item *theItem);
	short getLineCoordinates(short listOfCoordinates[][2], short originLoc[2], short targetLoc[2]);
	void getImpactLoc(short returnLoc[2], short originLoc[2], short targetLoc[2],
					  short maxDistance, boolean returnLastEmptySpace);
	void cancelMagic(creature *monst);
	void slow(creature *monst, short turns);
	void haste(creature *monst, short turns);
	void heal(creature *monst, short percent);
	boolean projectileReflects(creature *attacker, creature *defender);
	short reflectBolt(short targetX, short targetY, short listOfCoordinates[][2], short kinkCell, boolean retracePath);
	boolean zap(short originLoc[2], short targetLoc[2], enum boltType bolt, short boltLevel, boolean hideDetails);
	creature *nextTargetAfter(short targetX, short targetY, boolean targetAllies, boolean requireOpenPath);
	void moveCursor(boolean *targetConfirmed, boolean *canceled, boolean *tabKey, short targetLoc[2]);
	short numberOfItemsInPack();
	char nextAvailableInventoryCharacter();
	void itemName(item *theItem, char *buf, boolean includeSuffix, boolean includeArticle);
	void updateItemKnownMagicStatus(item *theItem);
	char displayInventory(unsigned short categoryMask,
						  unsigned long requiredFlags,
						  unsigned long forbiddenFlags,
						  boolean waitForAcknowledge);
	short numberOfMatchingPackItems(unsigned short categoryMask,
									unsigned long requiredFlags, unsigned long forbiddenFlags,
									boolean displayErrors);
	void clearInventory(char keystroke);
	item *generateItem(short theCategory, short theKind);
	short chooseKind(itemTable *theTable, short numKinds);
	item *makeItemInto(item *theItem, short itemCategory, short itemKind);
	void strengthCheck(item *theItem);
	boolean equipItem(item *theItem, boolean force);
	void equip();
	void unequip();
	void drop();
	boolean getQualifyingLocNear(short loc[2], short x, short y, short maxRadius,
								 unsigned long forbiddenTerrainFlags, unsigned long forbiddenMapFlags, boolean forbidLiquid);
	void demoteMonsterFromLeadership(creature *monst);
	void makeMonsterDropItem(creature *monst);
	void throwCommand();
	void apply(item *theItem);
	void call();
	enum monsterTypes chooseVorpalEnemy();
	void identify(item *theItem);
	void readScroll(item *theItem);
	void updateRingBonuses();
	void updatePlayerRegenerationDelay();
	void drinkPotion(item *theItem);
	item *promptForItemOfType(unsigned short category,
							  unsigned long requiredFlags,
							  unsigned long forbiddenFlags,
							  char *prompt);
	item *itemOfPackLetter(char letter);
	boolean unequipItem(item *theItem, boolean force);
	uchar itemMagicChar(item *theItem);
	item *itemAtLoc(short x, short y);
	item *dropItem(item *theItem);
	boolean isVowel(char theChar);
	void deleteItem(item *theItem);
	void shuffleFlavors();
	void combatMessage(char *theMsg);
	void displayCombatText();
	void flashMonster(creature *monst, color *theColor, short strength);
	
	lightSource *newLight(lightSource *modelLight, short x, short y, creature *followsCreature);
	void deleteLight(lightSource *theLight);
	void paintLight(lightSource *theLight);
	void updateLighting();
	void demoteVisibility();
	void updateVision();
	void burnItem(item *theItem);
	void promoteTile(short x, short y, enum dungeonLayers layer, boolean useFireDF);
	void autoPlayLevel(boolean fastForward);
	void updateClairvoyance();
	
	void checkForDungeonErrors();
	
	void dijkstraScan(short **distanceMap, short **costMap, char passMap[DCOLS][DROWS], boolean useDiagonals);
	void pdsClear(pdsMap *map, short maxDistance, boolean eightWays);
	void pdsSetDistance(pdsMap *map, short x, short y, short distance);
	void pdsBatchOutput(pdsMap *map, short **distanceMap);
	
#if defined __cplusplus
}
#endif
