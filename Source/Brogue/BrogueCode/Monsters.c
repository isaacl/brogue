/*
 *  Monsters.c
 *  Brogue
 *
 *  Created by Brian Walker on 1/13/09.
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

#include <math.h>
#include "Rogue.h"
#include "IncludeGlobals.h"

// Allocates space, generates a creature of the given type,
// prepends it to the list of creatures, and returns a pointer to that creature. Note that the creature
// is not given a map location here!
creature *generateMonster(short monsterID, boolean itemPossible) {
	short itemChance;
	creature *monst;
	
	monst = (creature *) malloc(sizeof(creature));
	memset(monst, '\0', sizeof(creature) );
	clearStatus(monst);
	monst->info = monsterCatalog[monsterID];
	monst->nextCreature = monsters->nextCreature;
	monsters->nextCreature = monst;
	monst->xLoc = monst->yLoc = 0;
	monst->bookkeepingFlags = 0;
	monst->mapToMe = NULL;
	monst->safetyMap = NULL;
	monst->leader = NULL;
	monst->creatureState = ((rand_percent(25) || monst->info.flags & MONST_NEVER_SLEEPS || rogue.aggravating)
							? MONSTER_TRACKING_SCENT : MONSTER_SLEEPING);
	monst->creatureMode = MODE_NORMAL;
	monst->currentHP = monst->info.maxHP;
	monst->spawnDepth = rogue.depthLevel;
	monst->statusLight = NULL;
	monst->ticksUntilTurn = monst->info.movementSpeed;
	monst->info.turnsBetweenRegen *= 1000; // tracked as thousandths to prevent rounding errors
	monst->turnsUntilRegen = monst->info.turnsBetweenRegen;
	monst->regenPerTurn = 0;
	monst->movementSpeed = monst->info.movementSpeed;
	monst->attackSpeed = monst->info.attackSpeed;
	monst->turnsSpentStationary = 0;
	monst->xpxp = 0;
	monst->absorbsAllowed = 1500;
	monst->targetCorpseLoc[0] = monst->targetCorpseLoc[1] = 0;
	monst->destination[0][0] = monst->destination[0][1] = monst->destination[1][0] = monst->destination[1][1] = 0;
	monst->comingFrom[0] = monst->comingFrom[1] = 0;
	if (monst->info.flags & MONST_FIERY) {
		monst->status.burning = monst->maxStatus.burning = 1000; // won't decrease
	}
	if (monst->info.flags & MONST_FLIES) {
		monst->status.levitating = monst->maxStatus.levitating = 1000; // won't decrease
	}
	if (monst->info.flags & MONST_IMMUNE_TO_FIRE) {
		monst->status.immuneToFire = monst->maxStatus.immuneToFire = 1000; // won't decrease
	}
	monst->status.nutrition = monst->maxStatus.nutrition = 1000;
	
	if (monst->info.flags & MONST_INTRINSIC_LIGHT) {
		monst->intrinsicLight = newLight(&lightCatalog[monst->info.intrinsicLightType], 0, 0, monst);
	} else {
		monst->intrinsicLight = NULL;
	}
	
	if (monst->info.flags & MONST_CARRY_ITEM_100) {
		itemChance = 100;
	} else if (monst->info.flags & MONST_CARRY_ITEM_25) {
		itemChance = 25;
	} else {
		itemChance = 0;
	}
	
	monst->carriedItem = ((ITEMS_ENABLED && itemPossible && (rogue.depthLevel <= AMULET_LEVEL) && rand_percent(itemChance))
						  ? generateItem(-1, -1) : NULL);
	
	return monst;
}

boolean canSeeMonster(creature *monst) {
	if (monst == &player) {
		return true;
	}
	if (playerCanSee(monst->xLoc, monst->yLoc)
		&& !(monst->info.flags & MONST_INVISIBLE || monst->bookkeepingFlags & MONST_SUBMERGED)) {
		return true;
	}
	return false;
}

void monsterName(char *buf, creature *monst, boolean includeArticle) {
	short oldRNG;
	
	if (monst == &player) {
		strcpy(buf, "you");
		return;
	}
	if (canSeeMonster(monst) || rogue.playbackOmniscience) {
		if (player.status.hallucinating && !rogue.playbackOmniscience) {
			oldRNG = rogue.RNG;
			rogue.RNG = RNG_COSMETIC;
			sprintf(buf, "%s%s", (includeArticle ? "the " : ""),
					monsterCatalog[rand_range(1, NUMBER_MONSTER_KINDS - 1)].monsterName);
			rogue.RNG = oldRNG;
			return;
		}
		sprintf(buf, "%s%s", (includeArticle ? (monst->creatureState == MONSTER_ALLY ? "your " : "the ") : ""),
				monst->info.monsterName);
		//monsterText[monst->info.monsterID].name);
		return;
	} else {
		strcpy(buf, "something");
		return;
	}
}

boolean monstersAreTeammates(creature *monst1, creature *monst2) {
	return ((((monst1->bookkeepingFlags & MONST_FOLLOWER) && monst1->leader == monst2)
			 || ((monst2->bookkeepingFlags & MONST_FOLLOWER) && monst2->leader == monst1)
			 || ((monst1->bookkeepingFlags & MONST_FOLLOWER) && (monst2->bookkeepingFlags & MONST_FOLLOWER)
				 && monst1->leader == monst2->leader)) ? true : false);
}

boolean monstersAreEnemies(creature *monst1, creature *monst2) {
	if ((monst1->bookkeepingFlags | monst2->bookkeepingFlags) & MONST_CAPTIVE) {
		return false;
	}
	if (monst1->status.discordant || monst2->status.discordant) {
		return true;
	}
	// eels and krakens attack anything in deep water
	if (((monst1->info.flags & MONST_RESTRICTED_TO_LIQUID)
		 && !(monst2->info.flags & MONST_IMMUNE_TO_WATER)
		 && !(monst2->status.levitating)
		 && cellHasTerrainFlag(monst2->xLoc, monst2->yLoc, IS_DEEP_WATER))
		
		|| ((monst2->info.flags & MONST_RESTRICTED_TO_LIQUID)
			&& !(monst1->info.flags & MONST_IMMUNE_TO_WATER)
			&& !(monst1->status.levitating)
			&& cellHasTerrainFlag(monst1->xLoc, monst1->yLoc, IS_DEEP_WATER))) {
			
			return true;
		}
	return ((monst1->creatureState == MONSTER_ALLY || monst1 == &player)
			!= (monst2->creatureState == MONSTER_ALLY || monst2 == &player));
}

short pickHordeType(enum monsterTypes summonerType, long forbiddenFlags, short depth) { // pass 0 for summonerType for an ordinary selection
	short i, index, possCount = 0;
	
	if (depth <= 0) {
		depth = rogue.depthLevel;
	}
	
	for (i=0; i<NUMBER_HORDES; i++) {
		if (!(hordeCatalog[i].flags & forbiddenFlags)
			&& ((!summonerType && hordeCatalog[i].minLevel <= depth && hordeCatalog[i].maxLevel >= depth)
				|| (summonerType && hordeCatalog[i].flags & HORDE_IS_SUMMONED && hordeCatalog[i].leaderType == summonerType))) {
				possCount += hordeCatalog[i].frequency;
			}
	}
	
	if (possCount == 0) {
		return -1;
	}
	
	index = rand_range(1, possCount);
	
	for (i=0; i<NUMBER_HORDES; i++) {
		if (!(hordeCatalog[i].flags & forbiddenFlags)
			&& ((!summonerType && hordeCatalog[i].minLevel <= depth && hordeCatalog[i].maxLevel >= depth)
				|| (summonerType && hordeCatalog[i].flags & HORDE_IS_SUMMONED && hordeCatalog[i].leaderType == summonerType))) {
				if (index <= hordeCatalog[i].frequency) {
					return i;
				}
				index -= hordeCatalog[i].frequency;
			}
	}
	return 0; // should never happen
}

creature *cloneMonster(creature *monst) {
	creature *newMonst, *nextMonst;
	short loc[2];
	char buf[DCOLS], monstName[DCOLS];
	
	newMonst = generateMonster(monst->info.monsterID, false);
	nextMonst = newMonst->nextCreature;
	*newMonst = *monst; // boink!
	newMonst->nextCreature = nextMonst;
	
	if (newMonst->statusLight) {
		newMonst->statusLight = newLight(newMonst->statusLight, newMonst->xLoc, newMonst->yLoc, newMonst);
	}
	if (newMonst->intrinsicLight) {
		newMonst->intrinsicLight = newLight(newMonst->intrinsicLight, newMonst->xLoc, newMonst->yLoc, newMonst);
	}
	
	newMonst->info.expForKilling = 0;
	newMonst->bookkeepingFlags &= ~(MONST_LEADER | MONST_CAPTIVE);
	newMonst->bookkeepingFlags |= MONST_FOLLOWER;
	newMonst->mapToMe = NULL;
	newMonst->safetyMap = NULL;
	newMonst->carriedItem = NULL;
	newMonst->ticksUntilTurn = newMonst->attackSpeed;
	if (monst->leader) {
		newMonst->leader = monst->leader;
	} else {
		newMonst->leader = monst;
		monst->bookkeepingFlags |= MONST_LEADER;
	}
	getQualifyingLocNear(loc, monst->xLoc, monst->yLoc, DCOLS, PATHING_BLOCKER, (HAS_PLAYER | HAS_MONSTER), false);
	newMonst->xLoc = loc[0];
	newMonst->yLoc = loc[1];
	pmap[loc[0]][loc[1]].flags |= HAS_MONSTER;
	refreshDungeonCell(loc[0], loc[1]);
	if (canSeeMonster(newMonst)) {
		monsterName(monstName, newMonst, false);
		sprintf(buf, "another %s appears!", monstName);
		message(buf, true, false);
	}
	
	if (monst == &player) { // player managed to clone himself
		newMonst->info.foreColor = &gray;
		newMonst->info.damage.lowerBound = 1;
		newMonst->info.damage.upperBound = 2;
		newMonst->info.damage.clumpFactor = 1;
		newMonst->info.defense = 0;
		strcpy(newMonst->info.monsterName, "clone");
		newMonst->creatureState = MONSTER_ALLY;
	}
	return newMonst;
}

// mallocing two-dimensional arrays! dun dun DUN!
short **allocDynamicGrid() {
	short i;
	short **array = malloc(DCOLS * sizeof(short *));
	array[0] = malloc(DROWS * DCOLS * sizeof(short));
	for(i = 1; i < DCOLS; i++) {
		array[i] = array[0] + i * DROWS;
	}
	return array;
}

void freeDynamicGrid(short **array) {
	free(array[0]);
	free(array);
}

void copyDynamicGrid(short **to, short **from) {
	short i, j;
	
	for(i = 0; i < DCOLS; i++) {
		for(j = 0; j < DROWS; j++) {
			to[i][j] = from[i][j];
		}
	}
}

// returns true if at least one minion spawned
boolean spawnMinions(short hordeID, creature *leader) {
	short loc[2];
	short iSpecies, iMember, count;
	unsigned long forbiddenFlags;
	hordeType *theHorde;
	creature *monst;
	short x, y;
	short failsafe;
	boolean atLeastOneMinion = false;
	
	x = leader->xLoc;
	y = leader->yLoc;
	
	theHorde = &hordeCatalog[hordeID];
	
	for (iSpecies = 0; iSpecies < theHorde->numberOfMemberTypes; iSpecies++) {
		count = randClump(theHorde->memberCount[iSpecies]);
		for (iMember = 0; iMember < count; iMember++) {
			monst = generateMonster(theHorde->memberType[iSpecies], true);
			forbiddenFlags = PATHING_BLOCKER;
			if (monst->info.flags & (MONST_IMMUNE_TO_FIRE | MONST_FLIES)) {
				forbiddenFlags &= ~LAVA_INSTA_DEATH;
			}
			if (monst->info.flags & (MONST_IMMUNE_TO_WATER | MONST_FLIES)) {
				forbiddenFlags &= ~IS_DEEP_WATER;
			}
			if (monst->info.flags & (MONST_FLIES)) {
				forbiddenFlags &= ~TRAP_DESCENT;
			}
			if (hordeCatalog[hordeID].spawnsIn) {
				forbiddenFlags &= ~(tileCatalog[hordeCatalog[hordeID].spawnsIn].flags);
			}
			failsafe = 0;
			do {
				getQualifyingLocNear(loc, x, y, DCOLS, forbiddenFlags, (HAS_PLAYER | HAS_MONSTER), false);
			} while (theHorde->spawnsIn && !cellHasTerrainType(theHorde->spawnsIn, loc[0], loc[1]) && failsafe++ < 20);
			if (failsafe >= 20) {
				// abort
				killCreature(monst);
				break;
			}
			monst->xLoc = loc[0];
			monst->yLoc = loc[1];
			if (monst->info.flags & MONST_SUBMERGES && cellHasTerrainFlag(x, y, ALLOWS_SUBMERGING)) {
				monst->bookkeepingFlags |= MONST_SUBMERGED;
			}
			pmap[loc[0]][loc[1]].flags |= HAS_MONSTER;
			monst->bookkeepingFlags |= (MONST_FOLLOWER | MONST_JUST_SUMMONED);
			monst->leader = leader;
			monst->creatureState = leader->creatureState;
			chooseNewWanderDestination(monst);
			monst->mapToMe = NULL;
			if (theHorde->flags & HORDE_DIES_ON_LEADER_DEATH) {
				monst->bookkeepingFlags |= MONST_BOUND_TO_LEADER;
			}
			atLeastOneMinion = true;
		}
	}
	
	if (atLeastOneMinion) {
		leader->bookkeepingFlags |= MONST_LEADER;
	}
	
	return atLeastOneMinion;
}

boolean drawManacle(short x, short y, enum directions dir) {
	enum tileType manacles[8] = {MANACLE_T, MANACLE_B, MANACLE_L, MANACLE_R, MANACLE_TL, MANACLE_BL, MANACLE_TR, MANACLE_BR};
	short newX = x + nbDirs[dir][0];
	short newY = y + nbDirs[dir][1];
	if (pmap[newX][newY].layers[DUNGEON] == FLOOR && pmap[newX][newY].layers[LIQUID] == NOTHING) {
		pmap[x + nbDirs[dir][0]][y + nbDirs[dir][1]].layers[SURFACE] = manacles[dir];
		return true;
	}
	return false;
}

void drawManacles(short x, short y) {
	enum directions fallback[4][3] = {{UPLEFT, UP, LEFT}, {DOWNLEFT, DOWN, LEFT}, {UPRIGHT, UP, RIGHT}, {DOWNRIGHT, DOWN, RIGHT}};
	short i, j;
	for (i = 0; i < 4; i++) {
		for (j = 0; j < 3 && !drawManacle(x, y, fallback[i][j]); j++);
	}
}

// if hordeID is 0, it's randomly assigned based on the depth. If x is 0, location is random.
// returns a pointer to the leader.
creature *spawnHorde(short hordeID, short x, short y, long forbiddenFlags) {
	short loc[2];
	short i, failsafe;
	hordeType *theHorde;
	creature *leader;
	boolean atLeastOneMinion = false;
	
	if (hordeID <= 0) {
		do {
			hordeID = pickHordeType(0, forbiddenFlags, 0);
			if (hordeID < 0) {
				return NULL;
			}
		} while (x > 0 && y > 0 && hordeCatalog[hordeID].spawnsIn && !cellHasTerrainType(x, y, hordeCatalog[hordeID].spawnsIn));
	}
	
	failsafe = 50;
	
	if (x <= 0 || y <= 0) {
		for (i = 0; i < 25 || (pmap[x][y].flags & VISIBLE); i++) {
			while (!randomMatchingLocation(loc, FLOOR, NOTHING, (hordeCatalog[hordeID].spawnsIn ? hordeCatalog[hordeID].spawnsIn : -1))) {
				if (!--failsafe) {
					return NULL;
				}
				hordeID = pickHordeType(0, forbiddenFlags, 0);
				
				if (hordeID < 0) {
					return NULL;
				}
			}
			x = loc[0];
			y = loc[1];
		}
	}
	
	theHorde = &hordeCatalog[hordeID];
	leader = generateMonster(theHorde->leaderType, true);
	leader->xLoc = x;
	leader->yLoc = y;
	chooseNewWanderDestination(leader);
	
	if (hordeCatalog[hordeID].flags & HORDE_LEADER_CAPTIVE) {
		leader->bookkeepingFlags |= MONST_CAPTIVE;
		leader->creatureState = MONSTER_WANDERING;
		leader->info.expForKilling = 0;
		leader->currentHP = leader->info.maxHP / 4 + 1;
		if (leader->carriedItem) {
			makeMonsterDropItem(leader);
		}
		
		// draw the manacles
		drawManacles(x, y);
	}
	
	pmap[x][y].flags |= HAS_MONSTER;
	if (leader->info.flags & MONST_SUBMERGES && cellHasTerrainFlag(x, y, ALLOWS_SUBMERGING)) {
		leader->bookkeepingFlags |= MONST_SUBMERGED;
	}
	
	atLeastOneMinion = spawnMinions(hordeID, leader);
	
	return leader;
}

void fadeInMonster(creature *monst) {
	color fColor, bColor;
	uchar displayChar;
	getCellAppearance(monst->xLoc, monst->yLoc, &displayChar, &fColor, &bColor);
	flashMonster(monst, &bColor, 100);
}

boolean summonMinions(creature *summoner) {
	enum monsterTypes summonerType = summoner->info.monsterID;
	short hordeID = pickHordeType(summonerType, 0, 0), seenMinionCount = 0;
	boolean atLeastOneMinion = false;
	creature *monst, *minion;
	char buf[DCOLS];
	char monstName[DCOLS];
	
	if (hordeID < 0) {
		return false;
	}
	
	atLeastOneMinion = spawnMinions(hordeID, summoner);
	
	for (monst = monsters->nextCreature; monst != NULL; monst = monst->nextCreature) {
		if (monst != summoner && monstersAreTeammates(monst, summoner)
			&& (monst->bookkeepingFlags & MONST_JUST_SUMMONED)) {
			monst->bookkeepingFlags &= ~MONST_JUST_SUMMONED;
			if (canSeeMonster(monst)) {
				seenMinionCount++;
				minion = monst;
				refreshDungeonCell(monst->xLoc, monst->yLoc);
			}
			monst->info.expForKilling = 0; // no farming summoners please
			monst->ticksUntilTurn = max(monst->info.attackSpeed, monst->info.movementSpeed);
			monst->leader = (summoner->bookkeepingFlags & MONST_FOLLOWER ? summoner->leader : summoner);
			if (monst->carriedItem) {
				deleteItem(monst->carriedItem);
				monst->carriedItem = NULL;
			}
			fadeInMonster(monst);
		}
	}
	
	if (canSeeMonster(summoner)) {
		monsterName(monstName, summoner, true);
		if (monsterText[summoner->info.monsterID].summonMessage) {
			sprintf(buf, "%s %s", monstName, monsterText[summoner->info.monsterID].summonMessage);
		} else {
			sprintf(buf, "%s incants darkly!", monstName);
		}
		message(buf, true, false);
	}
	
	return atLeastOneMinion;
}

// Generates and places monsters for the level.
void populateMonsters() {
	if (!MONSTERS_ENABLED) {
		return;
	}
	
	short i, numberOfMonsters = min(20, 6 + 3 * max(0, rogue.depthLevel - AMULET_LEVEL)); // almost always 6.
	while (rand_percent(60)) {
		numberOfMonsters++;
	}
	for (i=0; i<numberOfMonsters; i++) {
		spawnHorde(0, 0, 0, HORDE_IS_SUMMONED); // random horde type, random location
	}
}

void spawnPeriodicHorde() {
	if (!MONSTERS_ENABLED) {
		return;
	}
	
	creature *monst;
	short loc[2], failsafe = 0;
	do {
		failsafe++;
		if (failsafe > 20) {
			return;
		}
		randomMatchingLocation(loc, FLOOR, NOTHING, -1);
	} while ((pmap[loc[0]][loc[1]].flags & VISIBLE) ||
			 distanceBetween(player.xLoc, player.yLoc, loc[0], loc[1]) < 15);
	monst = spawnHorde(0, loc[0], loc[1], (HORDE_IS_SUMMONED | HORDE_LEADER_CAPTIVE | NO_PERIODIC_SPAWN));
	if (monst) {
		wakeUp(monst);
	}
}

void teleport(creature *monst) {
	short loc[2];
	
	randomMatchingLocation(loc, FLOOR, NOTHING, -1);
	
	if (monst == &player) {
		pmap[player.xLoc][player.yLoc].flags &= ~HAS_PLAYER;
		refreshDungeonCell(player.xLoc, player.yLoc);
		monst->xLoc = loc[0];
		monst->yLoc = loc[1];
		pmap[player.xLoc][player.yLoc].flags |= HAS_PLAYER;
		updateVision();
		// get any items at the destination location
		if (pmap[player.xLoc][player.yLoc].flags & HAS_ITEM) {
			pickUpItemAt(player.xLoc, player.yLoc);
		}
	} else {
		pmap[monst->xLoc][monst->yLoc].flags &= ~HAS_MONSTER;
		refreshDungeonCell(monst->xLoc, monst->yLoc);
		monst->xLoc = loc[0];
		monst->yLoc = loc[1];
		pmap[monst->xLoc][monst->yLoc].flags |= HAS_MONSTER;
		chooseNewWanderDestination(monst);
	}
	refreshDungeonCell(monst->xLoc, monst->yLoc);
}

void chooseNewWanderDestination(creature *monst) {
	short i, j, k, x, y, randIndex, monsterDistance[2], playerDistance[2];
	short monsterAdvantage, bestMonsterAdvantage = 0;
	short WPlist[50], WPcoords[50][2], WPcount, currentWP;
	char grid[DCOLS][DROWS];
	unsigned long monsterBlockers;
	
	if (monst->bookkeepingFlags & MONST_FOLLOWER && monst->leader && monst->leader->bookkeepingFlags & MONST_CAPTIVE) {
		monst->destination[0][0] = monst->leader->xLoc;
		monst->destination[0][1] = monst->leader->yLoc;
		return;
	}
	
	// list the available waypoint options
	
	// first add in all the WP's in line of sight
	monsterBlockers = WAYPOINT_BLOCKER;
	if (monst->status.levitating) {
		monsterBlockers &= ~(LAVA_INSTA_DEATH | IS_DEEP_WATER | TRAP_DESCENT | IS_DF_TRAP);
	}
	if (monst->status.immuneToFire) {
		monsterBlockers &= ~LAVA_INSTA_DEATH;
	}
	if (monst->info.flags & MONST_IMMUNE_TO_WATER) {
		monsterBlockers &= ~IS_DEEP_WATER;
	}
	
	zeroOutGrid(grid);
	
	getFOVMask(grid, monst->xLoc, monst->yLoc, WAYPOINT_SIGHT_RADIUS, monsterBlockers, DOORWAY, false);
	
	WPcount = 0;
	currentWP = -1;
	
	if (!(monst->info.flags & MONST_RESTRICTED_TO_LIQUID)) {
		
		for (i=0; i<numberOfWaypoints; i++) {
			if (monst->xLoc == waypoints[i].x && monst->yLoc == waypoints[i].y) {
				currentWP = i; // don't add the current WP to the list, but remember which it is.
			} else if (grid[waypoints[i].x][waypoints[i].y]
					   && !cellHasTerrainFlag(waypoints[i].x, waypoints[i].y, monsterBlockers)) {
				WPlist[WPcount] = i;
				WPcoords[WPcount][0] = waypoints[i].x;
				WPcoords[WPcount][1] = waypoints[i].y;
				WPcount++;
			}
		}
		
		// now, if we're starting on a WP, supplement with WP's marked as siblings
		if (currentWP > -1) {
			for (i=0; i < waypoints[currentWP].connectionCount; i++) {
				x = waypoints[currentWP].connection[i][0];
				y = waypoints[currentWP].connection[i][1];
				if (!(grid[x][y]) && !cellHasTerrainFlag(x, y, monsterBlockers)) {
					WPcoords[WPcount][0] = x;
					WPcoords[WPcount][1] = y;
					WPcount++;
				}
			}
		}
		
	}
	
	// listing complete.
	
	// if no waypoints are visible, choose a random spot that the monster can see and send it there.
	if ((monst->info.flags & MONST_RESTRICTED_TO_LIQUID)
		|| (WPcount == 0 && (monst->creatureState != MONSTER_FLEEING || !(pmap[monst->xLoc][monst->yLoc].flags & IN_FIELD_OF_VIEW)))) {
		
		if (currentWP > -1 && waypoints[currentWP].pointsTo[0] && waypoints[currentWP].pointsTo[1]) {
			monst->destination[0][0] = waypoints[currentWP].pointsTo[0];
			monst->destination[0][1] = waypoints[currentWP].pointsTo[1];
			return;
		}
		
		// We should get here only in unusual circumstances.
		
		monst->comingFrom[0] = monst->xLoc;
		monst->comingFrom[1] = monst->yLoc;
		
		for (i=0; i<DCOLS; i++) {
			for (j=0; j<DROWS; j++) {
				if (grid[i][j] && (monst->xLoc != i ||  monst->yLoc != j) && !cellHasTerrainFlag(i, j, monsterBlockers)
					&& (!(monst->info.flags & MONST_RESTRICTED_TO_LIQUID) || cellHasTerrainFlag(i, j, ALLOWS_SUBMERGING))) {
					WPcount++;
				}
			}
		}
		if (WPcount == 0) {
			return; // jeez, what is going on? Maybe trapped by a staff of obstruction? A teleported eel?
		}
		
		randIndex = rand_range(1, WPcount);
		
		for (i=0; i<DCOLS; i++) {
			for (j=0; j<DROWS; j++) {
				if (grid[i][j] && (monst->xLoc != i ||  monst->yLoc != j) && !cellHasTerrainFlag(i, j, monsterBlockers)
					&& (!(monst->info.flags & MONST_RESTRICTED_TO_LIQUID) || cellHasTerrainFlag(i, j, ALLOWS_SUBMERGING))
					&& !--randIndex) {
					monst->destination[0][0] = i;
					monst->destination[0][1] = j;
					
					return;
				}
			}
		}
		return; // just in case
	}
	
	if (monst->creatureState == MONSTER_WANDERING || !(pmap[monst->xLoc][monst->yLoc].flags & IN_FIELD_OF_VIEW)) {
		// if wandering, pick a random WP that the monster can see.
		for (i=0; i < 5; i++) {
			randIndex = rand_range(0, WPcount - 1);
			for (k=0; k<10 && monsterAvoids(monst, WPcoords[randIndex][0], WPcoords[randIndex][1]); k++) {
				randIndex = rand_range(0, WPcount - 1);
			}
			if (WPcoords[randIndex][0] != monst->comingFrom[0] || WPcoords[randIndex][1] != monst->comingFrom[1]) {
				break;
			}
		}
	} else {
		// monster is fleeing and the player is in sight;
		// for each door, find out the distance to it from the monster and the player,
		// and pick one for which the monster has the greatest advantage
		randIndex = 0; // just in case no other options are available
		bestMonsterAdvantage = -1000;
		for (i=0; i<WPcount; i++) {
			if (!monsterAvoids(monst, WPcoords[i][0], WPcoords[i][1])) {
				monsterDistance[0] = max(monst->xLoc, WPcoords[i][0]) - min(monst->xLoc, WPcoords[i][0]);
				monsterDistance[1] = max(monst->yLoc, WPcoords[i][1]) - min(monst->yLoc, WPcoords[i][1]);
				playerDistance[0] = max(player.xLoc, WPcoords[i][0]) - min(player.xLoc, WPcoords[i][0]);
				playerDistance[1] = max(player.yLoc, WPcoords[i][1]) - min(player.yLoc, WPcoords[i][1]);
				monsterAdvantage = (max(playerDistance[0], playerDistance[1]) - max(monsterDistance[0], monsterDistance[1]));
				if (monsterAdvantage + rand_range(0, 1) > bestMonsterAdvantage) {
					bestMonsterAdvantage = monsterAdvantage;
					randIndex = i;
				}
			}
		}
	}
	
	monst->destination[0][0] = WPcoords[randIndex][0];
	monst->destination[0][1] = WPcoords[randIndex][1];
	
	for (i=0; i<numberOfWaypoints; i++) {
		if (waypoints[i].x == WPcoords[randIndex][0] && waypoints[i].y == WPcoords[randIndex][1]) {
			if (waypoints[i].pointsTo[0] > 0) {
				monst->destination[1][0] = waypoints[i].pointsTo[0];
				monst->destination[1][1] = waypoints[i].pointsTo[1];
			}
			break;
		}
	}
	
	monst->comingFrom[0] = monst->xLoc;
	monst->comingFrom[1] = monst->yLoc;
	
	// if the monster is already standing on waypoint #1, bump down waypoint #2.
	if (monst->xLoc == monst->destination[0][0] && monst->yLoc == monst->destination[0][1]) {
		monst->destination[0][0] = monst->destination[1][0];
		monst->destination[0][1] = monst->destination[1][1];
		monst->destination[1][0] = monst->destination[1][1] = 0;
	}
}

void getRoomCenter(short returnLoc[2], room *theRoom) {
	if (!theRoom) {
		return;
	}
	returnLoc[0] = theRoom->roomX + (theRoom->width - 1) / 2;
	returnLoc[1] = theRoom->roomY + (theRoom->height - 1) / 2;
}

boolean monsterAvoids(creature *monst, short x, short y) {
	item *theItem;
	creature *defender;
	
	// scrolls of sanctuary
	if (pmap[x][y].flags & HAS_ITEM && monst != &player) {
		theItem = itemAtLoc(x, y);
		if (theItem && theItem->category & SCROLL && theItem->kind == SCROLL_SANCTUARY) {
			return true;
		}
	}
	
	// dry land
	if (monst->info.flags & MONST_RESTRICTED_TO_LIQUID
		&& !cellHasTerrainFlag(x, y, ALLOWS_SUBMERGING)) {
		return true;
	}
	
	// monsters can always attack player
	if (player.xLoc == x && player.yLoc == y && monst != &player) {
		return false;
	}
	
	// walls
	if (cellHasTerrainFlag(x, y, OBSTRUCTS_PASSABILITY)) {
		defender = monsterAtLoc(x, y);
		if ((monst != &player && cellHasTerrainFlag(x, y, IS_SECRET))
			|| (defender && (defender->info.flags & MONST_ATTACKABLE_THRU_WALLS))) {
			return false;
		}
		return true;
	}
	
	// hidden terrain
	if (cellHasTerrainFlag(x, y, IS_SECRET) && monst == &player) {
		return false; // player won't avoid what he doesn't know about
	}
	
	// brimstone
	if (!(monst->info.flags & MONST_IMMUNE_TO_FIRE) && cellHasTerrainFlag(x, y, (SPONTANEOUSLY_IGNITES))
		&& !(pmap[x][y].flags & (HAS_MONSTER | HAS_PLAYER))
		&& !cellHasTerrainFlag(monst->xLoc, monst->yLoc, IS_FIRE)
		&& (monst == &player || (monst->creatureState != MONSTER_TRACKING_SCENT && monst->creatureState != MONSTER_FLEEING))) {
		return true;
	}
	
	// burning wandering monsters avoid flammable terrain out of common courtesy
	if (monst != &player
		&& monst->creatureState == MONSTER_WANDERING
		&& (monst->info.flags & MONST_FIERY)
		&& cellHasTerrainFlag(x, y, IS_FLAMMABLE)) {
		return true;
	}
	
	// fire
	if (!(monst->info.flags & MONST_IMMUNE_TO_FIRE) && cellHasTerrainFlag(x, y, IS_FIRE)
		&& !cellHasTerrainFlag(monst->xLoc, monst->yLoc, IS_FIRE)
		&& !(pmap[x][y].flags & (HAS_MONSTER | HAS_PLAYER))
		&& (monst != &player || rogue.mapToShore[x][y] >= player.status.immuneToFire)) {
		return true;
	}
	
	// non-fire harmful terrain
	if (cellHasTerrainFlag(x, y, (HARMFUL_TERRAIN & ~IS_FIRE))
		&& !cellHasTerrainFlag(monst->xLoc, monst->yLoc, (HARMFUL_TERRAIN & ~IS_FIRE))) {
		return true;
	}
	
	if (!(monst->status.levitating)) { // monster doesn't fly
		
		// chasms or trap doors
		if (cellHasTerrainFlag(x, y, TRAP_DESCENT)
			&& (!cellHasTerrainFlag(x, y, ENTANGLES) || !(monst->info.flags & MONST_IMMUNE_TO_WEBS))) {
			return true;
		}
		
		// gas or other environmental traps
		if (cellHasTerrainFlag(x, y, IS_DF_TRAP)
			&& !(pmap[x][y].flags & PRESSURE_PLATE_DEPRESSED)
			&& (monst == &player || monst->creatureState == MONSTER_WANDERING
				|| (monst->creatureState == MONSTER_ALLY && !(cellHasTerrainFlag(x, y, IS_SECRET))))
			&& !(monst->status.entranced)
			&& (!cellHasTerrainFlag(x, y, ENTANGLES) || !(monst->info.flags & MONST_IMMUNE_TO_WEBS))) {
			return true;
		}
		
		// lava
		if (!(monst->info.flags & MONST_IMMUNE_TO_FIRE) && cellHasTerrainFlag(x, y, (LAVA_INSTA_DEATH))
			&& (!cellHasTerrainFlag(x, y, ENTANGLES) || !(monst->info.flags & MONST_IMMUNE_TO_WEBS))
			&& (monst != &player || rogue.mapToShore[x][y] >= max(player.status.immuneToFire, player.status.levitating))) {
			return true;
		}
		
		// deep water
		if (!(monst->info.flags & MONST_IMMUNE_TO_WATER) && cellHasTerrainFlag(x, y, IS_DEEP_WATER)
			&& (!cellHasTerrainFlag(x, y, ENTANGLES) || !(monst->info.flags & MONST_IMMUNE_TO_WEBS))
			&& !cellHasTerrainFlag(monst->xLoc, monst->yLoc, IS_DEEP_WATER)) {
			return true; // avoid only if not already in it
		}
		
		// poisonous lichen
		if (cellHasTerrainFlag(x, y, CAUSES_POISON)
			&& !cellHasTerrainFlag(monst->xLoc, monst->yLoc, CAUSES_POISON)
			&& (monst == &player || monst->creatureState != MONSTER_TRACKING_SCENT || monst->currentHP < 10)) {
			return true;
		}
	}
	return false;
}

boolean moveMonsterPassivelyTowards(creature *monst, short targetLoc[2], boolean willingToAttackPlayer) {
	short x, y, dx, dy, newX, newY;
	
	x = monst->xLoc;
	y = monst->yLoc;
	
	if (targetLoc[0] == x) {
		dx = 0;
	} else {
		dx = (targetLoc[0] < x ? -1 : 1);
	}
	if (targetLoc[1] == y) {
		dy = 0;
	} else {
		dy = (targetLoc[1] < y ? -1 : 1);
	}
	
	if (dx == 0 && dy == 0) { // already at the destination
		return false;
	}
	
	newX = x + dx;
	newY = y + dy;
	
	if (monst->creatureState != MONSTER_TRACKING_SCENT && dx && dy) {
		if (abs(targetLoc[0] - x) > abs(targetLoc[1] - y) && rand_range(0, abs(targetLoc[0] - x)) > abs(targetLoc[1] - y)) {
			if (!(monsterAvoids(monst, newX, y) || (!willingToAttackPlayer && (pmap[newX][y].flags & HAS_PLAYER)) || !moveMonster(monst, dx, 0))) {
				return true;
			}
		} else if (abs(targetLoc[0] - x) < abs(targetLoc[1] - y) && rand_range(0, abs(targetLoc[1] - y)) > abs(targetLoc[0] - x)) {
			if (!(monsterAvoids(monst, x, newY) || (!willingToAttackPlayer && (pmap[x][newY].flags & HAS_PLAYER)) || !moveMonster(monst, 0, dy))) {
				return true;
			}
		}
	}
	
	// Try to move toward the goal diagonally if possible or else straight.
	// If that fails, try both directions for the shorter coordinate.
	// If they all fail, return false.
	if (monsterAvoids(monst, newX, newY) || (!willingToAttackPlayer && pmap[newX][newY].flags & HAS_PLAYER) || !moveMonster(monst, dx, dy)) {
		if (distanceBetween(x, y, targetLoc[0], targetLoc[1]) <= 1 && (dx == 0 || dy == 0)) { // cardinally adjacent
			return false; // destination is blocked
		}
		//abs(targetLoc[0] - x) < abs(targetLoc[1] - y)
		if ((max(targetLoc[0], x) - min(targetLoc[0], x)) < (max(targetLoc[1], y) - min(targetLoc[1], y))) {
			if (monsterAvoids(monst, x, newY) || (!willingToAttackPlayer && pmap[x][newY].flags & HAS_PLAYER) || !moveMonster(monst, 0, dy)) {
				if (monsterAvoids(monst, newX, y) || (!willingToAttackPlayer &&  pmap[newX][y].flags & HAS_PLAYER) || !moveMonster(monst, dx, 0)) {
					if (monsterAvoids(monst, x-1, newY) || (!willingToAttackPlayer && pmap[x-1][newY].flags & HAS_PLAYER) || !moveMonster(monst, -1, dy)) {
						if (monsterAvoids(monst, x+1, newY) || (!willingToAttackPlayer && pmap[x+1][newY].flags & HAS_PLAYER) || !moveMonster(monst, 1, dy)) {
							return false;
						}
					}
				}
			}
		} else {
			if (monsterAvoids(monst, newX, y) || (!willingToAttackPlayer && pmap[newX][y].flags & HAS_PLAYER) || !moveMonster(monst, dx, 0)) {
				if (monsterAvoids(monst, x, newY) || (!willingToAttackPlayer && pmap[x][newY].flags & HAS_PLAYER) || !moveMonster(monst, 0, dy)) {
					if (monsterAvoids(monst, newX, y-1) || (!willingToAttackPlayer && pmap[newX][y-1].flags & HAS_PLAYER) || !moveMonster(monst, dx, -1)) {
						if (monsterAvoids(monst, newX, y+1) || (!willingToAttackPlayer && pmap[newX][y+1].flags & HAS_PLAYER) || !moveMonster(monst, dx, 1)) {
							return false;
						}
					}
				}
			}
		}
	}
	return true;
}

short distanceBetween(short x1, short y1, short x2, short y2) {
	short dx, dy;
	
	dx = x1 - x2;
	if (dx < 0) {
		dx *= -1;
	}
	
	dy = y1 - y2;
	if (dy < 0) {
		dy *= -1;
	}
	
	return max(dx, dy);
}

void wakeUp(creature *monst) {
	creature *teammate;
	
	monst->creatureState =
	(monst->creatureMode == MODE_PERM_FLEEING ? MONSTER_FLEEING : MONSTER_TRACKING_SCENT);
	chooseNewWanderDestination(monst);
	monst->ticksUntilTurn = 100;
	
	for (teammate = monsters->nextCreature; teammate != NULL; teammate = teammate->nextCreature) {
		if (monstersAreTeammates(monst, teammate) && teammate->creatureMode == MODE_NORMAL) {
			teammate->creatureState =
			(teammate->creatureMode == MODE_PERM_FLEEING ? MONSTER_FLEEING : MONSTER_TRACKING_SCENT);
			chooseNewWanderDestination(teammate);
			teammate->ticksUntilTurn = 100;
		}
	}
}

// Assumes that observer is not the player.
short awarenessDistance(creature *observer, creature *target) {
	short perceivedDistance, bonus = 0;
	
	// start with base distance
	
	if ((observer->status.levitating || (observer->info.flags & MONST_RESTRICTED_TO_LIQUID) || (observer->bookkeepingFlags & MONST_SUBMERGED)
		 || ((observer->info.flags & MONST_IMMUNE_TO_WEBS) && (observer->info.abilityFlags & MA_SHOOTS_WEBS)))
		&& ((target == &player && (pmap[observer->xLoc][observer->yLoc].flags & IN_FIELD_OF_VIEW)) ||
			(target != &player && openPathBetween(observer->xLoc, observer->yLoc, target->xLoc, target->yLoc)))) {
			// if monster flies or is waterbound or is underwater or can cross pits with webs:
			perceivedDistance = distanceBetween(observer->xLoc, observer->yLoc, target->xLoc, target->yLoc) * 3;
			
		} else {
			
			perceivedDistance = (rogue.scentTurnNumber - tmap[observer->xLoc][observer->yLoc].scent); // this value is triple the apparent distance
		}
	
	perceivedDistance = min(perceivedDistance, 1000);
	
	if (perceivedDistance < 0) {
		perceivedDistance = 1000;
	}
	
	// calculate bonus modifiers
	
	if (tmap[target->xLoc][target->yLoc].light[0] < 0
		&& tmap[target->xLoc][target->yLoc].light[1] < 0
		&& tmap[target->xLoc][target->yLoc].light[2] < 0) {
		// super-darkness
		bonus += 5;
	}
	if (observer->creatureState == MONSTER_SLEEPING) {
		bonus += 3;
	}
	
	if (target == &player) {
		bonus += rogue.stealthBonus;
		if (rogue.justRested) {
			bonus = (bonus + 1) * 2;
		}
		if (observer->creatureState == MONSTER_TRACKING_SCENT) {
			bonus -= 4;
		}
	}
	if (pmap[target->xLoc][target->yLoc].flags & IS_IN_SHADOW) {
		bonus = (bonus + 1) * 2;
	}
	
	// apply bonus
	perceivedDistance *= pow(1.1, bonus);
	if (perceivedDistance < 0 || perceivedDistance > 10000) {
		return 10000;
	}
	return perceivedDistance;
}

// yes or no -- observer is aware of the target as of this new turn.
// note that this takes into account whether it is ALREADY aware of the target.
boolean awareOfTarget(creature *observer, creature *target) {
	short perceivedDistance = awarenessDistance(observer, target);
	short awareness = observer->info.scentThreshold * 3; // forget sight, it sucks
	
	if (perceivedDistance > awareness) {
		// out of awareness range
		return false;
	}
	if (observer->creatureState == MONSTER_TRACKING_SCENT) {
		// already aware of the target
		return true;
	}
	if (observer->creatureState == MONSTER_SLEEPING && target == &player
		&& !(pmap[observer->xLoc][observer->yLoc].flags & IN_FIELD_OF_VIEW)) {
		// observer asleep and player-target not in field of view
		return false;
	}
	// within range but currently unaware
	return ((rand_range(0, perceivedDistance) == 0) ? true : false);
}

void updateMonsterState(creature *monst) {
	short x, y;
	boolean awareOfPlayer;
	
	if (monst->info.flags & MONST_ALWAYS_HUNTING) {
		monst->creatureState = MONSTER_TRACKING_SCENT;
		return;
	}
	
	x = monst->xLoc;
	y = monst->yLoc;
	
	awareOfPlayer = awareOfTarget(monst, &player);
	
	if (monst->creatureMode == MODE_PERM_FLEEING
		&& (monst->creatureState == MONSTER_WANDERING || monst->creatureState == MONSTER_TRACKING_SCENT)) {
		monst->creatureState = MONSTER_FLEEING;
		chooseNewWanderDestination(monst);
	}
	
	if ((monst->creatureState == MONSTER_WANDERING) && awareOfPlayer) {
		// If wandering, but the scent is stronger than the scent detection threshold, start tracking the scent.
		monst->creatureState = MONSTER_TRACKING_SCENT;
	} else if (monst->creatureState == MONSTER_SLEEPING) {
		// if sleeping, the monster has a chance to awaken
		
		if (awareOfPlayer) {
			wakeUp(monst); // wakes up the whole horde if necessary
			
			//			if (canSeeMonster(monst)) {
			//				monsterName(monstName, monst, true);
			//				sprintf(buf, "%s awakens!", monstName);
			//				message(buf, true, false);
			//			}
		}
	} else if ((monst->creatureState == MONSTER_TRACKING_SCENT) && !awareOfPlayer) {
		// if tracking scent, but the scent is weaker than the scent detection threshold, begin wandering.
		monst->creatureState = MONSTER_WANDERING;
		
		chooseNewWanderDestination(monst);
	} else if (monst->creatureState == MONSTER_TRACKING_SCENT
			   && (monst->info.flags & MONST_MAINTAINS_DISTANCE
				   || (monst->info.abilityFlags & MA_POISONS && player.status.poisoned))
			   && distanceBetween(x, y, player.xLoc, player.yLoc) < 3
			   && pmap[x][y].flags & IN_FIELD_OF_VIEW) {
		monst->creatureState = MONSTER_FLEEING;
	} else if (monst->creatureMode == MODE_NORMAL
			   && monst->creatureState == MONSTER_FLEEING
			   && monst->info.flags & MONST_MAINTAINS_DISTANCE
			   && !(monst->status.magicalFear)
			   && distanceBetween(x, y, player.xLoc, player.yLoc) >= 3) {
		monst->creatureState = MONSTER_TRACKING_SCENT;
	} else if (monst->creatureMode == MODE_NORMAL
			   && monst->creatureState == MONSTER_FLEEING
			   && monst->info.flags & MONST_FLEES_NEAR_DEATH
			   && !(monst->status.magicalFear)
			   && monst->currentHP >= monst->info.maxHP * 3 / 4) {
		monst->creatureState = (((monst->bookkeepingFlags & MONST_FOLLOWER) && monst->leader == &player)
								? MONSTER_ALLY : MONSTER_TRACKING_SCENT);
	} else if (monst->creatureMode == MODE_NORMAL
			   && monst->creatureState == MONSTER_FLEEING
			   && !(monst->info.flags & MONST_MAINTAINS_DISTANCE)
			   && !(monst->status.magicalFear)
			   && monst->info.abilityFlags & MA_POISONS
			   && !player.status.poisoned) {
		monst->creatureState = MONSTER_TRACKING_SCENT;
	}
}

void decrementMonsterStatus(creature *monst) {
	short damage;
	char buf[COLS], buf2[COLS];
	
	monst->bookkeepingFlags &= ~MONST_JUST_SUMMONED;
	
	if (monst->currentHP < monst->info.maxHP && monst->info.turnsBetweenRegen > 0 && !monst->status.poisoned) {
		if ((monst->turnsUntilRegen -= 1000) <= 0) {
			monst->currentHP++;
			monst->turnsUntilRegen += monst->info.turnsBetweenRegen;
		}
	}
	
	if (monst->status.telepathic) {
		monst->status.telepathic--;
	}
	
	if (monst->status.explosionImmunity) {
		monst->status.explosionImmunity--;
	}
	
	if (monst->status.discordant) {
		monst->status.discordant--;
	}
	
	if (monst->status.levitating && !(monst->info.flags & MONST_FLIES)) {
		monst->status.levitating--;
	}
	if (monst->status.slowed) {
		if (!--monst->status.slowed) {
			monst->movementSpeed = monst->info.movementSpeed;
			monst->attackSpeed = monst->info.attackSpeed;
		}
	}
	if (monst->status.hasted) {
		if (!--monst->status.hasted) {
			monst->movementSpeed = monst->info.movementSpeed;
			monst->attackSpeed = monst->info.attackSpeed;
		}
	}
	if (monst->status.confused) {
		monst->status.confused--;
	}
	if (monst->status.burning) {
		if (!(monst->info.flags & MONST_FIERY)) {
			monst->status.burning--;
		}
		damage = rand_range(1, 3);
		if (!(monst->info.flags & MONST_IMMUNE_TO_FIRE) && inflictDamage(NULL, monst, damage, &orange)) {
			if (canSeeMonster(monst)) {
				monsterName(buf, monst, true);
				sprintf(buf2, "%s burns to death.", buf);
				message(buf2, true, false);
			}
			return;
		}
		if (monst->status.burning <= 0) {
			extinguishFireOnCreature(monst);
		}
	}
	if (monst->status.poisoned) {
		monst->status.poisoned--;
		if (inflictDamage(NULL, monst, 1, &green)) {
			if (canSeeMonster(monst)) {
				monsterName(buf, monst, true);
				sprintf(buf2, "%s dies of poison.", buf);
				message(buf2, true, false);
			}
			return;
		}
	}
	if (monst->status.paralyzed) {
		monst->status.paralyzed--;
	}
	if (monst->status.entranced) {
		monst->status.entranced--;
	}
	if (monst->status.stuck && !cellHasTerrainFlag(monst->xLoc, monst->yLoc, ENTANGLES)) {
		monst->status.stuck = 0;
	}
	if (monst->status.stuck) {
		if (!--monst->status.stuck) {
			if (tileCatalog[pmap[monst->xLoc][monst->yLoc].layers[SURFACE]].flags & ENTANGLES) {
				pmap[monst->xLoc][monst->yLoc].layers[SURFACE] = NOTHING;
			}
		}
	}
	if (monst->status.nauseous) {
		monst->status.nauseous--;
	}
	//	if (monst->status.nutrition) {
	//		monst->status.nutrition--;
	//	}
	if (monst->status.entersLevelIn) {
		monst->status.entersLevelIn--;
	}
	if (monst->status.magicalFear) {
		if (!--monst->status.magicalFear
			&& monst->creatureState == MONSTER_FLEEING) {
			monst->creatureState = MONSTER_TRACKING_SCENT;
		}
	}
	if (monst->status.hallucinating) {
		monst->status.hallucinating--;
	}
	if (monst->info.flags & MONST_SUBMERGES && !(monst->bookkeepingFlags & MONST_SUBMERGED)
		&& (cellHasTerrainFlag(monst->xLoc, monst->yLoc, ALLOWS_SUBMERGING))
		&& !(monst->bookkeepingFlags & MONST_SEIZING)) {
		if (rand_percent(20)) {
			monst->bookkeepingFlags |= MONST_SUBMERGED;
			if (!monst->status.magicalFear && monst->creatureState == MONSTER_FLEEING
				&& (!(monst->info.flags & MONST_FLEES_NEAR_DEATH) || monst->currentHP >= monst->info.maxHP * 3 / 4)) {
				monst->creatureState = MONSTER_TRACKING_SCENT;
			}
			refreshDungeonCell(monst->xLoc, monst->yLoc);
		} else if (monst->info.flags & (MONST_RESTRICTED_TO_LIQUID)
				   && monst->creatureState != MONSTER_ALLY) {
			monst->creatureState = MONSTER_FLEEING;
			chooseNewWanderDestination(monst);
		}
	}
}

boolean traversiblePathBetween(creature *monst, short x2, short y2) {
	short coords[DCOLS][2], i, x, y, n;
	short originLoc[2] = {monst->xLoc, monst->yLoc};
	short targetLoc[2] = {x2, y2};
	
	n = getLineCoordinates(coords, originLoc, targetLoc);
	
	for (i=0; i<n; i++) {
		x = coords[i][0];
		y = coords[i][1];
		if (x == x2 && y == y2) {
			return true;
		}
		if (monsterAvoids(monst, x, y)) {
			return false;
		}
	}
	return true; // should never get here
}

boolean specifiedPathBetween(short x1, short y1, short x2, short y2,
							 unsigned long blockingTerrain, unsigned long blockingFlags) {
	short coords[DCOLS][2], i, x, y, n;
	short originLoc[2] = {x1, y1};
	short targetLoc[2] = {x2, y2};
	n = getLineCoordinates(coords, originLoc, targetLoc);
	
	for (i=0; i<n; i++) {
		x = coords[i][0];
		y = coords[i][1];
		if (cellHasTerrainFlag(x, y, blockingTerrain) || (pmap[x][y].flags & blockingFlags)) {
			return false;
		}
		if (x == x2 && y == y2) {
			return true;
		}
	}
	return true; // should never get here
}

boolean openPathBetween(short x1, short y1, short x2, short y2) {
	short returnLoc[2], startLoc[2] = {x1, y1}, targetLoc[2] = {x2, y2};
	
	getImpactLoc(returnLoc, startLoc, targetLoc, DCOLS, false);
	if (returnLoc[0] == targetLoc[0] && returnLoc[1] == targetLoc[1]) {
		return true;
	}
	return false;
}

// will return the player if the player is at (x, y).
creature *monsterAtLoc(short x, short y) {
	creature *monst;
	if (!(pmap[x][y].flags & (HAS_MONSTER | HAS_PLAYER))) {
		return NULL;
	}
	if (player.xLoc == x && player.yLoc == y) {
		return &player;
	}
	for (monst = monsters->nextCreature; monst != NULL && (monst->xLoc != x || monst->yLoc != y); monst = monst->nextCreature);
	return monst;
}

void moveTowardLeader(creature *monst) {
	short targetLoc[2], dir;
	
	if (monst->creatureState == MONSTER_ALLY && !(monst->leader)) {
		monst->leader = &player;
		monst->bookkeepingFlags |= MONST_FOLLOWER;
	}
	
	if ((monst->bookkeepingFlags & MONST_FOLLOWER)
		&& openPathBetween(monst->xLoc, monst->yLoc, monst->leader->xLoc, monst->leader->yLoc)) {
		targetLoc[0] = monst->leader->xLoc;
		targetLoc[1] = monst->leader->yLoc;
		
		moveMonsterPassivelyTowards(monst, targetLoc, (monst->creatureState != MONSTER_ALLY));
		return;
	}
	
	// okay, poor minion can't see his leader.
	
	// is the leader missing his map altogether?
	if (!monst->leader->mapToMe) {
		monst->leader->mapToMe = allocDynamicGrid();
		calculateDistances(monst->leader->mapToMe, monst->leader->xLoc, monst->leader->yLoc, 0, monst);
	}
	
	// is the leader map out of date?
	if (monst->leader->mapToMe[monst->leader->xLoc][monst->leader->yLoc] > 3) {
		// it is. recalculate the map.
		calculateDistances(monst->leader->mapToMe, monst->leader->xLoc, monst->leader->yLoc, 0, monst);
	}
	
	// follow the map.
	dir = nextStep(monst->leader->mapToMe, monst->xLoc, monst->yLoc);
	targetLoc[0] = monst->xLoc + nbDirs[dir][0];
	targetLoc[1] = monst->yLoc + nbDirs[dir][1];
	if (!moveMonsterPassivelyTowards(monst, targetLoc, (monst->creatureState != MONSTER_ALLY))) {
		// monster is blocking the way
		dir = randValidDirectionFrom(monst, monst->xLoc, monst->yLoc, true);
		targetLoc[0] = monst->xLoc + nbDirs[dir][0];
		targetLoc[1] = monst->yLoc + nbDirs[dir][1];
		moveMonsterPassivelyTowards(monst, targetLoc, (monst->creatureState != MONSTER_ALLY));
	}
}

// isomorphs a number in [0, 40] to coordinates along the square of radius 5 surrounding (0,0)
void perimeterCoords(short returnCoords[2], short n) {
	if (n == 40) {
		n = 0;
	}
	if (n <= 10) {			// top edge, left to right
		returnCoords[0] = n - 5;
		returnCoords[1] = -5;
	} else if (n <= 21) {	// bottom edge, left to right
		returnCoords[0] = (n - 11) - 5;
		returnCoords[1] = 5;
	} else if (n <= 30) {	// left edge, top to bottom
		returnCoords[0] = -5;
		returnCoords[1] = (n - 22) - 4;
	} else if (n <= 39) {	// right edge, top to bottom
		returnCoords[0] = 5;
		returnCoords[1] = (n - 31) - 4;		
	} else {
		returnCoords[0] = returnCoords[1] = 0; // garbage in, garbage out
	}
}

// returns whether the monster did something (and therefore ended its turn)
boolean monsterBlinkToSafety(creature *monst) {
	short i, bestTarget[2], bestSafety, nowSafety,
	maxDistance, target[2], impact[2], origin[2];
	short **blinkSafetyMap;
	boolean gotOne;
	char monstName[DCOLS];
	char buf[DCOLS];
	
	maxDistance = 5 * 2 + 2;
	gotOne = false;
	
	origin[0] = monst->xLoc;
	origin[1] = monst->yLoc;
	
	if (monst->creatureState == MONSTER_ALLY) {
		if (!rogue.updatedAllySafetyMapThisTurn) {
			updateAllySafetyMap();
		}
		blinkSafetyMap = allySafetyMap;
	} else if (pmap[monst->xLoc][monst->yLoc].flags & IN_FIELD_OF_VIEW) {
		if (monst->safetyMap) {
			freeDynamicGrid(monst->safetyMap);
			monst->safetyMap = NULL;
		}
		if (!rogue.updatedSafetyMapThisTurn) {
			updateSafetyMap();
		}
		blinkSafetyMap = safetyMap;
	} else {
		if (!monst->safetyMap) {
			monst->safetyMap = allocDynamicGrid();
			copyDynamicGrid(monst->safetyMap, safetyMap);
		}
		blinkSafetyMap = monst->safetyMap;
	}
	
	bestTarget[0]	= 0;
	bestTarget[1]	= 0;
	bestSafety		= blinkSafetyMap[monst->xLoc][monst->yLoc];
	
	for (i=0; i<40; i++) {
		perimeterCoords(target, i);
		target[0] += monst->xLoc;
		target[1] += monst->yLoc;
		
		getImpactLoc(impact, origin, target, maxDistance, true);
		nowSafety	= blinkSafetyMap[impact[0]][impact[1]];
		
		if (nowSafety < bestSafety) {
			bestTarget[0]	= target[0];
			bestTarget[1]	= target[1];
			bestSafety		= nowSafety;
			gotOne			= true;
		}
	}
	
	if (gotOne) {
		if (canSeeMonster(monst)) {
			monsterName(monstName, monst, true);
			sprintf(buf, "%s blinks", monstName);
			combatMessage(buf);
		}
		monst->ticksUntilTurn = monst->attackSpeed * (monst->info.flags & MONST_CAST_SPELLS_SLOWLY ? 2 : 1);
		zap(origin, bestTarget, BOLT_BLINKING, 5, false);
		return true;
	}
	return false;
}

// returns whether the monster did something (and therefore ended its turn)
boolean monstUseMagic(creature *monst) {
	short originLoc[2] = {monst->xLoc, monst->yLoc};
	short targetLoc[2];
	short listOfCoordinates[DCOLS][2], blinkDistance, numCells;
	short weakestAllyHealthFraction = 100;
	creature *target, *weakestAlly, *monst2;
	char monstName[DCOLS];
	char buf[DCOLS];
	short minionCount = 0;
	boolean noBlinking = false;
	
	// abilities that target itself:
	if (monst->info.abilityFlags & (MA_CAST_SUMMON)) {
		for (target = monsters->nextCreature; target != NULL; target = target->nextCreature) {
			if (monstersAreTeammates(monst, target)) {
				minionCount++;
			}
		}
		if ((monst->creatureState != MONSTER_ALLY || minionCount < 5)
			&& !rand_range(0, minionCount * minionCount * 3 + 1)) {
			summonMinions(monst);
			return true;
		}
	}
	
	// strong abilities that might target the caster's enemies:
	if (!monst->status.confused && monst->info.abilityFlags
		& (MA_CAST_BLINK | MA_CAST_CANCEL | MA_CAST_SLOW | MA_CAST_DISCORD | MA_BREATHES_FIRE | MA_SHOOTS_WEBS | MA_ATTACKS_FROM_DISTANCE)) {
		CYCLE_MONSTERS_AND_PLAYERS(target) {
			if (monstersAreEnemies(monst, target) && monst != target && !(monst->bookkeepingFlags & MONST_SUBMERGED)
				&& openPathBetween(monst->xLoc, monst->yLoc, target->xLoc, target->yLoc)) {
				
				targetLoc[0] = target->xLoc;
				targetLoc[1] = target->yLoc;
				
				// dragons sometimes breathe fire
				if (monst->info.abilityFlags & MA_BREATHES_FIRE && rand_percent(40)) {
					monsterName(monstName, monst, true);
					if (canSeeMonster(monst) || canSeeMonster(target)) {
						sprintf(buf, "%s breathes fire!", monstName);
						message(buf, true, false);
					}
					zap(originLoc, targetLoc, BOLT_FIRE, 18, false);
					if (player.currentHP <= 0) {
						gameOver(monsterCatalog[monst->info.monsterID].monsterName, false);
					}
					return true;
				}
				
				// spiders sometimes shoot webs:
				if (monst->info.abilityFlags & MA_SHOOTS_WEBS
					&& !(target->info.flags & MONST_INANIMATE)
					&& (!target->status.stuck || distanceBetween(monst->xLoc, monst->yLoc, targetLoc[0], targetLoc[1]) > 1)
					&& distanceBetween(monst->xLoc, monst->yLoc, targetLoc[0], targetLoc[1]) < 20
					&& ((!target->status.stuck && distanceBetween(monst->xLoc, monst->yLoc, targetLoc[0], targetLoc[1]) == 1)
						|| rand_percent(20))) {
						
						shootWeb(monst, targetLoc, SPIDERWEB);
						return true;
					}
				
				// centaurs shoot from a distance:
				if (monst->info.abilityFlags & MA_ATTACKS_FROM_DISTANCE
					//&& distanceBetween(monst->xLoc, monst->yLoc, targetLoc[0], targetLoc[1]) > 1
					&& distanceBetween(monst->xLoc, monst->yLoc, targetLoc[0], targetLoc[1]) < 11) {
					monsterShoots(monst, targetLoc,
								  (monst->info.abilityFlags & MA_HIT_DEGRADE_ARMOR ? '*' : WEAPON_CHAR),
								  (monst->info.abilityFlags & MA_HIT_DEGRADE_ARMOR ? &green : &gray));
					return true;
				}
				
				// dar battlemages sometimes blink
				if (monst->info.abilityFlags & (MA_CAST_BLINK) && !(monst->info.flags & MONST_MAINTAINS_DISTANCE)
					&& distanceBetween(monst->xLoc,	monst->yLoc, target->xLoc, target->yLoc) > 2 && rand_percent(50)) {
					
					CYCLE_MONSTERS_AND_PLAYERS(monst2) {
						if (monstersAreEnemies(monst, monst2)
							&& distanceBetween(monst->xLoc, monst->yLoc, monst2->xLoc, monst2->yLoc) <= 1) {
							noBlinking = true;
							break;
						}
					}
					if (!noBlinking) {
						numCells = getLineCoordinates(listOfCoordinates, originLoc, targetLoc);
						blinkDistance = min(numCells, 5 * 2 + 1);
						blinkDistance = min(blinkDistance, distanceBetween(monst->xLoc, monst->yLoc, target->xLoc, target->yLoc) - 2);
						if (!monsterAvoids(monst, listOfCoordinates[blinkDistance][0], listOfCoordinates[blinkDistance][1])) {
							
							if (canSeeMonster(monst)) {
								monsterName(monstName, monst, true);
								sprintf(buf, "%s blinks", monstName);
								combatMessage(buf);
							}
							
							zap(originLoc, targetLoc, BOLT_BLINKING, 5, false);
							return true;
						}
					}
				}
				
				if (monst->info.abilityFlags & (MA_CAST_DISCORD) && (!target->status.discordant)
					&& !(target->info.flags & MONST_INANIMATE) && (target != &player) && rand_percent(50)) {
					if (canSeeMonster(monst)) {
						monsterName(monstName, monst, true);
						sprintf(buf, "%s casts a spell of discord", monstName);
						combatMessage(buf);
					}
					zap(originLoc, targetLoc, BOLT_DISCORD, 10, false);
					return true;
				}
				
				if (monst->info.abilityFlags & (MA_CAST_CANCEL)
					&& (target->status.hasted || target->status.telepathic
						|| ((target->status.immuneToFire || target->status.levitating)
							&& cellHasTerrainFlag(target->xLoc, target->yLoc, (LAVA_INSTA_DEATH | DEEP_WATER | TRAP_DESCENT))))
					&& rand_percent(50)) {
					if (canSeeMonster(monst)) {
						monsterName(monstName, monst, true);
						sprintf(buf, "%s casts a cancellation spell", monstName);
						combatMessage(buf);
					}
					zap(originLoc, targetLoc, BOLT_CANCELLATION, 10, false);
					return true;
				}
				
				if (monst->info.abilityFlags & (MA_CAST_SLOW) && (!target->status.slowed)
					&& !(target->info.flags & MONST_INANIMATE) && rand_percent(50)) {
					if (canSeeMonster(monst)) {
						monsterName(monstName, monst, true);
						sprintf(buf, "%s casts a spell of slowness", monstName);
						combatMessage(buf);
					}
					zap(originLoc, targetLoc, BOLT_SLOW, 10, false);
					return true;
				}
			}
			
		}
	}
	
	// abilities that might target allies:
	
	if (!monst->status.confused && monst->info.abilityFlags & MA_CAST_CANCEL && rand_percent(75)) {
		CYCLE_MONSTERS_AND_PLAYERS(target) {
			if (target != monst
				&& (target->status.slowed || target->status.entranced || target->status.magicalFear)
				&& monstersAreTeammates(monst, target)
				&& !monstersAreEnemies(monst, target)
				&& !(monst->bookkeepingFlags & MONST_SUBMERGED)
				&& openPathBetween(monst->xLoc, monst->yLoc, target->xLoc, target->yLoc)) {
				
				if (canSeeMonster(monst)) {
					monsterName(monstName, monst, true);
					sprintf(buf, "%s casts a cancellation spell", monstName);
					combatMessage(buf);
				}
				
				targetLoc[0] = target->xLoc;
				targetLoc[1] = target->yLoc;
				zap(originLoc, targetLoc, BOLT_CANCELLATION, 10, false);
				return true;
			}
		}
	}
	
	if (!monst->status.confused && monst->info.abilityFlags & MA_CAST_HASTE && rand_percent(75)) {
		CYCLE_MONSTERS_AND_PLAYERS(target) {
			if (target != monst && !target->status.hasted && target->creatureState == MONSTER_TRACKING_SCENT
				&& monstersAreTeammates(monst, target)
				&& !monstersAreEnemies(monst, target)
				&& !(target->bookkeepingFlags & MONST_SUBMERGED)
				&& !(target->info.flags & MONST_INANIMATE)
				&& openPathBetween(monst->xLoc, monst->yLoc, target->xLoc, target->yLoc)) {
				
				if (canSeeMonster(monst)) {
					monsterName(monstName, monst, true);
					sprintf(buf, "%s casts a spell of speed", monstName);
					combatMessage(buf);
				}
				
				targetLoc[0] = target->xLoc;
				targetLoc[1] = target->yLoc;
				zap(originLoc, targetLoc, BOLT_HASTE, 2, false);
				return true;
			}
		}
	}
	
	weakestAlly = NULL;
	
	if (!monst->status.confused
		&& (monst->info.abilityFlags & MA_CAST_HEAL)
		&& !(monst->bookkeepingFlags & (MONST_SUBMERGED | MONST_CAPTIVE))
		&& rand_percent(50)) {
		CYCLE_MONSTERS_AND_PLAYERS(target) {
			if (target != monst
				&& (100 * target->currentHP / target->info.maxHP < weakestAllyHealthFraction)
				&& monstersAreTeammates(monst, target)
				&& !monstersAreEnemies(monst, target)
				&& openPathBetween(monst->xLoc, monst->yLoc, target->xLoc, target->yLoc)) {
				weakestAllyHealthFraction = 100 * target->currentHP / target->info.maxHP;
				weakestAlly = target;
			}
		}
		if (weakestAllyHealthFraction < 100) {
			target = weakestAlly;
			targetLoc[0] = target->xLoc;
			targetLoc[1] = target->yLoc;
			
			if (canSeeMonster(monst)) {
				monsterName(monstName, monst, true);
				sprintf(buf, "%s casts a healing spell", monstName);
				combatMessage(buf);
			}
			
			zap(originLoc, targetLoc, BOLT_HEALING, 5, false);
			return true;
		}
	}
	
	// weak direct damage spells against enemies:
	
	if (!monst->status.confused && monst->info.abilityFlags & (MA_CAST_FIRE | MA_CAST_SPARK)) {
		CYCLE_MONSTERS_AND_PLAYERS(target) {
			if (monstersAreEnemies(monst, target)
				&& monst != target
				&& !(monst->bookkeepingFlags & MONST_SUBMERGED)) {
				if (openPathBetween(monst->xLoc, monst->yLoc, target->xLoc, target->yLoc)) {
					
					targetLoc[0] = target->xLoc;
					targetLoc[1] = target->yLoc;
					
					if (monst->info.abilityFlags & (MA_CAST_FIRE) && rand_percent(50)) {
						if (canSeeMonster(monst)) {
							monsterName(monstName, monst, true);
							sprintf(buf, "%s shoots a firebolt", monstName);
							combatMessage(buf);
						}
						zap(originLoc, targetLoc, BOLT_FIRE, 2, false);
						if (player.currentHP <= 0) {
							monsterName(monstName, monst, false);
							gameOver(monsterCatalog[monst->info.monsterID].monsterName, false);
						}
						return true;
					}
					
					if (monst->info.abilityFlags & (MA_CAST_SPARK) && rand_percent(50)) {
						if (canSeeMonster(monst)) {
							monsterName(monstName, monst, true);
							sprintf(buf, "%s shoots a spark of lightning", monstName);
							combatMessage(buf);
						}
						zap(originLoc, targetLoc, BOLT_LIGHTNING, 1, false);
						if (player.currentHP <= 0) {
							monsterName(monstName, monst, false);
							gameOver(monsterCatalog[monst->info.monsterID].monsterName, false);
						}
						return true;
					}
				}
			}
		}
	}
	
	return false;
}

// returns the direction the player's scent points to from a given cell. Returns -1 if the nose comes up blank.
short scentDirection(creature *monst) {
	short i, j, newX, newY, x, y, bestDirection = -1, newestX, newestY;
	unsigned short bestNearbyScent = 0;
	boolean canTryAgain = true;
	
	x = monst->xLoc;
	y = monst->yLoc;
	
	for (;;) {
		
		for (i=0; i<8; i++) {
			newX = x + nbDirs[i][0];
			newY = y + nbDirs[i][1];
			if ((tmap[newX][newY].scent > bestNearbyScent) &&
				!(pmap[newX][newY].flags & HAS_MONSTER) &&
				!cellHasTerrainFlag(newX, newY, OBSTRUCTS_PASSABILITY) &&
				!cellHasTerrainFlag(newX, y, OBSTRUCTS_PASSABILITY) &&
				!cellHasTerrainFlag(x, newY, OBSTRUCTS_PASSABILITY) &&
				!monsterAvoids(monst, newX, newY)) {
				
				bestNearbyScent = tmap[newX][newY].scent;
				bestDirection = i;
			}
		}
		
		if (bestDirection >= 0 && bestNearbyScent > tmap[x][y].scent) {
			return bestDirection;
		}
		
		if (canTryAgain) {
			// okay the monster may be stuck in some irritating diagonal.
			// If so, we can diffuse the scent into the offending kink and solve the problem.
			// There's a possibility he's stuck for some other reason, though, so we'll only
			// try once per his move -- hence the failsafe.
			canTryAgain = false;
			for (i=0; i<4; i++) {
				newX = x + nbDirs[i][0];
				newY = y + nbDirs[i][1];
				for (j=0; j<4; j++) {
					newestX = newX + nbDirs[j][0];
					newestY = newY + nbDirs[j][1];
					tmap[newX][newY].scent = max(tmap[newX][newY].scent, tmap[newestX][newestY].scent - 1);
				}
			}
		} else {
			return -1; // failure!
		}
	}
}

void unAlly(creature *monst) {
	if (monst->creatureState == MONSTER_ALLY) {
		monst->creatureState = MONSTER_TRACKING_SCENT;
		monst->bookkeepingFlags &= ~MONST_FOLLOWER;
		monst->leader = NULL;
	}
}

void moveAlly(creature *monst) {
	creature *target, *closestMonster = NULL;
	short x, y, dir, shortestDistance, targetLoc[2];
	char buf[DCOLS], monstName[DCOLS];
	
	x = monst->xLoc;
	y = monst->yLoc;
	
	targetLoc[0] = targetLoc[1] = 0;
	
	// if we're standing in harmful terrain and there is a way to escape it, spend this turn escaping it.
	if (cellHasTerrainFlag(x, y, (HARMFUL_TERRAIN & ~(IS_FIRE | CAUSES_DAMAGE)))
		|| (cellHasTerrainFlag(x, y, IS_FIRE) && !monst->status.immuneToFire)
		|| (cellHasTerrainFlag(x, y, CAUSES_DAMAGE) && !(monst->info.flags & MONST_INANIMATE))) {
		
		if (!rogue.updatedMapToSafeTerrainThisTurn) {
			updateSafeTerrainMap();
		}
		dir = nextStep(rogue.mapToSafeTerrain, x, y);
		if (dir != -1) {
			targetLoc[0] = x + nbDirs[dir][0];
			targetLoc[1] = y + nbDirs[dir][1];
			if (moveMonsterPassivelyTowards(monst, targetLoc, true)) {
				return;
			}
		}
	}
	
	// magic users sometimes cast spells
	if (monst->info.abilityFlags & MAGIC_ATTACK) {
		if (monstUseMagic(monst)) { // if he actually cast a spell
			monst->ticksUntilTurn = monst->attackSpeed * (monst->info.flags & MONST_CAST_SPELLS_SLOWLY ? 2 : 1);
			return;
		}
	}
	
	// weak allies seek safety
	
	// look around for enemies
	shortestDistance = max(DROWS, DCOLS);
	for (target = monsters->nextCreature; target != NULL; target = target->nextCreature) {
		if (target != monst
			&& (!(target->bookkeepingFlags & MONST_SUBMERGED) || (monst->bookkeepingFlags & MONST_SUBMERGED))
			&& monstersAreEnemies(target, monst)
			&& !(target->bookkeepingFlags & MONST_CAPTIVE)
			&& distanceBetween(x, y, target->xLoc, target->yLoc) < shortestDistance
			&& traversiblePathBetween(monst, target->xLoc, target->yLoc)
			&& (!monsterAvoids(monst, target->xLoc, target->yLoc) || (target->info.flags & MONST_ATTACKABLE_THRU_WALLS))
			&& (!(target->info.flags & MONST_INVISIBLE) || rand_percent(33))) {
			
			shortestDistance = distanceBetween(x, y, target->xLoc, target->yLoc);
			closestMonster = target;
		}
	}
	
	// weak allies in the presence of enemies seek safety
	if (closestMonster && distanceBetween(x, y, closestMonster->xLoc, closestMonster->yLoc) < 10
		&& (100 * monst->currentHP / monst->info.maxHP < 33)
		&& ((monst->info.flags & MONST_FLEES_NEAR_DEATH)
			|| (100 * monst->currentHP / monst->info.maxHP * 2 < 100 * player.currentHP / player.info.maxHP))) {
			
			// if there's an enemy in sight, and it's within 9 spaces, and the ally has under 33% health,
			// and either has less than half the health fraction that the player does, or flees near death
			
			if ((monst->info.abilityFlags & MA_CAST_BLINK)
				&& rand_percent(30)
				&& monsterBlinkToSafety(monst)) {
				return;
			}
			
			if (!rogue.updatedAllySafetyMapThisTurn) {
				updateAllySafetyMap();
			}
			
			dir = nextStep(allySafetyMap, monst->xLoc, monst->yLoc);
			
			//OOSCheck(dir, 4);
			
			if (dir != -1) {
				targetLoc[0] = x + nbDirs[dir][0];
				targetLoc[1] = y + nbDirs[dir][1];
			}
			
			if (dir == -1
				|| (allySafetyMap[targetLoc[0]][targetLoc[1]] >= allySafetyMap[x][y])
				|| (!moveMonster(monst, nbDirs[dir][0], nbDirs[dir][1]) && !moveMonsterPassivelyTowards(monst, targetLoc, true))) {
				// ally can't flee; continue below
			} else {
				return;
			}
		}
	
	if (closestMonster
		&& (distanceBetween(x, y, player.xLoc, player.yLoc) < 10 || (monst->bookkeepingFlags & MONST_DOES_NOT_TRACK_LEADER))
		&& !(monst->info.flags & MONST_MAINTAINS_DISTANCE)) {
		
		targetLoc[0] = closestMonster->xLoc;
		targetLoc[1] = closestMonster->yLoc;
		moveMonsterPassivelyTowards(monst, targetLoc, false);
	} else if (monst->targetCorpseLoc[0]) {
		moveMonsterPassivelyTowards(monst, monst->targetCorpseLoc, false);
		if (monst->xLoc == monst->targetCorpseLoc[0]
			&& monst->yLoc == monst->targetCorpseLoc[1]
			&& !(monst->bookkeepingFlags & MONST_ABSORBING)) {
			if (canSeeMonster(monst)) {
				monsterName(monstName, monst, true);
				sprintf(buf, "%s begins %s the fallen %s.", monstName, monsterText[monst->info.monsterID].absorbing, monst->targetCorpseName);
				message(buf, true, false);
			}
			monst->corpseAbsorptionCounter = 20;
			monst->bookkeepingFlags |= MONST_ABSORBING;
		}
	} else if ((monst->bookkeepingFlags & MONST_DOES_NOT_TRACK_LEADER)
			   || (distanceBetween(x, y, player.xLoc, player.yLoc) < 3 && (pmap[x][y].flags & IN_FIELD_OF_VIEW))) {
		if (rand_percent(30)) {
			dir = randValidDirectionFrom(monst, x, y, true);
			targetLoc[0] = x + nbDirs[dir][0];
			targetLoc[1] = y + nbDirs[dir][1];
			moveMonsterPassivelyTowards(monst, targetLoc, false);
		}
	} else {
		dir = scentDirection(monst);
		if (dir != -1) {
			targetLoc[0] = x + nbDirs[dir][0];
			targetLoc[1] = y + nbDirs[dir][1];
			moveMonsterPassivelyTowards(monst, targetLoc, false);
		} else {
			moveTowardLeader(monst);
		}
	}
}

void monstersTurn(creature *monst) {
	short x, y, playerLoc[2], targetLoc[2], dir, shortestDistance;
	boolean alreadyAtBestScent;
	char buf[COLS], buf2[COLS];
	creature *ally, *target, *closestMonster;
	
	monst->turnsSpentStationary++;
	
	if (monst->corpseAbsorptionCounter) {
		if (monst->xLoc == monst->targetCorpseLoc[0]
			&& monst->yLoc == monst->targetCorpseLoc[1]
			&& (monst->bookkeepingFlags & MONST_ABSORBING)) {
			if (!--monst->corpseAbsorptionCounter) {
				monst->targetCorpseLoc[0] = monst->targetCorpseLoc[1] = 0;
				if (monst->absorbBehavior) {
					monst->info.flags |= monst->absorptionFlags;
				} else {
					monst->info.abilityFlags |= monst->absorptionFlags;
				}
				monst->absorbsAllowed -= 2000;
				monst->bookkeepingFlags &= ~MONST_ABSORBING;
				
				if (monst->info.flags & MONST_FIERY) {
					monst->status.burning = monst->maxStatus.burning = 1000; // won't decrease
				}
				if (monst->info.flags & MONST_FLIES) {
					monst->status.levitating = monst->maxStatus.levitating = 1000; // won't decrease
				}
				if (monst->info.flags & MONST_IMMUNE_TO_FIRE) {
					monst->status.immuneToFire = monst->maxStatus.immuneToFire = 1000; // won't decrease
				}
				if (canSeeMonster(monst)) {
					monsterName(buf2, monst, true);
					sprintf(buf, "%s finished %s the %s.", buf2, monsterText[monst->info.monsterID].absorbing, monst->targetCorpseName);
					message(buf, true, false);
					sprintf(buf, "%s now %s!", buf2,
							(monst->absorbBehavior ? monsterBehaviorFlagDescriptions[unflag(monst->absorptionFlags)] :
							 monsterAbilityFlagDescriptions[unflag(monst->absorptionFlags)]));
					message(buf, true, true);
				}
				monst->absorptionFlags = 0;
			}
			monst->ticksUntilTurn = 100;
			return;
		} else if (!--monst->corpseAbsorptionCounter) {
			monst->targetCorpseLoc[0] = monst->targetCorpseLoc[1] = 0; // lost its chance
			monst->bookkeepingFlags &= ~MONST_ABSORBING;
		} else if (monst->bookkeepingFlags & MONST_ABSORBING) {
			monst->bookkeepingFlags &= ~MONST_ABSORBING; // absorbing but not on the corpse
			if (monst->corpseAbsorptionCounter <= 15) {
				monst->targetCorpseLoc[0] = monst->targetCorpseLoc[1] = 0; // lost its chance
			}
		}
	}
	
	// if the monster is paralyzed, entranced or chained, this is where its turn ends.
	if (monst->status.paralyzed || monst->status.entranced || monst->bookkeepingFlags & MONST_CAPTIVE) {
		monst->ticksUntilTurn = monst->movementSpeed;
		return;
	}
	
	monst->ticksUntilTurn = monst->movementSpeed / 3; // will be later overwritten by movement or attack
	
	x = monst->xLoc;
	y = monst->yLoc;
	
	if (monst->creatureState == MONSTER_SLEEPING) {
		monst->ticksUntilTurn = monst->movementSpeed;
		updateMonsterState(monst);
		return;
	}
	
	// Update creature state if appropriate
	updateMonsterState(monst);
	
	if (monst->creatureState == MONSTER_SLEEPING) {
		monst->ticksUntilTurn = monst->movementSpeed;
		return;
	}
	
	// and move the monster.
	
	// immobile monsters can only use special abilities:
	if (monst->info.flags & MONST_IMMOBILE) {
		if (monst->info.abilityFlags & MAGIC_ATTACK) {
			if (monstUseMagic(monst)) { // if he actually cast a spell
				monst->ticksUntilTurn = monst->attackSpeed * (monst->info.flags & MONST_CAST_SPELLS_SLOWLY ? 2 : 1);
				return;
			}
		}
		monst->ticksUntilTurn = monst->attackSpeed;
		return;
	}
	
	// imps sometimes teleport when frightened:
	if (monst->creatureState == MONSTER_FLEEING
		&& monst->info.flags & MONST_TELEPORTS && rand_percent(10)) {
		if (canSeeMonster(monst)) {
			monsterName(buf2, monst, true);
			sprintf(buf, "%s disappears in a flash of light!", buf2);
			message(buf, true, false);
		}
		teleport(monst);
		monst->creatureState = MONSTER_SLEEPING;
	}
	
	if (monst->status.discordant && monst->creatureState != MONSTER_FLEEING) {
		shortestDistance = max(DROWS, DCOLS);
		closestMonster = NULL;
		CYCLE_MONSTERS_AND_PLAYERS(target) {
			if (target != monst
				&& (!(target->bookkeepingFlags & MONST_SUBMERGED) || (monst->bookkeepingFlags & MONST_SUBMERGED))
				&& monstersAreEnemies(target, monst)
				&& !(target->bookkeepingFlags & MONST_CAPTIVE)
				&& distanceBetween(x, y, target->xLoc, target->yLoc) < shortestDistance
				&& traversiblePathBetween(monst, target->xLoc, target->yLoc)
				&& (!monsterAvoids(monst, target->xLoc, target->yLoc) || (target->info.flags & MONST_ATTACKABLE_THRU_WALLS))
				&& (!(target->info.flags & MONST_INVISIBLE) || rand_percent(33))) {
				
				shortestDistance = distanceBetween(x, y, target->xLoc, target->yLoc);
				closestMonster = target;
			}
		}
		if (closestMonster && !(monst->info.flags & MONST_MAINTAINS_DISTANCE)) {
			targetLoc[0] = closestMonster->xLoc;
			targetLoc[1] = closestMonster->yLoc;
			if (moveMonsterPassivelyTowards(monst, targetLoc, false)) {
				return;
			}
		}
	}
	
	if (monst->creatureState == MONSTER_TRACKING_SCENT
		|| (monst->creatureState == MONSTER_ALLY && monst->status.discordant)) {
		
		// magic users sometimes cast spells
		if (monst->info.abilityFlags & MAGIC_ATTACK) {
			if (monstUseMagic(monst)) { // if he actually cast a spell
				monst->ticksUntilTurn = monst->attackSpeed * (monst->info.flags & MONST_CAST_SPELLS_SLOWLY ? 2 : 1);
				return;
			}
		}
		
		// if the monster is adjacent to an ally and not adjacent to the player, attack the ally
		if (distanceBetween(x, y, player.xLoc, player.yLoc) > 1
			|| cellHasTerrainFlag(x, player.yLoc, OBSTRUCTS_PASSABILITY)
			|| cellHasTerrainFlag(player.xLoc, y, OBSTRUCTS_PASSABILITY)) {
			for (ally = monsters->nextCreature; ally != NULL; ally = ally->nextCreature) {
				if (monstersAreEnemies(monst, ally) && distanceBetween(x, y, ally->xLoc, ally->yLoc) == 1
					&& (!(ally->info.flags & MONST_INVISIBLE) || rand_percent(33))) {
					targetLoc[0] = ally->xLoc;
					targetLoc[1] = ally->yLoc;
					if (moveMonsterPassivelyTowards(monst, targetLoc, true)) {
						return;
					}
				}
			}
		}
		
		if ((monst->status.levitating || monst->info.flags & MONST_RESTRICTED_TO_LIQUID || monst->bookkeepingFlags & MONST_SUBMERGED
			 || (monst->info.flags & MONST_IMMUNE_TO_WEBS && monst->info.abilityFlags & MA_SHOOTS_WEBS))
			&& (distanceBetween(x, y, player.xLoc, player.yLoc) /* * rogue.stealthBonusMultiplier*/) / 100 < monst->info.sightRadius
			&& pmap[x][y].flags & IN_FIELD_OF_VIEW) {
			playerLoc[0] = player.xLoc;
			playerLoc[1] = player.yLoc;
			moveMonsterPassivelyTowards(monst, playerLoc, true);
			return;
		}
		if (tmap[x][y].scent == 0) {
			return;
		}
		
		dir = scentDirection(monst);
		if (dir == -1) {
			alreadyAtBestScent = true;
			for (dir = 0; dir < 8; dir++) {
				if (tmap[x + nbDirs[dir][0]][y + nbDirs[dir][1]].scent > tmap[x][y].scent) {
					alreadyAtBestScent = false;
					break;
				}
			}
			if (alreadyAtBestScent) {
				monst->creatureState = MONSTER_WANDERING;
				chooseNewWanderDestination(monst);
			}
		} else {
			moveMonster(monst, nbDirs[dir][0], nbDirs[dir][1]);
		}
		
	} else if (monst->creatureState == MONSTER_FLEEING) {
		if ((monst->info.abilityFlags & MA_CAST_BLINK)
			&& rand_percent(30)
			&& monsterBlinkToSafety(monst)) {
			return;
		}
		
		if (pmap[x][y].flags & IN_FIELD_OF_VIEW) {
			if (monst->safetyMap) {
				freeDynamicGrid(monst->safetyMap);
				monst->safetyMap = NULL;
			}
			if (!rogue.updatedSafetyMapThisTurn) {
				updateSafetyMap();
			}
			dir = nextStep(safetyMap, monst->xLoc, monst->yLoc);
		} else {
			if (!monst->safetyMap) {
				monst->safetyMap = allocDynamicGrid();
				copyDynamicGrid(monst->safetyMap, safetyMap);
			}
			dir = nextStep(monst->safetyMap, monst->xLoc, monst->yLoc);
		}
		if (dir != -1) {
			targetLoc[0] = x + nbDirs[dir][0];
			targetLoc[1] = y + nbDirs[dir][1];
		}
		if (dir == -1 || (!moveMonster(monst, nbDirs[dir][0], nbDirs[dir][1]) && !moveMonsterPassivelyTowards(monst, targetLoc, true))) {
			CYCLE_MONSTERS_AND_PLAYERS(ally) {
				if (monstersAreEnemies(monst, ally)
					&& distanceBetween(x, y, ally->xLoc, ally->yLoc) <= 1) {
					moveMonster(monst, ally->xLoc - x, ally->yLoc - y); // attack the player if cornered
					return;
				}
			}
		}
		return;
	} else if (monst->creatureState == MONSTER_WANDERING) {
		
		// if we're standing in harmful terrain and there is a way to escape it, spend this turn escaping it.
		if (cellHasTerrainFlag(x, y, (HARMFUL_TERRAIN & ~IS_FIRE))
			|| (cellHasTerrainFlag(x, y, IS_FIRE) && !monst->status.immuneToFire)) {
			if (!rogue.updatedMapToSafeTerrainThisTurn) {
				updateSafeTerrainMap();
			}
			dir = nextStep(rogue.mapToSafeTerrain, x, y);
			if (dir != -1) {
				targetLoc[0] = x + nbDirs[dir][0];
				targetLoc[1] = y + nbDirs[dir][1];
				if (moveMonsterPassivelyTowards(monst, targetLoc, true)) {
					return;
				}
			}
		}
		
		// if a captive leader is adjacent and captive and healthy enough to withstand an attack, then attack him.
		if (monst->bookkeepingFlags & MONST_FOLLOWER && monst->leader->bookkeepingFlags & MONST_CAPTIVE
			&& distanceBetween(monst->xLoc, monst->yLoc, monst->leader->xLoc, monst->leader->yLoc) == 1
			&& monst->leader->currentHP > monst->info.damage.upperBound
			&& !cellHasTerrainFlag(monst->xLoc, monst->leader->yLoc, OBSTRUCTS_PASSABILITY)
			&& !cellHasTerrainFlag(monst->leader->xLoc, monst->yLoc, OBSTRUCTS_PASSABILITY)) {
			attack(monst, monst->leader);
			monst->ticksUntilTurn = monst->attackSpeed;
			return;
		}
		
		// if the monster is adjacent to an ally and not fleeing, attack the ally
		if (monst->creatureState == MONSTER_WANDERING) {
			for (ally = monsters->nextCreature; ally != NULL; ally = ally->nextCreature) {
				if (monstersAreEnemies(monst, ally) && distanceBetween(x, y, ally->xLoc, ally->yLoc) == 1
					&& (!(ally->info.flags & MONST_INVISIBLE) || rand_percent(33))) {
					monst->creatureState = MONSTER_TRACKING_SCENT; // this alerts the monster that you're nearby
					targetLoc[0] = ally->xLoc;
					targetLoc[1] = ally->yLoc;
					if (moveMonsterPassivelyTowards(monst, targetLoc, true)) {
						return;
					}
				}
			}
		}
		
		// if you're a follower, don't get separated from the pack
		if ((monst->bookkeepingFlags & MONST_FOLLOWER) && distanceBetween(x, y, monst->leader->xLoc, monst->leader->yLoc) > 2) {
			moveTowardLeader(monst);
		} else {
			
			if (monst->xLoc == monst->destination[0][0] && monst->yLoc == monst->destination[0][1]) {
				// if he's already on destination waypoint #1
				if (monst->destination[1][0] > 0) {
					// if there is a waypoint #2 in the wings, bump it down to #1
					monst->destination[0][0] = monst->destination[1][0];
					monst->destination[0][1] = monst->destination[1][1];
					monst->destination[1][0] = 0;
					monst->destination[1][1] = 0;
				} else {
					// if no waypoint #2 in the wings, refill them both
					chooseNewWanderDestination(monst);
				}
			}
			
			// try to step towards the first destination waypoint
			
			// if monster landed on its destination square OR failed to move, choose a random door to walk towards
			// and otherwise forfeit the turn.
			
			if (!moveMonsterPassivelyTowards(monst, monst->destination[0], false)) {
				CYCLE_MONSTERS_AND_PLAYERS(ally) {
					if (monstersAreEnemies(monst, ally)
						&& distanceBetween(x, y, ally->xLoc, ally->yLoc) <= 1) {
						moveMonster(monst, ally->xLoc - x, ally->yLoc - y); // attack the player if cornered
						if (monst->creatureState == MONSTER_FLEEING) {
							chooseNewWanderDestination(monst); // try to find another way to flee for next turn
						}
						return;
					}
				}
				chooseNewWanderDestination(monst);
			}
			
		}
		
	} else if (monst->creatureState == MONSTER_ALLY) {
		moveAlly(monst);
		//OOSCheck(monst->xLoc, 4);
		//OOSCheck(monst->yLoc, 4);
	}
}

boolean canPass(creature *mover, creature *blocker) {
	return (!blocker->status.confused && !blocker->status.stuck && !blocker->status.paralyzed
			&& !(blocker->bookkeepingFlags & MONST_CAPTIVE) && !(blocker->info.flags & MONST_IMMOBILE)
			&& !monstersAreEnemies(mover, blocker)
			&& (blocker->leader == mover ||
				(mover->leader != blocker
				 && monstersAreTeammates(mover, blocker)
				 && blocker->ticksUntilTurn > rogue.ticksTillUpdateEnvironment)));
}

boolean isPassableOrSecretDoor(short x, short y) {
	return (!cellHasTerrainFlag(x, y, OBSTRUCTS_PASSABILITY) || cellHasTerrainFlag(x, y, IS_SECRET));
}

// Tries to move the given monster in the given vector; returns true if the move was legal
// (including attacking player, vomiting or struggling in vain)
// Be sure that dx, dy are both in the range [-1, 1] or the move will sometimes fail due to the diagonal check.
boolean moveMonster(creature *monst, short dx, short dy) {
	short x = monst->xLoc, y = monst->yLoc;
	short newX = x + dx, newY = y + dy;
	short confusedDirection;
	creature *defender = NULL;
	
	if (newX >= DCOLS || newX < 0 || newY >= DROWS || newY < 0) {
		//DEBUG printf("\nProblem! Monster trying to move more than one space at a time.");
		return false;
	}
	
	// vomiting
	if (monst->status.nauseous && rand_percent(25)) {
		vomit(monst);
		monst->ticksUntilTurn = monst->movementSpeed;
		return true;
	}
	
	// move randomly?
	if (!monst->status.entranced) {
		if (monst->status.confused) {
			confusedDirection = randValidDirectionFrom(monst, x, y, false);
			dx = nbDirs[confusedDirection][0];
			dy = nbDirs[confusedDirection][1];
		} else if ((monst->info.flags & MONST_FLITS) && !(monst->bookkeepingFlags & MONST_SEIZING) && rand_percent(33)) {
			confusedDirection = randValidDirectionFrom(monst, x, y, true);
			dx = nbDirs[confusedDirection][0];
			dy = nbDirs[confusedDirection][1];	
		}
	}
	
	newX = x + dx;
	newY = y + dy;
	
	// liquid monsters should never move or attack outside of liquid.
	if (monst->info.flags & MONST_RESTRICTED_TO_LIQUID && !cellHasTerrainFlag(newX, newY, (ALLOWS_SUBMERGING))) {
		return false;
	}
	
	// caught in spiderweb?
	if (monst->status.stuck && !(pmap[newX][newY].flags & (HAS_PLAYER | HAS_MONSTER))
		&& cellHasTerrainFlag(x, y, ENTANGLES) && !(monst->info.flags & MONST_IMMUNE_TO_WEBS)) {
		if (--monst->status.stuck) {
			monst->ticksUntilTurn = monst->movementSpeed;
			return true;
		} else if (tileCatalog[pmap[x][y].layers[SURFACE]].flags & ENTANGLES) {
			pmap[x][y].layers[SURFACE] = NOTHING;
		}
	}
	
	if (pmap[newX][newY].flags & (HAS_MONSTER | HAS_PLAYER)) {
		defender = monsterAtLoc(newX, newY);
	} else {
		if (monst->bookkeepingFlags & MONST_SEIZED) {
			for (defender = monsters->nextCreature; defender != NULL; defender = defender->nextCreature) {
				if (defender->bookkeepingFlags & MONST_SEIZING
					&& distanceBetween(monst->xLoc, monst->yLoc, defender->xLoc, defender->yLoc) == 1
					&& !monst->status.levitating) {
					monst->ticksUntilTurn = monst->movementSpeed;
					return true;
				}
			}
			monst->bookkeepingFlags &= ~MONST_SEIZED; // failsafe
		}
		if (monst->bookkeepingFlags & MONST_SEIZING) {
			monst->bookkeepingFlags &= ~MONST_SEIZING;
		}
	}
	
	if (((defender && (defender->info.flags & MONST_ATTACKABLE_THRU_WALLS))
		 || (isPassableOrSecretDoor(newX, newY)
			 && isPassableOrSecretDoor(newX, y)
			 && isPassableOrSecretDoor(x, newY)
			 && isPassableOrSecretDoor(x, y)))
		&& (!defender || monst->status.confused || monst->status.entranced ||
			canPass(monst, defender) || monstersAreEnemies(monst, defender))) {
			// if it's a legal move	
			if (defender) {
				if (pmap[newX][newY].flags & (HAS_PLAYER)) {
					defender = &player;
				} else { // attacking another monster!
					if (canPass(monst, defender)) {
						// swap places
						monst->xLoc = newX;
						monst->yLoc = newY;
						defender->xLoc = x;
						defender->yLoc = y;
						refreshDungeonCell(x, y);
						refreshDungeonCell(newX, newY);
						monst->ticksUntilTurn = monst->movementSpeed;
						return true;
					}
				}
				monst->ticksUntilTurn = monst->attackSpeed;
				if (!(monst->info.abilityFlags & MA_SEIZES && !(monst->bookkeepingFlags & MONST_SEIZING))) {
					monst->bookkeepingFlags &= ~MONST_SUBMERGED;
				}
				refreshDungeonCell(x, y);
				attack(monst, defender);
				return true;
			} else {
				// okay we're moving!
				monst->turnsSpentStationary = 0;
				monst->xLoc = newX;
				monst->yLoc = newY;
				pmap[x][y].flags &= ~HAS_MONSTER;
				pmap[newX][newY].flags |= HAS_MONSTER;
				if (monst->bookkeepingFlags & MONST_SUBMERGED && !cellHasTerrainFlag(newX, newY, ALLOWS_SUBMERGING)) {
					monst->bookkeepingFlags &= ~MONST_SUBMERGED;
				}
				if (playerCanSee(newX, newY)
					&& cellHasTerrainFlag(newX, newY, IS_SECRET) && cellHasTerrainFlag(newX, newY, OBSTRUCTS_PASSABILITY)) {
					discover(newX, newY); // if you see a monster use a secret door, you discover it
				}
				refreshDungeonCell(x, y);
				refreshDungeonCell(newX, newY);
				monst->ticksUntilTurn = monst->movementSpeed;
				applyInstantTileEffectsToCreature(monst);
				return true;
			}
		}
	return false;
}

void clearStatus(creature *monst) {
	monst->status.telepathic = monst->status.hallucinating = monst->status.levitating
	= monst->status.slowed = monst->status.hasted = monst->status.confused = monst->status.burning
	= monst->status.paralyzed = monst->status.poisoned = monst->status.stuck = monst->status.nauseous
	= monst->status.entersLevelIn = monst->status.magicalFear = monst->status.immuneToFire
	= monst->status.explosionImmunity = monst->status.entranced = monst->status.discordant = monst->status.darkness = 0;
}

boolean getQualifyingLocNear(short loc[2], short x, short y, short maxRadius,
							 unsigned long forbiddenTerrainFlags, unsigned long forbiddenMapFlags, boolean forbidLiquid) {
	short i, j, k, candidateLocs, randIndex;
	
	candidateLocs = 0;
	
	// count up the number of candidate locations
	for (k=0; k<maxRadius && !candidateLocs; k++) {
		for (i = x-k; i <= x+k; i++) {
			for (j = y-k; j <= y+k; j++) {
				if (coordinatesAreInMap(i, j)
					&& (i == x-k || i == x+k || j == y-k || j == y+k)
					&& !cellHasTerrainFlag(i, j, forbiddenTerrainFlags)
					&& !(pmap[i][j].flags & forbiddenMapFlags)
					&& (!forbidLiquid || pmap[i][j].layers[LIQUID] == NOTHING)) {
					candidateLocs++;
				}
			}
		}
	}
	
	if (candidateLocs == 0) {
		return false;
	}
	
	// and pick one
	randIndex = rand_range(1, candidateLocs);
	
	for (k=0; k<maxRadius; k++) {
		for (i = x-k; i <= x+k; i++) {
			for (j = y-k; j <= y+k; j++) {
				if (coordinatesAreInMap(i, j)
					&& (i == x-k || i == x+k || j == y-k || j == y+k)
					&& !cellHasTerrainFlag(i, j, forbiddenTerrainFlags)
					&& !(pmap[i][j].flags & forbiddenMapFlags)
					&& (!forbidLiquid || pmap[i][j].layers[LIQUID] == NOTHING)) {
					if (--randIndex == 0) {
						loc[0] = i;
						loc[1] = j;
						return true;
					}
				}
			}
		}
	}
	
	return false; // should never reach this point
}

void makeMonsterDropItem(creature *monst) {
	short loc[2];
	getQualifyingLocNear(loc, monst->xLoc, monst->yLoc, 5, (OBSTRUCTS_ITEMS), (HAS_ITEM), false);
	placeItem(monst->carriedItem, loc[0], loc[1]);
	monst->carriedItem = NULL;
	refreshDungeonCell(loc[0], loc[1]);
}

void demoteMonsterFromLeadership(creature *monst) {
	creature *follower, *nextFollower, *newLeader = NULL;
	
	monst->bookkeepingFlags &= ~MONST_LEADER;
	if (monst->mapToMe) {
		freeDynamicGrid(monst->mapToMe);
		monst->mapToMe = NULL;
	}
	for (follower = monsters->nextCreature; follower != NULL; follower = nextFollower) {
		nextFollower = follower->nextCreature;
		if (monstersAreTeammates(monst, follower) && monst != follower) {
			if (follower->bookkeepingFlags & MONST_BOUND_TO_LEADER) {
				// gonna die in playerTurnEnded().
				follower->leader = NULL;
				follower->bookkeepingFlags &= ~MONST_FOLLOWER;
			} else if (newLeader) {
				follower->leader = newLeader;
				follower->destination[0][0] = newLeader->destination[0][0];
				follower->destination[0][1] = newLeader->destination[0][1];
				follower->destination[1][0] = newLeader->destination[1][0];
				follower->destination[1][1] = newLeader->destination[1][1];
				follower->comingFrom[0] = newLeader->comingFrom[0];
				follower->comingFrom[1] = newLeader->comingFrom[1];
			} else {
				newLeader = follower;
				follower->bookkeepingFlags |= MONST_LEADER;
				follower->bookkeepingFlags &= ~MONST_FOLLOWER;
				follower->leader = NULL;
			}
		}
	}
}

void monsterDetails(char buf[], creature *monst) {
	char monstName[COLS], theItemName[COLS], newText[10*COLS];
	short i, j, combatMath, combatMath2, playerKnownAverageDamage, playerKnownMaxDamage, commaCount, realArmorValue;
	boolean anyFlags, printedDominationText = false;
	item *theItem;
	
	buf[0] = '\0';
	commaCount = 0;
	
	monsterName(monstName, monst, true);
	
	sprintf(newText, "     %s\n     ", monsterText[monst->info.monsterID].flavorText);
	upperCase(newText);
	strcat(buf, newText);
	
	if (!rogue.armor || (rogue.armor->flags & ITEM_IDENTIFIED)) {
		combatMath2 = hitProbability(monst, &player);
	} else {
		realArmorValue = player.info.defense;
		player.info.defense = (armorTable[rogue.armor->kind].range.upperBound + armorTable[rogue.armor->kind].range.lowerBound) / 2;
		combatMath2 = hitProbability(monst, &player);
		player.info.defense = realArmorValue;
	}
	
	if (monst->info.damage.upperBound == 0) {
		sprintf(newText, "%s deals no direct damage.\n     ", monstName);
	} else {
		combatMath = (player.currentHP + monst->info.damage.upperBound - 1) / monst->info.damage.upperBound;
		sprintf(newText, "%s has a %i%% chance to hit you, typically hits for %i%% of your maximum HP, and at worst, could defeat you in %i hit%s.\n     ",
				monstName,
				combatMath2,
				100 * (monst->info.damage.lowerBound + monst->info.damage.upperBound) / 2 / player.info.maxHP,
				combatMath,
				(combatMath > 1 ? "s" : ""));
	}
	upperCase(newText);
	strcat(buf, newText);
	
	if (monst->creatureState == MONSTER_ALLY) {
		sprintf(newText, "%s is your ally.", monstName);
	} else if (monst->bookkeepingFlags & MONST_CAPTIVE) {
		sprintf(newText, "%s is being held captive.", monstName);
	} else {
		
		if (!rogue.weapon || (rogue.weapon->flags & ITEM_IDENTIFIED)) {
			playerKnownAverageDamage = (player.info.damage.upperBound + player.info.damage.lowerBound) / 2;
			playerKnownMaxDamage = player.info.damage.upperBound;
		} else {
			playerKnownAverageDamage = (rogue.weapon->damage.upperBound + rogue.weapon->damage.lowerBound) / 2;
			playerKnownMaxDamage = rogue.weapon->damage.upperBound;
		}
		
		if (playerKnownMaxDamage == 0) {
			sprintf(newText, "You deal no direct damage.");
		} else {
			combatMath = (monst->currentHP + playerKnownMaxDamage - 1) / playerKnownMaxDamage;
			if (rogue.weapon && !(rogue.weapon->flags & ITEM_IDENTIFIED)) {
				realArmorValue = rogue.weapon->enchant1;
				rogue.weapon->enchant1 = 0;
				combatMath2 = hitProbability(&player, monst);
				rogue.weapon->enchant1 = realArmorValue;
			} else {
				combatMath2 = hitProbability(&player, monst);
			}
			sprintf(newText, "You have a %i%% chance to hit %s, typically hit for %i%% of its maximum HP, and at best, could defeat it in %i hit%s.",
					combatMath2,
					monstName,
					100 * playerKnownAverageDamage / monst->info.maxHP,
					combatMath,
					(combatMath > 1 ? "s" : ""));
		}
	}
	upperCase(newText);
	strcat(buf, newText);
	
	anyFlags = false;
	for (theItem = packItems->nextItem; theItem != NULL; theItem = theItem->nextItem) {
		itemName(theItem, theItemName, false, false);
		
		if (theItem->category == STAFF
			&& (theItem->flags & (ITEM_MAX_CHARGES_KNOWN | ITEM_IDENTIFIED))
			&& (staffTable[theItem->kind].identified)) {
			switch (theItem->kind) {
				case STAFF_FIRE:
				case STAFF_LIGHTNING:
					sprintf(newText, "\n     Your %s (%c) will hit %s for between %i%% and %i%% of its maximum HP.",
							theItemName,
							theItem->inventoryLetter,
							monstName,
							100 * staffDamageLow(theItem->enchant1) / monst->info.maxHP,
							100 * staffDamageHigh(theItem->enchant1) / monst->info.maxHP);
					strcat(buf, newText);
					anyFlags = true;
					break;
				case STAFF_POISON:
					sprintf(newText, "\n     Your %s (%c) will poison %s for %i%% of its maximum HP.",
							theItemName,
							theItem->inventoryLetter,
							monstName,
							100 * staffPoison(theItem->enchant1) / monst->info.maxHP);
					strcat(buf, newText);
					anyFlags = true;
					break;
				default:
					break;
			}
		} else if (theItem->category == WAND
				   && theItem->kind == WAND_DOMINATION
				   && wandTable[WAND_DOMINATION].identified
				   && !printedDominationText) {
			printedDominationText = true;
			sprintf(newText, "\n     A wand of domination will have a %i%% chance of success at %s's current health level.",
					wandDominate(monst),
					monstName);
			strcat(buf, newText);
			anyFlags = true;
		}
	}
	if (anyFlags) {
		strcat(buf, "\n     ");
	}
	
	anyFlags = false;
	sprintf(newText, "%s ", monstName);
	upperCase(newText);
	
	if (monst->attackSpeed < 100) {
		strcat(newText, "attacks quickly");
		anyFlags = true;
	} else if (monst->attackSpeed > 100) {
		strcat(newText, "attacks slowly");
		anyFlags = true;
	}
	
	if (monst->movementSpeed < 100) {
		if (anyFlags) {
			strcat(newText, "& ");
			commaCount++;
		}
		strcat(newText, "moves quickly");
		anyFlags = true;
	} else if (monst->movementSpeed > 100) {
		if (anyFlags) {
			strcat(newText, "& ");
			commaCount++;
		}
		strcat(newText, "moves slowly");
		anyFlags = true;
	}
	
	if (monst->info.turnsBetweenRegen == 0) {
		if (anyFlags) {
			strcat(newText, "& ");
			commaCount++;
		}
		strcat(newText, "does not regenerate");
		anyFlags = true;
	} else if (monst->info.turnsBetweenRegen < 5000) {
		if (anyFlags) {
			strcat(newText, "& ");
			commaCount++;
		}
		strcat(newText, "regenerates quickly");
		anyFlags = true;
	}
	
	// ability flags
	for (i=0; i<32; i++) {
		if ((monst->info.abilityFlags & (1 << i))
			&& monsterAbilityFlagDescriptions[i][0]) {
			if (anyFlags) {
				strcat(newText, "& ");
				commaCount++;
			}
			strcat(newText, monsterAbilityFlagDescriptions[i]);
			anyFlags = true;
		}
	}
	
	// behavior flags
	for (i=0; i<32; i++) {
		if ((monst->info.flags & (1 << i))
			&& monsterBehaviorFlagDescriptions[i][0]) {
			if (anyFlags) {
				strcat(newText, "& ");
				commaCount++;
			}
			strcat(newText, monsterBehaviorFlagDescriptions[i]);
			anyFlags = true;
		}
	}
	
	// bookkeeping flags
	for (i=0; i<32; i++) {
		if ((monst->bookkeepingFlags & (1 << i))
			&& monsterBookkeepingFlagDescriptions[i][0]) {
			if (anyFlags) {
				strcat(newText, "& ");
				commaCount++;
			}
			strcat(newText, monsterBookkeepingFlagDescriptions[i]);
			anyFlags = true;
		}
	}
	
	if (anyFlags) {
		strcat(newText, ".");
		//strcat(buf, "\n\n");
		j = strlen(buf);
		for (i=0; newText[i] != '\0'; i++) {
			if (newText[i] == '&') {
				if (!--commaCount) {
					buf[j] = '\0';
					strcat(buf, " and");
					j += 4;
				} else {
					buf[j++] = ',';
				}
			} else {
				buf[j++] = newText[i];
			}
		}
		buf[j] = '\0';
	}
}
