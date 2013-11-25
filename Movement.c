/*
 *  Movement.c
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
#include <math.h>

void checkForDungeonErrors() {
	short i, j;
	extern color black, yellow;
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			if (pmap[i][j].layers[DUNGEON] > TORCH_WALL || pmap[i][j].layers[DUNGEON] < 0) {
				plotCharWithColor('&', i + STAT_BAR_WIDTH, j + MESSAGE_LINES, black, yellow);
				printf("\n\nERROR at (%i, %i)!!", i, j);
			}
		}
	}
}

void playerRuns(short direction) {
	short i, tempTile, newX, newY;
	boolean cardinalPassability[4];
	
	rogue.disturbed = (player.status.confused ? true : false);
	
	for (i = 0; i < 4; i++) {
		tempTile = pmap[player.xLoc + nbDirs[i][0]][player.yLoc + nbDirs[i][1]].layers[DUNGEON];
		cardinalPassability[i] = (tileCatalog[tempTile].flags & OBSTRUCTS_PASSABILITY ? -1 : 
								  (tileCatalog[tempTile].flags & IS_SECRET ? FLOOR : tempTile));
	}
	
	while (!rogue.disturbed) {
		if (!playerMoves(direction)) {
			rogue.disturbed = true;
			break;
		}
		
		newX = player.xLoc + nbDirs[direction][0];
		newY = player.yLoc + nbDirs[direction][1];
		if (/*pmap[newX][newY].layers[LIQUID] != liquidType ||*/ monsterAvoids(&player, newX, newY)) {
			rogue.disturbed = true;
		}		
		if (isDisturbed(player.xLoc, player.yLoc)) {
			rogue.disturbed = true;
		} else if (direction < 4) {
			for (i = 0; i < 4; i++) {
				tempTile = pmap[player.xLoc + nbDirs[i][0]][player.yLoc + nbDirs[i][1]].layers[DUNGEON];
				if (cardinalPassability[i] !=
					((tileCatalog[tempTile].flags & OBSTRUCTS_PASSABILITY ? -1 : 
					  (tileCatalog[tempTile].flags & IS_SECRET ? FLOOR : tempTile))) && 
					// i is not the x-opposite or y-opposite of direction
					!(nbDirs[i][0] + nbDirs[direction][0] == 0 &&
					  nbDirs[i][1] + nbDirs[direction][1] == 0)) {
					rogue.disturbed = true;
				}
			}
		}
	}
	updateFlavorText();
}

enum dungeonLayers highestPriorityLayer(short x, short y, boolean skipGas) {
	short bestPriority = 10000;
	enum dungeonLayers tt, best;
	
	for (tt = 0; tt < NUMBER_TERRAIN_LAYERS; tt++) {
		if (tt == GAS && skipGas) {
			continue;
		}
		if (pmap[x][y].layers[tt] && tileCatalog[pmap[x][y].layers[tt]].drawPriority < bestPriority) {
			bestPriority = tileCatalog[pmap[x][y].layers[tt]].drawPriority;
			best = tt;
		}
	}
	return best;
}

short layerWithFlag(short x, short y, unsigned long flag) {
	short layer;
	
	for (layer = 0; layer < NUMBER_TERRAIN_LAYERS; layer++) {
		if (tileCatalog[pmap[x][y].layers[layer]].flags & flag) {
			return layer;
		}
	}
	return -1;
}

// Retrieves a pointer to the flavor text of the highest-priority terrain at the given location
char *tileFlavor(short x, short y) {
	return tileCatalog[pmap[x][y].layers[highestPriorityLayer(x, y, false)]].flavorText;
}

// Retrieves a pointer to the description text of the highest-priority terrain at the given location
char *tileText(short x, short y) {
	return tileCatalog[pmap[x][y].layers[highestPriorityLayer(x, y, false)]].description;
}

void describedItemCategory(short theCategory, char *buf) {
	unsigned short itemCats[9] = {FOOD, WEAPON, ARMOR, POTION, SCROLL, STAFF, WAND, RING, GOLD};
	if (player.status.hallucinating) {
		theCategory = itemCats[rand_range(0, 8)];
	}
	switch (theCategory) {
		case FOOD:
			strcpy(buf, "food");
			break;
		case WEAPON:
			strcpy(buf, "a weapon");
			break;
		case ARMOR:
			strcpy(buf, "a suit of armor");
			break;
		case POTION:
			strcpy(buf, "a potion");
			break;
		case SCROLL:
			strcpy(buf, "a scroll");
			break;
		case STAFF:
			strcpy(buf, "a staff");
			break;
		case WAND:
			strcpy(buf, "a wand");
			break;
		case RING:
			strcpy(buf, "a ring");
			break;
		case AMULET:
			strcpy(buf, "the Amulet of Yendor");
			break;
		case GEM:
			strcpy(buf, "a lumenstone");
			break;
		case GOLD:
			strcpy(buf, "a pile of gold");
			break;
		default:
			strcpy(buf, "something strange");
			break;
	}
}

// Describes the item in question either by naming it if the player has already seen its name,
// or by tersely identifying its category otherwise.
void describedItemName(item *theItem, char *buf) {
	if (!player.status.hallucinating
		&& ((theItem->flags & ITEM_NAMED)
		|| (theItem->category & (GOLD | AMULET | GEM)))) {
		itemName(theItem, buf, (theItem->category & (WEAPON | ARMOR) ? false : true), true);
	} else {
		describedItemCategory(theItem->category, buf);
	}
}

void describeLocation(char *buf, short x, short y) {
	creature *monst;
	item *theItem, *magicItem;
	boolean standsInTerrain;
	boolean subjectMoving;
	boolean prepositionLocked = false;
	
	char subject[DCOLS];
	char verb[DCOLS];
	char preposition[DCOLS];
	char object[DCOLS];
	
	if (x == player.xLoc && y == player.yLoc) {
		if (player.status.levitating) {
			sprintf(buf, "you are hovering above %s.", tileText(x, y));
		} else {
			strcpy(buf, tileFlavor(x, y));
		}
		return;
	}
	
	standsInTerrain = ((tileCatalog[pmap[x][y].layers[highestPriorityLayer(x, y, false)]].flags & STAND_IN_TILE) ? true : false);
	theItem = itemAtLoc(x, y);
	monst = monsterAtLoc(x, y);
	
	// detecting magical items
	magicItem = NULL;
	if (theItem && !(pmap[x][y].flags & (VISIBLE | CLAIRVOYANT_VISIBLE))
		&& (theItem->flags & ITEM_MAGIC_DETECTED)
		&& itemMagicChar(theItem)) {
		magicItem = theItem;
	} else if (monst && !canSeeMonster(monst)
			   && monst->carriedItem
			   && (monst->carriedItem->flags & ITEM_MAGIC_DETECTED)
			   && itemMagicChar(magicItem)) {
		magicItem = monst->carriedItem;
	}
	if (magicItem) {
		switch (itemMagicChar(magicItem)) {
			case GOOD_MAGIC_CHAR:
				strcpy(object, "benevolent magic");
				break;
			case BAD_MAGIC_CHAR:
				strcpy(object, "malevolent magic");
				break;
			case AMULET_CHAR:
				strcpy(object, "the Amulet of Yendor");
				break;
			default:
				strcpy(object, "mysterious magic");
				break;
		}
		sprintf(buf, "you can detect the aura of %s here.", object);
		return;
	}
	
	// telepathy
	if (monst && !canSeeMonster(monst) && player.status.telepathic) {
		sprintf(buf, "you can sense a %s psychic emanation here.",
				((!player.status.hallucinating && monst->info.displayChar >= 'a' && monst->info.displayChar <= 'z')
				 || (player.status.hallucinating && rand_range(0, 1)) ? "small" : "large"));
		return;
	}
	
	if (monst && !canSeeMonster(monst)	// monster is not visible
										// and not invisible but outlined in a gas cloud
		&& (!pmap[x][y].layers[GAS] || !(monst->info.flags & MONST_INVISIBLE) || !(pmap[x][y].flags & VISIBLE | CLAIRVOYANT_VISIBLE))) {
		monst = NULL;
	}
	
	if (!(pmap[x][y].flags & (VISIBLE | CLAIRVOYANT_VISIBLE))) {
		if (pmap[x][y].flags & DISCOVERED) { // memory
			if (pmap[x][y].rememberedItemCategory) {
				describedItemCategory(pmap[x][y].rememberedItemCategory, object);
			} else {
				strcpy(object, tileCatalog[pmap[x][y].rememberedTerrain].description);
			}
			sprintf(buf, "you remember seeing %s here.", object);
			return;
		} else if (pmap[x][y].flags & MAGIC_MAPPED) { // magic mapped
			sprintf(buf, "you expect %s to be here.", tileCatalog[pmap[x][y].rememberedTerrain].description);
			return;
		}
		strcpy(buf, "");
		return;
	}
	
	if (monst) {
		
		monsterName(subject, monst, true);
		
		if (pmap[x][y].layers[GAS] && (monst->info.flags & MONST_INVISIBLE)) { // phantoms in gas
			sprintf(buf, "you can perceive the faint outline of %s in %s.", subject, tileCatalog[pmap[x][y].layers[GAS]].description);
			return;
		}
		
		subjectMoving = (monst->turnsSpentStationary == 0);
		
		if (cellHasTerrainFlag(x, y, OBSTRUCTS_PASSABILITY)) {
			strcpy(verb, "is embedded");
			subjectMoving = false;
		} else if (monst->bookkeepingFlags & MONST_CAPTIVE) {
			strcpy(verb, "is shackled in place");
			subjectMoving = false;
		} else if (monst->status.paralyzed) {
			strcpy(verb, "is frozen in place");
			subjectMoving = false;
		} else if (monst->status.stuck) {
			strcpy(verb, "is entangled");
			subjectMoving = false;
		} else if (monst->status.levitating) {
			strcpy(verb, (subjectMoving ? "is flying" : "is hovering"));
			strcpy(preposition, "over");
			prepositionLocked = true;
		} else if (cellHasTerrainFlag(x, y, ALLOWS_SUBMERGING) && (monst->info.flags & MONST_SUBMERGES)) {
			strcpy(verb, (subjectMoving ? "is gliding" : "is drifting"));
		} else if (cellHasTerrainFlag(x, y, MOVES_ITEMS) && !(monst->info.flags & MONST_SUBMERGES)) {
			strcpy(verb, (subjectMoving ? "is swimming" : "is struggling"));
		} else if (cellHasTerrainFlag(x, y, TRAP_DESCENT)) {
			strcpy(verb, "is suspended in mid-air");
			strcpy(preposition, "over");
			prepositionLocked = true;
			subjectMoving = false;
		} else if (monst->status.confused) {
			strcpy(verb, "is staggering");
		} else {
			switch (monst->creatureState) {
				case MONSTER_SLEEPING:
					strcpy(verb, "is sleeping");
					subjectMoving = false;
					break;
				case MONSTER_WANDERING:
					strcpy(verb, subjectMoving ? "is wandering" : "is standing");
					break;
				case MONSTER_FLEEING:
					strcpy(verb, subjectMoving ? "is fleeing" : "is standing");
					break;
				case MONSTER_TRACKING_SCENT:
					strcpy(verb, subjectMoving ? "is moving" : "is standing");
					break;
				case MONSTER_ALLY:
					strcpy(verb, subjectMoving ? "is following you" : "is standing");
					break;
				default:
					strcpy(verb, "is standing");
					break;
			}
		}
		if (monst->status.burning && !(monst->info.flags & MONST_FIERY)) {
			strcat(verb, ", burning,");
		}
		
		if (theItem) {
			strcpy(preposition, "over");
			describedItemName(theItem, object);
		} else {
			if (!prepositionLocked) {
				strcpy(preposition, subjectMoving ? (standsInTerrain ? "through" : "across")
					   : (standsInTerrain ? "in" : "on"));
			}
			
			strcpy(object, tileText(x, y));
			
		}
	} else { // no monster
		strcpy(object, tileText(x, y));
		if (theItem) {
			describedItemName(theItem, subject);
			subjectMoving = cellHasTerrainFlag(x, y, MOVES_ITEMS);
			
			strcpy(verb, ((theItem->quantity > 1
						   && (theItem->flags & ITEM_NAMED))
						  || (theItem->category & (GOLD | AMULET | GEM))) ? "are" : "is");
			if (cellHasTerrainFlag(x, y, OBSTRUCTS_PASSABILITY)) {
				strcat(verb, " embedded");
			} else {
				strcat(verb, subjectMoving ? " drifting" : " lying");
			}
			strcpy(preposition, standsInTerrain ? (subjectMoving ? "through" : "in")
				   : (subjectMoving ? "across" : "on"));
			

		} else { // no item
			sprintf(buf, "you %s %s.", ((pmap[x][y].flags & VISIBLE) ? "see" : "sense"), object);
			return;
		}
	}
	
	sprintf(buf, "%s %s %s %s.", subject, verb, preposition, object);
}

void printLocationDescription(short x, short y) {
	char buf[DCOLS*3];
	describeLocation(buf, x, y);
	message(buf, false, false);
}

void exposeCreatureToFire(creature *monst) {
	char buf[COLS], buf2[COLS];
	if (monst->info.flags & MONST_IMMUNE_TO_FIRE
		|| monst->bookkeepingFlags & MONST_SUBMERGED
		|| ((!monst->status.levitating) && cellHasTerrainFlag(monst->xLoc, monst->yLoc, EXTINGUISHES_FIRE))) {
		return;
	}
	if (monst->status.burning == 0) {
		if (monst == &player) {
			rogue.minersLight->lightColor = &fireForeColor;
			player.info.foreColor = &torchLightColor;
			refreshDungeonCell(player.xLoc, player.yLoc);
			updateVision(); // remember that this is here
			message("you catch fire!", true, true);
		} else if (canSeeMonster(monst)) {
			if (monst->statusLight) {
				deleteLight(monst->statusLight);
			}
			monst->statusLight = newLight(&lightCatalog[BURNING_CREATURE_LIGHT], 0, 0, monst);
			monsterName(buf, monst, true);
			sprintf(buf2, "%s catches fire!", buf);
			message(buf2, true, false);
		}
	}
	monst->status.burning = monst->maxStatus.burning = max(monst->status.burning, 12);
}

void updateFlavorText() {
	if (!(player.status.levitating) && rogue.disturbed && !rogue.gameHasEnded) {
		message(tileFlavor(player.xLoc, player.yLoc), false, false);
	}
}

// Called every 100 ticks; may be called more frequently.
void applyInstantTileEffectsToCreature(creature *monst) {
	char buf[COLS], buf2[COLS];
	short *x = &(monst->xLoc), *y = &(monst->yLoc), damage;
	enum dungeonLayers layer;
	creature *previousCreature;
	
	if (monst == &player && !player.status.levitating) {
		pmap[*x][*y].flags |= PLAYER_STEPPED_HERE;
	}
	
	if (monst == &player && !(monst->status.levitating) && cellHasTerrainFlag(*x, *y, IS_SECRET)
		&& pmap[*x][*y].flags & (VISIBLE | CLAIRVOYANT_VISIBLE)) {
		
		discover(*x, *y);
	}
	
	if (monst == &player) {
		if (rogue.inWater) {
			if (!cellHasTerrainFlag(player.xLoc, player.yLoc, IS_DEEP_WATER) || player.status.levitating
				|| cellHasTerrainFlag(player.xLoc, player.yLoc, ENTANGLES)
				|| cellHasTerrainFlag(player.xLoc, player.yLoc, OBSTRUCTS_PASSABILITY)) {	
				rogue.inWater = false;
				updateMinersLightRadius();
				updateVision();
				displayLevel();
			}
		} else {
			if (cellHasTerrainFlag(player.xLoc, player.yLoc, IS_DEEP_WATER) && !player.status.levitating
				&& !cellHasTerrainFlag(player.xLoc, player.yLoc, ENTANGLES)
				&& !cellHasTerrainFlag(player.xLoc, player.yLoc, OBSTRUCTS_PASSABILITY)) {	
				rogue.inWater = true;
				updateMinersLightRadius();
				updateVision();
				displayLevel();
			}
		}
	}
	
	if (!(monst->status.levitating) && !(monst->info.flags & MONST_IMMUNE_TO_FIRE)
		&& !cellHasTerrainFlag(*x, *y, ENTANGLES)
		&& !cellHasTerrainFlag(*x, *y, OBSTRUCTS_PASSABILITY)
		&& cellHasTerrainFlag(*x, *y, LAVA_INSTA_DEATH)) {
		
		if (monst == &player) {
			sprintf(buf, "you plunge into %s!",
					tileCatalog[pmap[*x][*y].layers[layerWithFlag(*x, *y, LAVA_INSTA_DEATH)]].description);
			message(buf, true, true);
			sprintf(buf, "Killed by %s",
					tileCatalog[pmap[*x][*y].layers[layerWithFlag(*x, *y, LAVA_INSTA_DEATH)]].description);
			gameOver(buf, true);
			return;
		} else { // it's a monster
			if (canSeeMonster(monst)) {
				monsterName(buf, monst, true);
				sprintf(buf2, "%s is consumed by %s instantly!", buf,
						tileCatalog[pmap[*x][*y].layers[layerWithFlag(*x, *y, LAVA_INSTA_DEATH)]].description);
				message(buf2, true, false);
			}
			addExperience(monst->info.expForKilling);
			killCreature(monst);
			refreshDungeonCell(*x, *y);
			return;
		}
	}
	
	if (cellHasTerrainFlag(*x, *y, EXTINGUISHES_FIRE) && monst->status.burning && !monst->status.levitating
		&& !(monst->info.flags & MONST_FIERY)) {
		extinguishFireOnCreature(monst);
	}
	
	if (pmap[*x][*y].flags & (VISIBLE | CLAIRVOYANT_VISIBLE) && cellHasTerrainFlag(*x, *y, IS_SECRET)
		&& (cellHasTerrainFlag(*x, *y, OBSTRUCTS_PASSABILITY) || !monst->status.levitating)) {
		discover(*x, *y); // if you see a monster use a secret door, you discover it
	}
	
	if (!(monst->status.levitating) && cellHasTerrainFlag(*x, *y, TRAP_DESCENT)
		&& !cellHasTerrainFlag(*x, *y, ENTANGLES)
		&& !cellHasTerrainFlag(*x, *y, OBSTRUCTS_PASSABILITY)
		&& !(monst->bookkeepingFlags & MONST_PREPLACED)) {
		if (monst == &player) {
			rogue.disturbed = true;
			message(tileFlavor(player.xLoc, player.yLoc), true, true);
			rogue.depthLevel++;
			startLevel(rogue.depthLevel - 1, 0);
			damage = randClumpedRange(6, 12, 2);
			message("You are damaged by the fall.", true, false);
			if (inflictDamage(NULL, &player, damage, &red)) {
				gameOver("Killed by a fall", true);
			}
			return;
		} else { // it's a monster
			if (canSeeMonster(monst)) {
				monsterName(buf, monst, true);
				sprintf(buf2, "%s plunges out of sight!", buf);
				message(buf2, true, false);
			}
			monst->bookkeepingFlags |= MONST_PREPLACED;
			
			if (!inflictDamage(NULL, monst, randClumpedRange(6, 12, 2), &red)) {
				// remove from monster chain
				for (previousCreature = monsters;
					 previousCreature->nextCreature != monst;
					 previousCreature = previousCreature->nextCreature);
				previousCreature->nextCreature = monst->nextCreature;
				
				// add to next level's chain
				monst->nextCreature = levels[rogue.depthLevel-1 + 1].monsters;
				levels[rogue.depthLevel-1 + 1].monsters = monst;
			}
			pmap[*x][*y].flags &= ~HAS_MONSTER;
			
			refreshDungeonCell(*x, *y);
			
			return;
		}
	}
	
	if (!(monst->status.levitating) && cellHasTerrainFlag(*x, *y, IS_DF_TRAP)
		&& !(pmap[*x][*y].flags & PRESSURE_PLATE_DEPRESSED)) {
		pmap[*x][*y].flags |= PRESSURE_PLATE_DEPRESSED;
		if (pmap[*x][*y].flags & (VISIBLE | CLAIRVOYANT_VISIBLE) && cellHasTerrainFlag(*x, *y, IS_SECRET)) {
			discover(*x, *y);
			refreshDungeonCell(*x, *y);
		}
		if (canSeeMonster(monst)) {
			monsterName(buf, monst, true);
			sprintf(buf2, "a hidden pressure plate under %s clicks!", buf);
			message(buf2, true, true);
		} else if (pmap[*x][*y].flags & (VISIBLE | CLAIRVOYANT_VISIBLE)) {
			// usually means an invisible monster
			message("a hidden pressure plate clicks!", true, false);
		}
		for (layer = 0; layer < NUMBER_TERRAIN_LAYERS; layer++) {
			if (tileCatalog[pmap[*x][*y].layers[layer]].flags & IS_DF_TRAP) {
				spawnDungeonFeature(*x, *y, &(dungeonFeatureCatalog[tileCatalog[pmap[*x][*y].layers[layer]].fireType]), true);
				break;
			}
		}
	}
	
	if (cellHasTerrainFlag(*x, *y, PROMOTES_ON_STEP)) { // flying creatures activate too
		// Because this uses no pressure plate, it will trigger every time this function is called while
		// the monster or player is on the tile. Because this function can be called several times per
		// turn, multiple promotions can happen unpredictably if the tile does not promote to a tile without
		// the PROMOTES_ON_STEP attribute.
		for (layer = 0; layer < NUMBER_TERRAIN_LAYERS; layer++) {
			if (tileCatalog[pmap[*x][*y].layers[layer]].flags & PROMOTES_ON_STEP) {
				promoteTile(*x, *y, layer, false);
			}
		}
	}
	
	if (cellHasTerrainFlag(*x, *y, ENTANGLES) && !monst->status.stuck
		&& !(monst->info.flags & MONST_IMMUNE_TO_WEBS) && !(monst->bookkeepingFlags & MONST_SUBMERGED)) {
		monst->status.stuck = monst->maxStatus.stuck = rand_range(3, 7);
		if (monst == &player) {
			sprintf(buf2, "you are stuck fast in %s!",
					tileCatalog[pmap[*x][*y].layers[layerWithFlag(*x, *y, ENTANGLES)]].description);
			message(buf2, true, false);
		} else if (canSeeMonster(monst)) { // it's a monster
			monsterName(buf, monst, true);
			sprintf(buf2, "%s is stuck fast in %s!", buf,
					tileCatalog[pmap[*x][*y].layers[layerWithFlag(*x, *y, ENTANGLES)]].description);
			message(buf2, true, false);
		}
	}
	
	if (cellHasTerrainFlag(*x, *y, CAUSES_EXPLOSIVE_DAMAGE) && !monst->status.explosionImmunity
		&& !(monst->bookkeepingFlags & MONST_SUBMERGED)) {
		damage = rand_range(15, 20);
		damage = max(damage, monst->info.maxHP / 2);
		monst->status.explosionImmunity = 5;
		if (monst == &player) {
			rogue.disturbed = true;
			for (layer = 0; layer < NUMBER_TERRAIN_LAYERS && !(tileCatalog[pmap[*x][*y].layers[layer]].flags & CAUSES_EXPLOSIVE_DAMAGE); layer++);
			message(tileCatalog[pmap[*x][*y].layers[layer]].flavorText, true, false);
			if (inflictDamage(NULL, &player, damage, &yellow)) {
				strcpy(buf2, tileCatalog[pmap[*x][*y].layers[layerWithFlag(*x, *y, CAUSES_EXPLOSIVE_DAMAGE)]].description);
				sprintf(buf, "Killed by %s", buf2);
				gameOver(buf, true);
				return;
			}
		} else { // it's a monster
			if (monst->creatureState == MONSTER_SLEEPING) {
				monst->creatureState = MONSTER_TRACKING_SCENT;
			}
			monsterName(buf, monst, true);
			if (inflictDamage(NULL, monst, damage, &yellow)) {
				// if killed
				sprintf(buf2, "%s dies in %s.", buf,
						tileCatalog[pmap[*x][*y].layers[layerWithFlag(*x, *y, CAUSES_EXPLOSIVE_DAMAGE)]].description);
				message(buf2, true, false);
				refreshDungeonCell(*x, *y);
				return;
			} else {
				// if survived
				sprintf(buf2, "%s engulfs %s.",
						tileCatalog[pmap[*x][*y].layers[layerWithFlag(*x, *y, CAUSES_EXPLOSIVE_DAMAGE)]].description, buf);
				message(buf2, true, false);
			}
		}
	}
	
	if (cellHasTerrainFlag(*x, *y, CAUSES_NAUSEA) && !(monst->info.flags & MONST_INANIMATE) && !(monst->bookkeepingFlags & MONST_SUBMERGED)) {
		if (monst == &player) {
			rogue.disturbed = true;
		}
		if (canSeeMonster(monst) && !(monst->status.nauseous)) {
			if (monst->creatureState == MONSTER_SLEEPING) {
				monst->creatureState = MONSTER_TRACKING_SCENT;
			}
			flashMonster(monst, &brown, 100);
			monsterName(buf, monst, true);
			sprintf(buf2, "%s choke%s and gag%s on the stench of rot.", buf,
					(monst == &player ? "": "s"), (monst == &player ? "": "s"));
			message(buf2, true, false);
		}
		monst->status.nauseous = monst->maxStatus.nauseous = max(monst->status.nauseous, 20);
	}
	
	if (cellHasTerrainFlag(*x, *y, CAUSES_POISON) && !(monst->info.flags & MONST_INANIMATE) && !monst->status.levitating) {
		if (monst == &player && !player.status.poisoned) {
			rogue.disturbed = true;
		}
		if (canSeeMonster(monst) && !(monst->status.poisoned)) {
			if (monst->creatureState == MONSTER_SLEEPING) {
				monst->creatureState = MONSTER_TRACKING_SCENT;
			}
			flashMonster(monst, &green, 100);
			monsterName(buf, monst, true);
			sprintf(buf2, "the lichen's grasping tendrils poison %s.", buf);
			message(buf2, true, false);
		}
		monst->status.poisoned = monst->maxStatus.poisoned = max(monst->status.poisoned, 10);
	}
	
	if (cellHasTerrainFlag(*x, *y, CAUSES_CONFUSION) && !(monst->info.flags & MONST_INANIMATE)) {
		if (monst == &player) {
			rogue.disturbed = true;
		}
		if (canSeeMonster(monst) && !(monst->status.confused)) {
			if (monst->creatureState == MONSTER_SLEEPING) {
				monst->creatureState = MONSTER_TRACKING_SCENT;
			}
			flashMonster(monst, &confusionGasColor, 100);
			monsterName(buf, monst, true);
			sprintf(buf2, "%s %s very confused!", buf, (monst == &player ? "feel": "looks"));
			message(buf2, true, false);
		}
		monst->status.confused = monst->maxStatus.confused = max(monst->status.confused, 25);
	}
	
	if (cellHasTerrainFlag(*x, *y, CAUSES_PARALYSIS) && !(monst->info.flags & MONST_INANIMATE) && !(monst->bookkeepingFlags & MONST_SUBMERGED)) {
		if (canSeeMonster(monst) && !monst->status.paralyzed) {
			flashMonster(monst, &pink, 100);
			monsterName(buf, monst, true);
			sprintf(buf2, "%s %s paralyzed!", buf, (monst == &player ? "are": "is"));
			message(buf2, true, (monst == &player));
		}
		monst->status.paralyzed = monst->maxStatus.paralyzed = max(monst->status.paralyzed, 10);
		if (monst == &player) {
			rogue.disturbed = true;
		}
	}
	
	if (cellHasTerrainFlag(*x, *y, IS_FIRE)) {
		exposeCreatureToFire(monst);
	}
	
	if (cellHasTerrainFlag(*x, *y, IS_FLAMMABLE) && monst->status.burning) {
		exposeTileToFire(*x, *y, true);
	}
}

void applyGradualTileEffectsToCreature(creature *monst, short ticks) {
	short itemCandidates, randItemIndex;
	short x = monst->xLoc, y = monst->yLoc, damage;
	char buf[COLS], buf2[COLS];
	item *theItem;
	enum dungeonLayers layer;
	
	if (!(monst->status.levitating) && cellHasTerrainFlag(x, y, IS_DEEP_WATER)
		&& !(monst->info.flags & MONST_IMMUNE_TO_WATER)) {
		if (monst == &player) {
			if (!(pmap[x][y].flags & HAS_ITEM) && rand_percent(ticks * 50 / 100)) {
				itemCandidates = numberOfMatchingPackItems(ALL_ITEMS, 0, (ITEM_EQUIPPED), false);
				if (itemCandidates) {
					randItemIndex = rand_range(1, itemCandidates);
					for (theItem = packItems->nextItem; theItem != NULL; theItem = theItem->nextItem) {
						if (!(theItem->flags & (ITEM_EQUIPPED))) {
							if (randItemIndex == 1) {
								break;
							} else {
								randItemIndex--;
							}
						}
					}
					theItem = dropItem(theItem);
					if (theItem) {
						itemName(theItem, buf2, false, true);
						sprintf(buf, "%s floats away in the current!", buf2);
						message(buf, true, false);
					}
				}
			}
		} else if (monst->carriedItem && !(pmap[x][y].flags & HAS_ITEM) && rand_percent(ticks * 50 / 100)) { // it's a monster with an item
			makeMonsterDropItem(monst);
		}
	}
	
	if (cellHasTerrainFlag(x, y, CAUSES_DAMAGE) && !(monst->info.flags & MONST_INANIMATE)) {
		damage = (monst->info.maxHP / 15) * ticks / 100;
		damage = max(1, damage);
		for (layer = 0; layer < NUMBER_TERRAIN_LAYERS && !(tileCatalog[pmap[x][y].layers[layer]].flags & CAUSES_DAMAGE); layer++);
		if (monst == &player) {
			rogue.disturbed = true;
			message(tileCatalog[pmap[x][y].layers[layer]].flavorText, true, false);
			if (inflictDamage(NULL, &player, damage, tileCatalog[pmap[x][y].layers[layer]].backColor)) {
				sprintf(buf, "Killed by %s", tileCatalog[pmap[x][y].layers[layer]].description);
				gameOver(buf, true);
				return;
			}
		} else { // it's a monster
			if (monst->creatureState == MONSTER_SLEEPING) {
				monst->creatureState = MONSTER_TRACKING_SCENT;
			}
			if (inflictDamage(NULL, monst, damage, tileCatalog[pmap[x][y].layers[layer]].backColor)) {
				if (canSeeMonster(monst)) {
					monsterName(buf, monst, true);
					sprintf(buf2, "%s dies.", buf);
					message(buf2, true, false);
				}
				refreshDungeonCell(x, y);
				return;
			}
		}
	}
}

short randValidDirectionFrom(creature *monst, short x, short y, boolean respectAvoidancePreferences) {
	short i, newX, newY, validDirectionCount = 0, randIndex;
	for (i=0; i<8; i++) {
		newX = x + nbDirs[i][0];
		newY = y + nbDirs[i][1];
		if (!cellHasTerrainFlag(newX, newY, OBSTRUCTS_PASSABILITY)
			&& !cellHasTerrainFlag(newX, y, OBSTRUCTS_PASSABILITY)
			&& !cellHasTerrainFlag(x, newY, OBSTRUCTS_PASSABILITY)
			&& (!respectAvoidancePreferences
				|| (!monsterAvoids(monst, newX, newY))
				|| (!(pmap[newX][newY].flags & HAS_PLAYER) || monst->creatureState != MONSTER_ALLY))) {
			validDirectionCount++;
		}
	}
	if (validDirectionCount == 0) { // can't think of a case when this would happen
		return UP;
	}
	randIndex = rand_range(1, validDirectionCount);
	validDirectionCount = 0;
	for (i=0; i<8; i++) {
		newX = x + nbDirs[i][0];
		newY = y + nbDirs[i][1];
		if (!cellHasTerrainFlag(newX, newY, OBSTRUCTS_PASSABILITY)
			&& !cellHasTerrainFlag(newX, y, OBSTRUCTS_PASSABILITY)
			&& !cellHasTerrainFlag(x, newY, OBSTRUCTS_PASSABILITY)
			&& (!respectAvoidancePreferences
				|| (!monsterAvoids(monst, newX, newY))
				|| (!(pmap[newX][newY].flags & HAS_PLAYER) || monst->creatureState != MONSTER_ALLY))) {
			validDirectionCount++;
			if (validDirectionCount == randIndex) {
				return i;
			}
		}
	}
	return UP; // should never get here
}

void vomit(creature *monst) {
	char buf[COLS], monstName[COLS];
	spawnDungeonFeature(monst->xLoc, monst->yLoc, &dungeonFeatureCatalog[DF_VOMIT], true);
	
	if (canSeeMonster(monst)) {
		monsterName(monstName, monst, true);
		sprintf(buf, "%s vomit%s profusely.", monstName, (monst == &player ? "" : "s"));
		message(buf, true, false);
	}
}

void moveEntrancedMonsters(enum directions dir) {
	creature *monst;
	
	dir = oppositeDirection(dir);
	
	for (monst = monsters->nextCreature; monst != NULL; monst = monst->nextCreature) {
		if (monst->status.entranced) {
			if (!monst->status.stuck && !monst->status.paralyzed && !(monst->bookkeepingFlags & MONST_CAPTIVE)) {
				// && !monsterAvoids(monst, monst->xLoc + nbDirs[dir][0], monst->yLoc + nbDirs[dir][1])
				moveMonster(monst, nbDirs[dir][0], nbDirs[dir][1]);
			}
		}
	}
}

void freeCaptive(creature *monst) {
	demoteMonsterFromLeadership(monst);
	player.ticksUntilTurn += player.attackSpeed;
	monst->creatureState = MONSTER_ALLY;
	monst->bookkeepingFlags |= MONST_FOLLOWER;
	monst->leader = &player;
	monst->bookkeepingFlags &= ~MONST_CAPTIVE;
}

boolean playerMoves(short direction) {
	short x = player.xLoc, y = player.yLoc;
	short newX = x + nbDirs[direction][0], newY = y + nbDirs[direction][1];
	boolean playerMoved = false; // as opposed to attacking
	creature *defender = NULL;
	char monstName[COLS];
	char buf[COLS];
	
	if (player.status.nauseous && rand_percent(25)) {
		vomit(&player);
		playerTurnEnded();
		return true;
	}
	
	if (player.status.confused) {
		direction = randValidDirectionFrom(&player, x, y, false);
		newX = x + nbDirs[direction][0];
		newY = y + nbDirs[direction][1];
	}
	
	if (player.status.stuck && cellHasTerrainFlag(x, y, ENTANGLES) && (~pmap[newX][newY].flags & (VISIBLE | HAS_MONSTER))) {
		if (--player.status.stuck) {
			message("you struggle but cannot free yourself.", true, false);
			moveEntrancedMonsters(direction);
			playerTurnEnded();
			return true;
		}
		if (tileCatalog[pmap[x][y].layers[SURFACE]].flags & ENTANGLES) {
			pmap[x][y].layers[SURFACE] = NOTHING;
		}
	}
	
	if (pmap[newX][newY].flags & HAS_MONSTER) {
		defender = monsterAtLoc(newX, newY);
	}
	
	if ((!cellHasTerrainFlag(newX, newY, OBSTRUCTS_PASSABILITY) &&
				!cellHasTerrainFlag(newX, y, OBSTRUCTS_PASSABILITY) &&
				!cellHasTerrainFlag(x, newY, OBSTRUCTS_PASSABILITY) &&
				!cellHasTerrainFlag(x, y, OBSTRUCTS_PASSABILITY))
		|| (defender && defender->info.flags & MONST_ATTACKABLE_THRU_WALLS)) {
		// if the move is not blocked
		
		if (defender) {
			// if there is a monster there, attack.
			
			if (defender->bookkeepingFlags & MONST_CAPTIVE && !rogue.blockCombatText) {
				monsterName(monstName, defender, false);
				sprintf(buf, "Free the captive %s? (y/n)", monstName);
				if (confirm(buf)) {
					freeCaptive(defender);
					playerTurnEnded();
					return true;
				} else {
					return false;
				}
			}
			
			if (defender->creatureState != MONSTER_ALLY) {
				attack(&player, defender);
				
				player.ticksUntilTurn += player.attackSpeed;
				moveEntrancedMonsters(direction);
				
				playerTurnEnded();
				return true;
			}
		}
		
		if (player.bookkeepingFlags & MONST_SEIZED) {
			for (defender = monsters->nextCreature; defender != NULL; defender = defender->nextCreature) {
				if (defender->bookkeepingFlags & MONST_SEIZING
					&& distanceBetween(player.xLoc, player.yLoc, defender->xLoc, defender->yLoc) == 1
					&& !player.status.levitating) {
					monsterName(monstName, defender, true);
					sprintf(buf, "you struggle but %s is holding your legs!", monstName);
					message(buf, true, false);
					playerTurnEnded();
					return true;
				}
			}
			player.bookkeepingFlags &= ~MONST_SEIZED; // failsafe
		}
		
		if (pmap[newX][newY].flags & (DISCOVERED | MAGIC_MAPPED)
				   && !player.status.levitating
				   && !player.status.confused
				   && cellHasTerrainFlag(newX, newY, LAVA_INSTA_DEATH)
				   && !(player.info.flags & MONST_IMMUNE_TO_FIRE)
				   && !cellHasTerrainFlag(newX, newY, IS_SECRET | ENTANGLES)
				   && !player.status.confused) {
			message("that would be certain death!", true, false);
			return false; // player won't willingly step into lava
		} else if (pmap[newX][newY].flags & (DISCOVERED | MAGIC_MAPPED)
				   && !player.status.levitating
				   && !player.status.confused
				   && cellHasTerrainFlag(newX, newY, TRAP_DESCENT)
				   && !cellHasTerrainFlag(newX, newY, IS_SECRET | ENTANGLES)
				   && !confirm("Dive into the depths? (y/n)")) {
			return false;
		} else if (pmap[newX][newY].flags & (VISIBLE | CLAIRVOYANT_VISIBLE)
				   && !player.status.levitating
				   && !player.status.confused
				   && !player.status.burning
				   && !player.status.immuneToFire
				   && cellHasTerrainFlag(newX, newY, IS_FIRE)
				   && !cellHasTerrainFlag(newX, newY, EXTINGUISHES_FIRE)
				   && !confirm("Venture into flame? (y/n)")) {
			return false;
		} else if (pmap[newX][newY].flags & (VISIBLE | CLAIRVOYANT_VISIBLE | MAGIC_MAPPED)
				   && !player.status.levitating
				   && !player.status.confused
				   && cellHasTerrainFlag(newX, newY, IS_DF_TRAP)
				   && !(pmap[newX][newY].flags & PRESSURE_PLATE_DEPRESSED)
				   && !cellHasTerrainFlag(newX, newY, IS_SECRET)
				   && !confirm("Step onto the trap? (y/n)")) {
			return false;
		} else {
			if (defender) { // swap places with ally
				pmap[defender->xLoc][defender->yLoc].flags &= ~HAS_MONSTER;
				defender->xLoc = player.xLoc;
				defender->yLoc = player.yLoc;
				pmap[defender->xLoc][defender->yLoc].flags |= HAS_MONSTER;
			}
			player.xLoc += nbDirs[direction][0];
			player.yLoc += nbDirs[direction][1];
			pmap[x][y].flags &= ~HAS_PLAYER;
			pmap[player.xLoc][player.yLoc].flags |= HAS_PLAYER;
			if (pmap[player.xLoc][player.yLoc].flags & HAS_ITEM) {
				pickUpItemAt(player.xLoc, player.yLoc);
				rogue.disturbed = true;
			}
			refreshDungeonCell(x, y);
			refreshDungeonCell(player.xLoc, player.yLoc);
			playerMoved = true;
			moveEntrancedMonsters(direction);
		}
		
		playerTurnEnded(); // causes all the monsters to move and everything else to be updated
	}
	return playerMoved;
}

// returns true if the cell value changed
boolean updateDistanceCell(short **distanceMap, short x, short y) {
	short dir, newX, newY;
	boolean somethingChanged = false;
	
	if (distanceMap[x][y] >= 0 && distanceMap[x][y] < 30000) {
		for (dir=0; dir<8; dir++) {
			newX = x + nbDirs[dir][0];
			newY = y + nbDirs[dir][1];
			if (coordinatesAreInMap(newX, newY) && distanceMap[newX][newY] >= distanceMap[x][y] + 2
				&& !cellHasTerrainFlag(x, newY, OBSTRUCTS_PASSABILITY)
				&& !cellHasTerrainFlag(newX, y, OBSTRUCTS_PASSABILITY)) {
				distanceMap[newX][newY] = distanceMap[x][y] + 1;
				somethingChanged = true;
			}
		}
	}
	return somethingChanged;
}


// replaced in Dijkstra.c:
/*void dijkstraScan(short **distanceMap, char passMap[DCOLS][DROWS], boolean allowDiagonals) {
	short i, j, maxDir;
	enum directions dir;
	boolean somethingChanged;
	
	maxDir = (allowDiagonals ? 8 : 4);
	
	do {
		somethingChanged = false;
		for (i=1; i<DCOLS-1; i++) {
			for (j=1; j<DROWS-1; j++) {
				if (!passMap || passMap[i][j]) {
					for (dir = 0; dir < maxDir; dir++) {
						if (coordinatesAreInMap(i + nbDirs[dir][0], j + nbDirs[dir][1])
							&& (!passMap || passMap[i + nbDirs[dir][0]][j + nbDirs[dir][1]])
							&& distanceMap[i + nbDirs[dir][0]][j + nbDirs[dir][1]] >= distanceMap[i][j] + 2) {
							distanceMap[i + nbDirs[dir][0]][j + nbDirs[dir][1]] = distanceMap[i][j] + 1;
							somethingChanged = true;
						}
					}
				}
			}
		}
		
		
		for (i = DCOLS - 1; i >= 0; i--) {
			for (j = DROWS - 1; j >= 0; j--) {
				if (!passMap || passMap[i][j]) {
					for (dir = 0; dir < maxDir; dir++) {
						if (coordinatesAreInMap(i + nbDirs[dir][0], j + nbDirs[dir][1])
							&& (!passMap || passMap[i + nbDirs[dir][0]][j + nbDirs[dir][1]])
							&& distanceMap[i + nbDirs[dir][0]][j + nbDirs[dir][1]] >= distanceMap[i][j] + 2) {
							distanceMap[i + nbDirs[dir][0]][j + nbDirs[dir][1]] = distanceMap[i][j] + 1;
							somethingChanged = true;
						}
					}
				}
			}
		}
	} while (somethingChanged);
}*/

/*void enqueue(short x, short y, short val, distanceQueue *dQ) {
	short *qX2, *qY2, *qVal2;
	
	// if we need to allocate more memory:
	if (dQ->qLen + 1 > dQ->qMaxLen) {
		dQ->qMaxLen *= 2;
		qX2 = realloc(dQ->qX, dQ->qMaxLen);
		if (qX2) {
			free(dQ->qX);
			dQ->qX = qX2;
		} else {
			// out of memory
		}
		qY2 = realloc(dQ->qY, dQ->qMaxLen);
		if (qY2) {
			free(dQ->qY);
			dQ->qY = qY2;
		} else {
			// out of memory
		}
		qVal2 = realloc(dQ->qVal, dQ->qMaxLen);
		if (qVal2) {
			free(dQ->qVal);
			dQ->qVal = qVal2;
		} else {
			// out of memory
		}
	}
	
	dQ->qX[dQ->qLen] = x;
	dQ->qY[dQ->qLen] = y;
	(dQ->qVal)[dQ->qLen] = val;
	
	dQ->qLen++;
	
	if (val < dQ->qMinVal) {
		dQ->qMinVal = val;
		dQ->qMinCount = 1;
	} else if (val == dQ->qMinVal) {
		dQ->qMinCount++;
	}
}

void updateQueueMinCache(distanceQueue *dQ) {
	short i;
	dQ->qMinCount = 0;
	dQ->qMinVal = 30001;
	for (i = 0; i < dQ->qLen; i++) {
		if (dQ->qVal[i] < dQ->qMinVal) {
			dQ->qMinVal = dQ->qVal[i];
			dQ->qMinCount = 1;
		} else if (dQ->qVal[i] == dQ->qMinVal) {
			dQ->qMinCount++;
		}
	}
}

// removes the lowest value from the queue, populates x/y/value variables and updates min caching
void dequeue(short *x, short *y, short *val, distanceQueue *dQ) {
	short i, minIndex;
	
	if (dQ->qMinCount <= 0) {
		updateQueueMinCache(dQ);
	}
	
	*val = dQ->qMinVal;
	
	// find the last instance of the minVal
	for (minIndex = dQ->qLen - 1; minIndex >= 0 && dQ->qVal[minIndex] != *val; minIndex--);
	
	// populate the return variables
	*x = dQ->qX[minIndex];
	*y = dQ->qY[minIndex];
	
	dQ->qLen--;
	
	// delete the minValue queue entry
	for (i = minIndex; i < dQ->qLen; i++) {
		dQ->qX[i] = dQ->qX[i+1];
		dQ->qY[i] = dQ->qY[i+1];
		dQ->qVal[i] = dQ->qVal[i+1];
	}
	
	// update min values
	dQ->qMinCount--;
	if (!dQ->qMinCount && dQ->qLen) {
		updateQueueMinCache(dQ);
	}
	
}

void dijkstraScan(short **distanceMap, char passMap[DCOLS][DROWS], boolean allowDiagonals) {
	short i, j, maxDir, val;
	enum directions dir;
	distanceQueue dQ;
	
	dQ.qMaxLen = DCOLS * DROWS * 1.5;
	dQ.qX = (short *) malloc(dQ.qMaxLen * sizeof(short));
	dQ.qY = (short *) malloc(dQ.qMaxLen * sizeof(short));
	dQ.qVal = (short *) malloc(dQ.qMaxLen * sizeof(short));
	dQ.qLen = 0;
	dQ.qMinVal = 30000;
	dQ.qMinCount = 0;
	
	maxDir = (allowDiagonals ? 8 : 4);
	
	// seed the queue with the entire map
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			if (!passMap || passMap[i][j]) {
				enqueue(i, j, distanceMap[i][j], &dQ);
			}
		}
	}
	
	// iterate through queue updating lowest entries until the queue is empty
	while (dQ.qLen) {
		dequeue(&i, &j, &val, &dQ);
		if (distanceMap[i][j] == val) { // if it hasn't been improved since joining the queue
			for (dir = 0; dir < maxDir; dir++) {
				if (coordinatesAreInMap(i + nbDirs[dir][0], j + nbDirs[dir][1])
					&& (!passMap || passMap[i + nbDirs[dir][0]][j + nbDirs[dir][1]])
					&& distanceMap[i + nbDirs[dir][0]][j + nbDirs[dir][1]] >= distanceMap[i][j] + 2) {
					
					distanceMap[i + nbDirs[dir][0]][j + nbDirs[dir][1]] = distanceMap[i][j] + 1;
					
					enqueue(i + nbDirs[dir][0], j + nbDirs[dir][1], distanceMap[i][j] + 1, &dQ);
				}
			}
		}
	}
	
	free(dQ.qX);
	free(dQ.qY);
	free(dQ.qVal);
}*/

/*
void calculateDistances(short **distanceMap, short destinationX, short destinationY, unsigned long blockingTerrainFlags, creature *traveler) {
	short i, j;
	boolean somethingChanged;
	
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			distanceMap[i][j] = ((traveler && traveler == &player && !(pmap[i][j].flags & (DISCOVERED | MAGIC_MAPPED)))
								 || ((traveler && monsterAvoids(traveler, i, j))
									 || cellHasTerrainFlag(i, j, blockingTerrainFlags))) ? -1 : 30000;
		}
	}
	
	distanceMap[destinationX][destinationY] = 0;
	
//	dijkstraScan(distanceMap);
	do {
		somethingChanged = false;
		for (i=0; i<DCOLS; i++) {
			for (j=0; j<DROWS; j++) {
				if (updateDistanceCell(distanceMap, i, j)) {
					somethingChanged = true;
				}
			}
		}
		
		
		for (i = DCOLS - 1; i >= 0; i--) {
			for (j = DROWS - 1; j >= 0; j--) {
				if (updateDistanceCell(distanceMap, i, j)) {
					somethingChanged = true;
				}
			}
		}
	} while (somethingChanged);
}*/

// returns -1 if there are no beneficial moves
short nextStep(short **distanceMap, short x, short y) {
	short dir, newX, newY, bestScore, bestDir;
	
	bestScore = 0;
	bestDir = -1;
	
	for (dir = 0; dir < 8; dir++) {
		newX = x + nbDirs[dir][0];
		newY = y + nbDirs[dir][1];
		if (distanceMap[x][y] - distanceMap[newX][newY] > bestScore
			&& isPassableOrSecretDoor(x, newY)
			&& isPassableOrSecretDoor(newX, y)
			&& isPassableOrSecretDoor(newX, newY)) {
			bestDir = dir;
			bestScore = distanceMap[x][y] - distanceMap[newX][newY];
		}
	}
	return bestDir;
}

void displayRoute(short **distanceMap, boolean removeRoute) {
	short currentX = player.xLoc, currentY = player.yLoc, dir, newX, newY;
	boolean advanced;
	
	if (distanceMap[player.xLoc][player.yLoc] < 0 || distanceMap[player.xLoc][player.yLoc] == 30000) {
		return;
	}
	do {
		if (removeRoute) {
			refreshDungeonCell(currentX, currentY);
		} else {
			hiliteCell(currentX, currentY, &hiliteColor, 50);
		}
		advanced = false;
		for (dir = 0; dir < 8; dir++) {
			newX = currentX + nbDirs[dir][0];
			newY = currentY + nbDirs[dir][1];
			if (distanceMap[newX][newY] >= 0 && distanceMap[newX][newY] < distanceMap[currentX][currentY]
				&& !cellHasTerrainFlag(newX, currentY, OBSTRUCTS_PASSABILITY)
				&& !cellHasTerrainFlag(currentX, newY, OBSTRUCTS_PASSABILITY)) {
				currentX = newX;
				currentY = newY;
				advanced = true;
				break;
			}
		}
	} while (advanced);
}

void travelRoute(short **distanceMap) {
	short currentX = player.xLoc, currentY = player.yLoc, dir, newX, newY;
	boolean advanced;
	
	rogue.disturbed = false;
	rogue.automationActive = true;
	
	if (distanceMap[player.xLoc][player.yLoc] < 0 || distanceMap[player.xLoc][player.yLoc] == 30000) {
		return;
	}
	do {
		advanced = false;
		for (dir = 0; dir < 8; dir++) {
			newX = currentX + nbDirs[dir][0];
			newY = currentY + nbDirs[dir][1];
			if (distanceMap[newX][newY] >= 0 && distanceMap[newX][newY] < distanceMap[currentX][currentY]
				&& !cellHasTerrainFlag(newX, currentY, OBSTRUCTS_PASSABILITY)
				&& !cellHasTerrainFlag(currentX, newY, OBSTRUCTS_PASSABILITY)) {
				if (!playerMoves(dir)) {
					rogue.disturbed = true;
				}
				if (pauseBrogue(1)) {
					rogue.disturbed = true;
				}
				currentX = newX;
				currentY = newY;
				advanced = true;
				break;
			}
		}
	} while (advanced && !rogue.disturbed);
	rogue.disturbed = true;
	rogue.automationActive = false;
	updateFlavorText();
}

void travel(short x, short y, boolean autoConfirm) {
	short **distanceMap;
	rogueEvent theEvent;
	unsigned short staircaseConfirmKey;
	
	confirmMessages();
	
	if (D_WORMHOLING) {
		pmap[player.xLoc][player.yLoc].flags &= ~HAS_PLAYER;
		refreshDungeonCell(player.xLoc, player.yLoc);
		player.xLoc = x;
		player.yLoc = y;
		pmap[x][y].flags |= HAS_PLAYER;
		refreshDungeonCell(x, y);
		updateVision();
		return;
	}
	
	if (!(pmap[x][y].flags & (DISCOVERED | MAGIC_MAPPED))) {
		message("You have not explored that location.", true, false);
		return;
	}
	
	distanceMap = allocDynamicGrid();
	
	calculateDistances(distanceMap, x, y, 0, &player);
	if (distanceMap[player.xLoc][player.yLoc] < 30000) {
		if (autoConfirm) {
			travelRoute(distanceMap);
			refreshSideBar(NULL);
		} else {
			if (cellHasTerrainFlag(x, y, PERMITS_ASCENT)) {
				staircaseConfirmKey = ASCEND_KEY;
			} else if (cellHasTerrainFlag(x, y, PERMITS_DESCENT)) {
				staircaseConfirmKey = DESCEND_KEY;
			} else {
				staircaseConfirmKey = 0;
			}
			displayRoute(distanceMap, false);
			message("Travel this route? (y/n)", true, false);
			
			do {
				nextBrogueEvent(&theEvent, false);
			} while (theEvent.eventType != MOUSE_UP && theEvent.eventType != KEYSTROKE);
			
			displayRoute(distanceMap, true); // clear route display
			confirmMessages();
			
			if ((theEvent.eventType == MOUSE_UP && theEvent.param1 - STAT_BAR_WIDTH == x && theEvent.param2 - MESSAGE_LINES == y)
				|| (theEvent.eventType == KEYSTROKE && (theEvent.param1 == 'Y' || theEvent.param1 == 'y'
														|| theEvent.param1 == RETURN_KEY
														|| (theEvent.param1 == staircaseConfirmKey
															&& theEvent.param1 != 0)))) {
				travelRoute(distanceMap);
				refreshSideBar(NULL);
				commitDraws();
			} else if (theEvent.eventType == MOUSE_UP) {
				executeMouseClick(&theEvent);
			}
		}
		if (player.xLoc == x && player.yLoc == y) {
			rogue.lastTravelLoc[0] = rogue.lastTravelLoc[1] = 0;
		} else {
			rogue.lastTravelLoc[0] = x;
			rogue.lastTravelLoc[1] = y;
		}
	} else {
		rogue.lastTravelLoc[0] = rogue.lastTravelLoc[1] = 0;
		message("No path is available.", true, false);
	}
	freeDynamicGrid(distanceMap);
}

short exploreCost(short x, short y) {
	unsigned long flags = terrainFlags(x, y);
	if (flags & ENTANGLES | CAUSES_PARALYSIS) {
		return 5;
	} else if (flags & CAUSES_DAMAGE) {
		return 4;
	} else if (flags & CAUSES_NAUSEA) {
		return 3;
	}
	if (pmap[x][y].flags & PLAYER_STEPPED_HERE) {
		return 1;
	} else {
		return 2;
	}
}

boolean explore(short frameDelay) {
	short **distanceMap, i, j;
	boolean passMap[DCOLS][DROWS];
	short goalValue[DCOLS][DROWS];
	boolean madeProgress, headingToStairs;
	short dir;
	item *theItem;
	creature *monst;
	
	distanceMap = allocDynamicGrid();
	
	
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			goalValue[i][j] = 0 - abs(i - DCOLS / 2) - abs(j - DROWS / 2);
		}
	}
	
	madeProgress = false;
	headingToStairs = false;
	
	if (player.status.confused) {
		message("Not while you're confused.", true, false);
		return false;
	}
	
	// fight any adjacent enemies
	for (dir = 0; dir < 8; dir++) {
		monst = monsterAtLoc(player.xLoc + nbDirs[dir][0], player.yLoc + nbDirs[dir][1]);
		if (monst && canSeeMonster(monst) && monstersAreEnemies(&player, monst)) {
			startFighting(dir, (player.status.hallucinating ? true : false));
			if (rogue.disturbed) {
				return true;
			}
		}
	}
	
	if (!rogue.autoPlayingLevel) {
		confirmMessages();
		// don't want to have to press space as soon as something happens:
		for (i=STAT_BAR_WIDTH; i < COLS; i++) {
			plotCharWithColor(' ', i, 0, black, black);
		}
		message("Exploring... press any key to stop.", true, false);
	}
	rogue.disturbed = false;
	rogue.automationActive = true;
	
	do {
		
		// fight any adjacent enemies (mainly for autoplay)
		for (dir = 0; dir < 8; dir++) {
			monst = monsterAtLoc(player.xLoc + nbDirs[dir][0], player.yLoc + nbDirs[dir][1]);
			if (monst && canSeeMonster(monst) && monstersAreEnemies(&player, monst)) {
				startFighting(dir, (player.status.hallucinating ? true : false));
				if (rogue.disturbed) {
					return true;
				}
			}
		}
		
		// calculate explore map
		for (i=0; i<DCOLS; i++) {
			for (j=0; j<DROWS; j++) {
				
				if (pmap[i][j].flags & HAS_ITEM) {
					theItem = itemAtLoc(i, j);
				} else {
					theItem = NULL;
				}
				
				if (pmap[i][j].flags & HAS_MONSTER) {
					monst = monsterAtLoc(i, j);
				} else {
					monst = NULL;
				}
				
				if (!(pmap[i][j].flags & DISCOVERED)) {
					passMap[i][j] = true;
					distanceMap[i][j] = goalValue[i][j];
				} else if ((theItem && !(theItem->flags & ITEM_NAMED) && !player.status.levitating)) {
						  // || (monst && monst->bookkeepingFlags & MONST_CAPTIVE && canSeeMonster(monst))) {
					passMap[i][j] = true;
					distanceMap[i][j] = goalValue[i][j] - 20;
				} else {
					passMap[i][j] = monsterAvoids(&player, i, j) ? false : true;
					distanceMap[i][j] = 30000;
				}
			}
		}
		
		if (headingToStairs) {
			distanceMap[rogue.downLoc[0]][rogue.downLoc[1]] = 0; // head to the stairs
		}
		
		dijkstraScan(distanceMap, NULL, passMap, false);
		
		// take a step
		dir = nextStep(distanceMap, player.xLoc, player.yLoc);
		
		if (!headingToStairs && rogue.autoPlayingLevel && dir == -1) {
			headingToStairs = true;
			continue;
		}
		
		if (dir == -1) {
			message("I see no path for further exploration.", true, false);
			rogue.disturbed = true;
		} else if (!playerMoves(dir)) {
			rogue.disturbed = true;
		} else {
			madeProgress = true;
			if (pauseBrogue(frameDelay)) {
				rogue.disturbed = true;
				rogue.autoPlayingLevel = false;
			}
		}
		
	} while(!rogue.disturbed);
	
	rogue.automationActive = false;
	
	refreshSideBar(NULL);
	
	commitDraws();
	
	freeDynamicGrid(distanceMap);
	
	return madeProgress;
}

void examine(short x, short y) {
	
}

void unexamine(short x, short y) {
	
}

void examineMode() {
	short originLoc[2], targetLoc[2], oldTargetLoc[2];
	creature *monst;
	cellDisplayBuffer rbuf[COLS][ROWS];
	boolean canceled, targetConfirmed, tabKey, focusedOnMonster = false;
	
	temporaryMessage("Examine what? (<hjklyubn>, mouse, or <tab>; <return> to travel)", false);
	
	originLoc[0] = player.xLoc;
	originLoc[1] = player.yLoc;
	
	targetLoc[0] = player.xLoc;
	targetLoc[1] = player.yLoc;
	
	hiliteCell(targetLoc[0], targetLoc[1], &white, 75);
	
	do {
		printLocationDescription(targetLoc[0], targetLoc[1]);
		
		oldTargetLoc[0] = targetLoc[0];
		oldTargetLoc[1] = targetLoc[1];
		
		monst = monsterAtLoc(targetLoc[0], targetLoc[1]);
		if (monst != NULL && monst != &player && canSeeMonster(monst)) {
			refreshSideBar(monst);
			focusedOnMonster = true;
			if (!player.status.hallucinating) {
				printMonsterDetails(monst, rbuf);
			}
		}
		
		moveCursor(&targetConfirmed, &canceled, &tabKey, targetLoc);
		
		if (focusedOnMonster) {
			refreshSideBar(NULL);
			focusedOnMonster = false;
			if (!player.status.hallucinating) {
				overlayDisplayBuffer(rbuf, 0);
			}
		}
		
		if (tabKey) {
			monst = nextTargetAfter(targetLoc[0], targetLoc[1], false, false);
			if (monst) {
				targetLoc[0] = monst->xLoc;
				targetLoc[1] = monst->yLoc;
			}
		}
		
		refreshDungeonCell(oldTargetLoc[0], oldTargetLoc[1]);
		
		if (!targetConfirmed) {
			hiliteCell(targetLoc[0], targetLoc[1], &white, 75);
		}
		
	} while (!targetConfirmed && !canceled);
	
	if (canceled) {
		refreshDungeonCell(oldTargetLoc[0], oldTargetLoc[1]);
		confirmMessages();
	} else {
		confirmMessages();
		
		if (originLoc[0] == targetLoc[0] && originLoc[1] == targetLoc[1]) {
			confirmMessages();
		} else {
			travel(targetLoc[0], targetLoc[1], false);
		}
	}
}

void autoPlayLevel(boolean fastForward) {
	boolean madeProgress;
	
	rogue.autoPlayingLevel = true;
	
	confirmMessages();
	message("Playing... press any key to stop.", true, false);
	
	// explore until we are not making progress
	do {
		madeProgress = explore(fastForward ? 1 : 50);
		refreshSideBar(NULL);
		
		if (!madeProgress && cellHasTerrainFlag(player.xLoc, player.yLoc, PERMITS_DESCENT)) {
			useStairs(1);
			madeProgress = true;
		}
	} while (madeProgress && rogue.autoPlayingLevel);
	
	confirmMessages();
	
	rogue.autoPlayingLevel = false;
}

void updateClairvoyance() {
	short i, j, clairvoyanceRadius, dx, dy;
	boolean cursed;
	unsigned long cFlags;
	
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			
			pmap[i][j].flags &= ~WAS_CLAIRVOYANT_VISIBLE;
			
			if (pmap[i][j].flags & CLAIRVOYANT_VISIBLE) {
				pmap[i][j].flags |= WAS_CLAIRVOYANT_VISIBLE;
			}
			
			pmap[i][j].flags &= ~CLAIRVOYANT_VISIBLE;
			pmap[i][j].flags &= ~CLAIRVOYANT_DARKENED;
		}
	}
	
	cursed = (rogue.clairvoyance < 0);
	if (cursed) {
		clairvoyanceRadius = (rogue.clairvoyance - 1) * -1;
		cFlags = CLAIRVOYANT_DARKENED;
	} else {
		clairvoyanceRadius = (rogue.clairvoyance > 0) ? rogue.clairvoyance + 1 : 0;
		cFlags = CLAIRVOYANT_VISIBLE | DISCOVERED;
	}
	
	for (i = max(0, player.xLoc - clairvoyanceRadius); i < min(DCOLS, player.xLoc + clairvoyanceRadius + 1); i++) {
		for (j = max(0, player.yLoc - clairvoyanceRadius); j < min(DROWS, player.yLoc + clairvoyanceRadius + 1); j++) {
			
			dx = (player.xLoc - i);
			dy = (player.yLoc - j);
			
			if (dx*dx + dy*dy < clairvoyanceRadius*clairvoyanceRadius + clairvoyanceRadius
				&& (pmap[i][j].layers[DUNGEON] != GRANITE || pmap[i][j].flags & DISCOVERED)) {
				pmap[i][j].flags |= cFlags;
				if (!(pmap[i][j].flags & HAS_PLAYER) && !cursed) {
					pmap[i][j].flags &= ~STABLE_MEMORY;
				}
			}
		}
	}
}

void updateScent() {
	short i, j;
	char grid[DCOLS][DROWS];
	zeroOutGrid(grid);
	
	getFOVMask(grid, player.xLoc, player.yLoc, DCOLS, OBSTRUCTS_SCENT, 0, false);
	
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			if (grid[i][j]) {
				if (abs(player.xLoc - i) > abs(player.yLoc - j)) {
					addScentToCell(i, j, 2 * abs(player.xLoc - i) + abs(player.yLoc - j));
				} else {
					addScentToCell(i, j, abs(player.xLoc - i) + 2 * abs(player.yLoc - j));
				}
			}
		}
	}
	addScentToCell(player.xLoc, player.yLoc, 0);
}

void demoteVisibility() {
	short i, j;
	
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			pmap[i][j].flags &= ~WAS_VISIBLE;
			if (pmap[i][j].flags & VISIBLE) {
				pmap[i][j].flags &= ~VISIBLE;
				pmap[i][j].flags |= WAS_VISIBLE;
			}
		}
	}
}

void updateVision() {
	short i, j;
	char grid[DCOLS][DROWS];
	item *theItem;
	creature *monst;
	
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			pmap[i][j].flags &= ~WAS_VISIBLE;
			if (pmap[i][j].flags & VISIBLE) {
				pmap[i][j].flags &= ~VISIBLE;
				pmap[i][j].flags |= WAS_VISIBLE;
			}
			pmap[i][j].flags &= ~IN_FIELD_OF_VIEW;
		}
	}
	
	zeroOutGrid(grid);
	getFOVMask(grid, player.xLoc, player.yLoc, DCOLS + DROWS, (OBSTRUCTS_VISION), 0, false);
	
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			if (grid[i][j]) {
				pmap[i][j].flags |= IN_FIELD_OF_VIEW;
			}
		}
	}
	pmap[player.xLoc][player.yLoc].flags |= IN_FIELD_OF_VIEW;
	
	if (rogue.clairvoyance < 0) {
		pmap[player.xLoc][player.yLoc].flags |= DISCOVERED;
		pmap[player.xLoc][player.yLoc].flags &= ~STABLE_MEMORY;
	}
	
	if (rogue.clairvoyance != 0) {
		updateClairvoyance();
	}
	
	updateLighting();
	
	updateFieldOfViewDisplay(true);
	
	if (player.status.hallucinating > 0) {
		for (theItem = floorItems->nextItem; theItem != NULL; theItem = theItem->nextItem) {
			if (pmap[theItem->xLoc][theItem->yLoc].flags & DISCOVERED) {
				refreshDungeonCell(theItem->xLoc, theItem->yLoc);
			}
		}
		for (monst = monsters->nextCreature; monst != NULL; monst = monst->nextCreature) {
			if (pmap[monst->xLoc][monst->yLoc].flags & DISCOVERED) {
				refreshDungeonCell(monst->xLoc, monst->yLoc);
			}
		}
	}
}

void checkNutrition() {
	item *theItem;
	char buf[DCOLS];
	if (player.status.nutrition == HUNGER_THRESHOLD) {
		message("you are hungry.", true, false);
		// updateStatBar(HUNGER_STAT);
	} else if (player.status.nutrition == WEAK_THRESHOLD) {
		message("you feel weak with hunger.", true, true);
		// updateStatBar(HUNGER_STAT);
	} else if (player.status.nutrition == FAINT_THRESHOLD) {
		message("you feel faint with hunger.", true, true);
		// updateStatBar(HUNGER_STAT);
	} else if (player.status.nutrition <= 0) {
		if (!player.status.paralyzed) {
			// force the player to eat something if he has it
			for (theItem = packItems->nextItem; theItem != NULL; theItem = theItem->nextItem) {
				if (theItem->category == FOOD) {
					sprintf(buf, "unable to control your hunger, you eat a %s.", (theItem->kind == FRUIT ? "mango" : "ration of food"));
					message(buf, true, true);
					apply(theItem);
					return;
				}
			}
		}
		gameOver("Starved to death", true);
	}
}

void burnItem(item *theItem) {
	short x, y;
	char buf1[DCOLS], buf2[DCOLS];
	item *prevItem;
	itemName(theItem, buf1, false, true);
	sprintf(buf2, "%s burns up!", buf1);
	
	x = theItem->xLoc;
	y = theItem->yLoc;
	
	for(prevItem = floorItems; prevItem->nextItem != theItem; prevItem = prevItem->nextItem);
	prevItem->nextItem = theItem->nextItem;
	
	pmap[x][y].flags &= ~(HAS_ITEM | ITEM_DETECTED);
	
	if (pmap[x][y].flags & (VISIBLE | CLAIRVOYANT_VISIBLE | DISCOVERED | ITEM_DETECTED)) {	
		refreshDungeonCell(x, y);
	}
	
	if (pmap[x][y].flags & (VISIBLE | CLAIRVOYANT_VISIBLE)) {
		message(buf2, true, false);
	}
}

void promoteTile(short x, short y, enum dungeonLayers layer, boolean useFireDF) {
	enum dungeonFeatureTypes DFType;
	floorTileType *tile;
	
	tile = &(tileCatalog[pmap[x][y].layers[layer]]);
	
	DFType = (useFireDF ? tile->fireType : tile->promoteType);
	
	if (tile->flags & VANISHES_UPON_PROMOTION) {
		pmap[x][y].layers[layer] = (layer == DUNGEON ? FLOOR : NOTHING);
		if (layer == GAS) {
			pmap[x][y].volume = 0;
		}
		refreshDungeonCell(x, y);
	}
	if (DFType) {
		spawnDungeonFeature(x, y, &dungeonFeatureCatalog[DFType], true);
	}
}

boolean exposeTileToFire(short x, short y, boolean alwaysIgnite) {
	enum dungeonLayers layer;
	short ignitionChance = 0;
	boolean fireIgnited = false;
	
	if (!cellHasTerrainFlag(x, y, IS_FLAMMABLE)) {
		return false;
	}
	
	// pick the fire type of the most flammable layer
	for (layer=0; layer < NUMBER_TERRAIN_LAYERS; layer++) {
		if (tileCatalog[pmap[x][y].layers[layer]].flags & IS_FLAMMABLE
			&& tileCatalog[pmap[x][y].layers[layer]].chanceToIgnite > ignitionChance) {
			ignitionChance = tileCatalog[pmap[x][y].layers[layer]].chanceToIgnite;
		}
	}
	
	if (alwaysIgnite || rand_percent(ignitionChance)) {	// if it ignites
		fireIgnited = true;
		
		// flammable layers are consumed
		for (layer=0; layer < NUMBER_TERRAIN_LAYERS; layer++) {
			if (tileCatalog[pmap[x][y].layers[layer]].flags & IS_FLAMMABLE) {
				promoteTile(x, y, layer, true);
				if (layer == GAS) {
					pmap[x][y].volume = 0; // flammable gas burns its volume away
				}
			}
		}
		
		refreshDungeonCell(x, y);
		
	}
	return fireIgnited;
}

// only the gas layer can be volumetric
void updateVolumetricMedia() {
	short i, j, newX, newY, numSpaces;
	unsigned long highestNeighborVolume;
	unsigned long sum;
	enum tileType gasType;
	// assume gas layer
	//	enum dungeonLayers layer;
	enum directions dir;
	unsigned short newGasVolume[DCOLS][DROWS];
	
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			newGasVolume[i][j] = 0;
			if (!cellHasTerrainFlag(i, j, OBSTRUCTS_GAS)) {
				sum = pmap[i][j].volume;
				numSpaces = 1;
				highestNeighborVolume = pmap[i][j].volume;
				gasType = pmap[i][j].layers[GAS];
				for (dir=0; dir<8; dir++) {
					newX = i + nbDirs[dir][0];
					newY = j + nbDirs[dir][1];
					if (coordinatesAreInMap(newX, newY) && !cellHasTerrainFlag(newX, newY, OBSTRUCTS_GAS)) {
						sum += pmap[newX][newY].volume;
						numSpaces++;
						if (pmap[newX][newY].volume > highestNeighborVolume) {
							highestNeighborVolume = pmap[newX][newY].volume;
							gasType = pmap[newX][newY].layers[GAS];
						}
					}
				}
				if (cellHasTerrainFlag(i, j, TRAP_DESCENT)) { // if it's a chasm tile or trap door
					numSpaces++; // this will allow gas to escape from the level entirely
				}
				newGasVolume[i][j] = sum / max(1, numSpaces);
				if (rand_range(0, numSpaces - 1) < sum % numSpaces) {
					newGasVolume[i][j]++; // stochastic rounding
				}
				if (pmap[i][j].layers[GAS] != gasType && newGasVolume[i][j] > 3) {
					if (pmap[i][j].layers[GAS] != NOTHING) {
						newGasVolume[i][j] = min(3, newGasVolume[i][j]); // otherwise interactions between gases are crazy
					}
					pmap[i][j].layers[GAS] = gasType;
				} else if (pmap[i][j].layers[GAS] && newGasVolume[i][j] < 1) {
					pmap[i][j].layers[GAS] = NOTHING;
					refreshDungeonCell(i, j);
				}
				if (pmap[i][j].volume > 0) {
					if (tileCatalog[pmap[i][j].layers[GAS]].flags & GAS_DISSIPATES_QUICKLY) {
						newGasVolume[i][j] -= (rand_percent(50) ? 1 : 0);
					} else if (tileCatalog[pmap[i][j].layers[GAS]].flags & GAS_DISSIPATES) {
						newGasVolume[i][j] -= (rand_percent(20) ? 1 : 0);
					}
				}
			} else if (pmap[i][j].volume > 0) { // if has gas but can't hold gas
				// disperse gas instantly into neighboring tiles that can hold gas
				numSpaces = 0;
				for (dir = 0; dir < 8; dir++) {
					newX = i + nbDirs[dir][0];
					newY = j + nbDirs[dir][1];
					if (coordinatesAreInMap(newX, newY) && !cellHasTerrainFlag(newX, newY, OBSTRUCTS_GAS)) {
						numSpaces++;
					}
				}
				if (numSpaces > 0) {
					for (dir = 0; dir < 8; dir++) {
						newX = i + nbDirs[dir][0];
						newY = j + nbDirs[dir][1];
						if (coordinatesAreInMap(newX, newY) && !cellHasTerrainFlag(newX, newY, OBSTRUCTS_GAS)) {
							newGasVolume[newX][newY] = pmap[newX][newY].volume + (pmap[i][j].volume / numSpaces);
							if (newGasVolume[newX][newY]) {
								pmap[newX][newY].layers[GAS] = pmap[i][j].layers[GAS];
							}
						}
					}
				}
				newGasVolume[i][j] = 0;
				pmap[i][j].layers[GAS] = NOTHING;
			}
		}
	}
	
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			if (pmap[i][j].volume != newGasVolume[i][j]) {
				pmap[i][j].volume = newGasVolume[i][j];
				refreshDungeonCell(i, j);
			}
		}
	}
}

void updateEnvironment() {
	short i, j, direction, newX, newY, loc[2], x, y, promoteChance;
	enum dungeonLayers layer;
	floorTileType *tile;
	item *theItem, *nextItem, *previousItem;
	boolean isVolumetricGas = false;
	char buf[DCOLS], buf2[DCOLS];
	
	
	// update gases twice
	for (i=0; i<DCOLS && !isVolumetricGas; i++) {
		for (j=0; j<DROWS && !isVolumetricGas; j++) {
			if (!isVolumetricGas && pmap[i][j].layers[GAS]) {
				isVolumetricGas = true;
			}
		}
	}
	if (isVolumetricGas) {
		updateVolumetricMedia();
		updateVolumetricMedia();
	}
	
	// do random tile promotions
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			for (layer = 0; layer < NUMBER_TERRAIN_LAYERS; layer++) {
				tile = &(tileCatalog[pmap[i][j].layers[layer]]);
				if (tile->promoteChance < 0) {
					promoteChance = 0;
					for (direction = 0; direction < 4; direction++) {
						if (!cellHasTerrainFlag(i + nbDirs[direction][0], j + nbDirs[direction][1], OBSTRUCTS_PASSABILITY)
							&& pmap[i + nbDirs[direction][0]][j + nbDirs[direction][1]].layers[layer]
							!= pmap[i][j].layers[layer]
							&& !(pmap[i][j].flags & CAUGHT_FIRE_THIS_TURN)) {
							promoteChance += -1 * tile->promoteChance;
						}
					}
				} else {
					promoteChance = tile->promoteChance;
				}
				if (promoteChance && !(pmap[i][j].flags & CAUGHT_FIRE_THIS_TURN) && rand_range(0, 10000) < promoteChance) {
					promoteTile(i, j, layer, false);
					pmap[i][j].flags |= CAUGHT_FIRE_THIS_TURN;
				}
			}
		}
	}
	
	// bookkeeping
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			pmap[i][j].flags &= ~CAUGHT_FIRE_THIS_TURN;
			if (!(pmap[i][j].flags & (HAS_PLAYER | HAS_MONSTER | HAS_ITEM)) && pmap[i][j].flags & PRESSURE_PLATE_DEPRESSED) {
				pmap[i][j].flags &= ~PRESSURE_PLATE_DEPRESSED;
			}
		}
	}
	
	// update fire
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			if (cellHasTerrainFlag(i, j, IS_FIRE) && !(pmap[i][j].flags & CAUGHT_FIRE_THIS_TURN)) {
				exposeTileToFire(i, j, false);
				for (direction=0; direction<4; direction++) {
					newX = i + nbDirs[direction][0];
					newY = j + nbDirs[direction][1];
					exposeTileToFire(newX, newY, false);
				}
			}
		}
	}
	
	for (theItem=floorItems->nextItem; theItem != NULL; theItem = nextItem) {
		nextItem = theItem->nextItem;
		x = theItem->xLoc;
		y = theItem->yLoc;
		if ((cellHasTerrainFlag(x, y, IS_FIRE) && theItem->flags & ITEM_FLAMMABLE)
			 || (cellHasTerrainFlag(x, y, LAVA_INSTA_DEATH) && theItem->category != AMULET)) {
			burnItem(theItem);
			continue;
		}
		if (cellHasTerrainFlag(x, y, MOVES_ITEMS)) {
			getQualifyingLocNear(loc, x, y, 10, (OBSTRUCTS_ITEMS), (HAS_ITEM), false);
			pmap[x][y].flags &= ~HAS_ITEM;
			pmap[loc[0]][loc[1]].flags |= HAS_ITEM;
			if (pmap[x][y].flags & ITEM_DETECTED) {
				pmap[x][y].flags &= ~ITEM_DETECTED;
				pmap[loc[0]][loc[1]].flags |= ITEM_DETECTED;
			}
			theItem->xLoc = loc[0];
			theItem->yLoc = loc[1];
			refreshDungeonCell(x, y);
			refreshDungeonCell(loc[0], loc[1]);
		}
		if (cellHasTerrainFlag(x, y, TRAP_DESCENT)) {
			
			if (pmap[x][y].flags & VISIBLE) {
				itemName(theItem, buf, false, false);
				sprintf(buf2, "The %s plunge%s out of sight!", buf, (theItem->quantity > 1 ? "" : "s"));
				message(buf2, true, false);
			}
			theItem->flags |= ITEM_PREPLACED;
			
			// remove from item chain
			for (previousItem = floorItems;
				 previousItem->nextItem != theItem;
				 previousItem = previousItem->nextItem);
			previousItem->nextItem = theItem->nextItem;
			
			pmap[x][y].flags &= ~(HAS_ITEM | ITEM_DETECTED);

			if (theItem->category == POTION) {
				// potions don't survive the fall
				free(theItem);
			} else {
				// add to next level's chain
				theItem->nextItem = levels[rogue.depthLevel-1 + 1].items;
				levels[rogue.depthLevel-1 + 1].items = theItem;
			}
			refreshDungeonCell(x, y);
		}
		if (cellHasTerrainFlag(x, y, PROMOTES_ON_STEP)) {
			for (layer = 0; layer < NUMBER_TERRAIN_LAYERS; layer++) {
				if (tileCatalog[pmap[x][y].layers[layer]].flags & PROMOTES_ON_STEP) {
					promoteTile(x, y, layer, false);
				}
			}
		}
	}
}

void updateSafetyMap() {
	short i, j, **costMap;
	char playerPassMap[DCOLS][DROWS], monsterPassMap[DCOLS][DROWS];
	creature *monst;
	item *theItem;
	
	rogue.updatedSafetyMapThisTurn = true;
	
	costMap = allocDynamicGrid();
	
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			safetyMap[i][j] = 30000;
			
			if ((cellHasTerrainFlag(i, j, (OBSTRUCTS_PASSABILITY)) && !cellHasTerrainFlag(i, j, (IS_SECRET)))
				|| cellHasTerrainFlag(i, j, LAVA_INSTA_DEATH)) {
				playerPassMap[i][j] = monsterPassMap[i][j] = false;
				costMap[i][j] = PDS_OBSTRUCTION;
				continue;
			} else {
				if (pmap[i][j].flags & HAS_MONSTER) {
					monst = monsterAtLoc(i, j);
					if ((monst->creatureState == MONSTER_SLEEPING
						 || monst->turnsSpentStationary > 2
						 || monst->creatureState == MONSTER_ALLY)
						&& monst->creatureState != MONSTER_FLEEING) {
						playerPassMap[i][j] = true;
						monsterPassMap[i][j] = false;
						costMap[i][j] = PDS_OBSTRUCTION;
						continue;
					}
				} else if (pmap[i][j].flags & HAS_ITEM) {
					theItem = itemAtLoc(i, j);
					if (theItem->category & SCROLL && theItem->kind == SCROLL_SANCTUARY) {
						playerPassMap[i][j] = true;
						monsterPassMap[i][j] = false;
						costMap[i][j] = PDS_OBSTRUCTION;
						continue;
					}
				}
				
				if (cellHasTerrainFlag(i, j, (TRAP_DESCENT | IS_DF_TRAP | IS_FIRE))) {
					playerPassMap[i][j] = true;
					monsterPassMap[i][j] = true;
					costMap[i][j] = 300;
				} else if (cellHasTerrainFlag(i, j, (IS_DEEP_WATER | SPONTANEOUSLY_IGNITES))) {
					playerPassMap[i][j] = true;
					monsterPassMap[i][j] = true;
					costMap[i][j] = 200;
				} else {
					playerPassMap[i][j] = monsterPassMap[i][j] = true;
					costMap[i][j] = 1;
				}
			}
		}
	}
	
	safetyMap[player.xLoc][player.yLoc] = 0;
	playerPassMap[player.xLoc][player.yLoc] = true;
	monsterPassMap[player.xLoc][player.yLoc] = false;
	costMap[player.xLoc][player.yLoc] = PDS_OBSTRUCTION;
	
	dijkstraScan(safetyMap, NULL, playerPassMap, false);
	
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			if (!monsterPassMap[i][j]) {
				continue;
			}
			
			if (safetyMap[i][j] == 30000) {
				safetyMap[i][j] = 150;
			}
			
			safetyMap[i][j] = 50 * safetyMap[i][j] / (50 + safetyMap[i][j]);
			
			safetyMap[i][j] *= -3;
		}
	}
	
	monsterPassMap[player.xLoc][player.yLoc] = false;
	
	dijkstraScan(safetyMap, costMap, monsterPassMap, false);
	
	freeDynamicGrid(costMap);
}

void rechargeStaffs() {
	item *theItem;
	char buf[DCOLS], buf2[DCOLS];
	short rechargeIncrement;
	
	if (rogue.armor && rogue.armor->enchant2 == A_WISDOM && rogue.armor->flags & ITEM_RUNIC) {
		rechargeIncrement = (int) 10 * pow(1.3, min(27, rogue.armor->enchant1)); // at level 27, you recharge anything to full in one turn
	} else {
		rechargeIncrement = 10;
	}
	
	for (theItem = packItems->nextItem; theItem != NULL; theItem = theItem->nextItem) {
		if (theItem->category & STAFF) {
			if (theItem->enchant2 <= 0 && theItem->charges < theItem->enchant1) {
				// if it's time for a recharge
				theItem->charges++;
				// staves of blinking and obstruction recharge half as fast so they're less powerful
				theItem->enchant2 = (theItem->kind == STAFF_BLINKING || theItem->kind == STAFF_OBSTRUCTION ? 10000 : 5000) / theItem->enchant1;
			} else if (theItem->charges < theItem->enchant1) {
				theItem->enchant2 -= rechargeIncrement;
			}
		}
	}
	
	if (rogue.armor
		&& rogue.armor->charges > 0
		&& !(rogue.armor->flags & ITEM_IDENTIFIED)) {
		
		rogue.armor->charges--;
		if (rogue.armor->charges <= 0) {
			rogue.armor->flags |= ITEM_IDENTIFIED;
			if (!(rogue.armor->flags & ITEM_RUNIC) || (rogue.armor->flags & ITEM_RUNIC_IDENTIFIED)) {
				rogue.armor->flags &= ~ITEM_CAN_BE_IDENTIFIED;
			}
			message("you are now familiar enough with your armor to identify it.", true, false);
			itemName(rogue.armor, buf2, true, true);
			sprintf(buf, "%s %s.", (rogue.armor->quantity > 1 ? "they are" : "it is"), buf2);
			message(buf, true, false);
		}
	}
}

void extinguishFireOnCreature(creature *monst) {
	char buf[COLS], buf2[COLS];
	
	monst->status.burning = 0;
	if (monst->statusLight) {
		deleteLight(monst->statusLight);
	}
	monst->statusLight = NULL;
	if (monst == &player) {
		player.info.foreColor = &white;
		rogue.minersLight->lightColor = &minersLightColor;
		refreshDungeonCell(player.xLoc, player.yLoc);
		updateVision();
		message("you are no longer on fire.", true, false);
	} else if (canSeeMonster(monst)) {
		monsterName(buf, monst, true);
		sprintf(buf2, "%s is no longer on fire.", buf);
		message(buf2, true, false);
	}
}

void monstersApproachStairs() {
	creature *monst, *prevMonst, *nextMonst;
	short n;
	char monstName[COLS], buf[COLS];
	boolean monsterDied;
	
	for (n = rogue.depthLevel - 2; n <= rogue.depthLevel; n += 2) { // cycle through previous and next level
		if (n >= 0 && levels[n].visited) {
			for (monst = levels[n].monsters; monst != NULL;) {
				nextMonst = monst->nextCreature;
				monsterDied = false;
				if (monst->status.entersLevelIn > 1) {
					monst->status.entersLevelIn--;
				} else if (monst->status.entersLevelIn == 1) {
					// monster is entering the level!
					
					// remove traversing monster from other level monster chain
					if (monst == levels[n].monsters) {
						levels[n].monsters = monst->nextCreature;
					} else {
						for (prevMonst = levels[n].monsters; prevMonst->nextCreature != monst; prevMonst = prevMonst->nextCreature);
						prevMonst->nextCreature = monst->nextCreature;
					}
					
					// prepend traversing monster to current level monster chain
					monst->nextCreature = monsters->nextCreature;
					monsters->nextCreature = monst;
					
					// place traversing monster near the stairs on this level
					if (monst->bookkeepingFlags & MONST_APPROACHING_DOWNSTAIRS) {
						monst->xLoc = rogue.upLoc[0];
						monst->yLoc = rogue.upLoc[1];
					} else if (monst->bookkeepingFlags & MONST_APPROACHING_UPSTAIRS) {
						monst->xLoc = rogue.downLoc[0];
						monst->yLoc = rogue.downLoc[1];					
					} else { // jumping down pit
						monst->xLoc = levels[n].playerExitedVia[0];
						monst->yLoc = levels[n].playerExitedVia[1];
						monsterName(monstName, monst, true);
						if (!monst->status.levitating) {
							if (inflictDamage(NULL, monst, randClumpedRange(6, 12, 2), &red)) {
								monsterDied = true;
								if (canSeeMonster(monst)) {
									sprintf(buf, "%s plummets from above and splatters against the ground!", monstName);
									message(buf, true, false);
								}
							} else {
								if (canSeeMonster(monst)) {
									sprintf(buf, "%s falls from above and crashes to the ground!", monstName);
									message(buf, true, false);
								}
							}
						} else if (canSeeMonster(monst)) {
							sprintf(buf, "%s swoops into the cavern from above.", monstName);
							message(buf, true, false);
						}
					}
					if (!monsterDied) {
						monst->status.entersLevelIn = 0;
						monst->bookkeepingFlags |= MONST_PREPLACED;
						restoreMonster(monst, NULL, NULL);
						monst->ticksUntilTurn = monst->movementSpeed;
						refreshDungeonCell(monst->xLoc, monst->yLoc);
					}
				}
				monst = nextMonst;
			}
		}
	}
}

void decrementPlayerStatus() {
	creature *monst;
	
	if (numberOfMatchingPackItems(AMULET, 0, 0, false)) {
		player.status.nutrition -= rand_percent(20); // 1/5 digestion rate when you have the amulet
	} else {
		player.status.nutrition--;
	}
	
	if (player.status.telepathic > 0 && !--player.status.telepathic) {
		for (monst=monsters->nextCreature; monst != NULL; monst = monst->nextCreature) {
			refreshDungeonCell(monst->xLoc, monst->yLoc);
		}
		updateVision();
		message("your preternatural mental sensitivity fades.", true, false);
	}
	
	if (player.status.blind > 0 && !--player.status.blind) {
		updateVision();
		message("you can see again.", true, false);
	}
	
	if (player.status.darkness > 0) {
		player.status.darkness--;
		updateMinersLightRadius();
		updateVision();
	}
	
	if (player.status.hallucinating > 0 && !--player.status.hallucinating) {
		displayLevel();
		message("your hallucinations fade.", true, false);
	}
	
	if (player.status.levitating > 0 && !--player.status.levitating) {
		message("you are no longer levitating.", true, false);
	}
	
	if (player.status.confused > 0 && !--player.status.confused) {
		message("you no longer feel confused.", true, false);
	}
	
	if (player.status.nauseous > 0 && !--player.status.nauseous) {
		message("you feel less nauseous.", true, false);
	}
	
	if (player.status.paralyzed > 0 && !--player.status.paralyzed) {
		message("you can move again.", true, false);
	}
	
	if (player.status.hasted > 0 && !--player.status.hasted) {
		player.movementSpeed = player.info.movementSpeed;
		player.attackSpeed = player.info.attackSpeed;
		message("your supernatural speed fades.", true, false);
	}
	
	if (player.status.slowed > 0 && !--player.status.slowed) {
		player.movementSpeed = player.info.movementSpeed;
		player.attackSpeed = player.info.attackSpeed;
		message("your normal speed resumes.", true, false);
	}
	
	if (player.status.immuneToFire > 0 && !--player.status.immuneToFire) {
		player.info.flags &= ~MONST_IMMUNE_TO_FIRE;
		message("you no longer feel immune to fire.", true, false);
	}
	
	if (player.status.stuck && !cellHasTerrainFlag(player.xLoc, player.yLoc, ENTANGLES)) {
		player.status.stuck = 0;
	}
	
	if (player.status.explosionImmunity) {
		player.status.explosionImmunity--;
	}
	
	if (player.status.discordant) {
		player.status.discordant--;
	}
	
	if (rogue.monsterSpawnFuse <= 0) {
		spawnPeriodicHorde();
		rogue.monsterSpawnFuse = rand_range(125, 175);
	}
}

void autoRest() {
	short i = 0;
	rogue.disturbed = false;
	rogue.automationActive = true;
	if ((player.currentHP < player.info.maxHP
		 || player.status.blind
		 || player.status.hallucinating
		 || player.status.confused
		 || player.status.nauseous
		 || player.status.poisoned
		 || player.status.darkness
		 || cellHasTerrainFlag(player.xLoc, player.yLoc, OBSTRUCTS_PASSABILITY))
		&& !rogue.disturbed && i++ < TURNS_FOR_FULL_REGEN) {
		while ((player.currentHP < player.info.maxHP
				|| player.status.blind
				|| player.status.hallucinating
				|| player.status.confused
				|| player.status.nauseous
				|| player.status.poisoned
				|| player.status.darkness
				|| cellHasTerrainFlag(player.xLoc, player.yLoc, OBSTRUCTS_PASSABILITY))
			   && !rogue.disturbed && i++ < TURNS_FOR_FULL_REGEN) {
			rogue.justRested = true;
			playerTurnEnded();
			refreshSideBar(NULL);
			if (pauseBrogue(1)) {
				rogue.disturbed = true;
			}
			refreshSideBar(NULL);
		}
	} else {
		for (i=0; i<100 && !rogue.disturbed; i++) {
			rogue.justRested = true;
			playerTurnEnded();
			refreshSideBar(NULL);
			if (pauseBrogue(1)) {
				rogue.disturbed = true;
			}
			refreshSideBar(NULL);
		}
	}
	rogue.automationActive = false;
}

short directionOfKeypress(unsigned short ch) {
	switch (ch) {
		case LEFT_KEY:
		case LEFT_ARROW:
		case NUMPAD_4:
			return LEFT;
		case RIGHT_KEY:
		case RIGHT_ARROW:
		case NUMPAD_6:
			return RIGHT;
		case UP_KEY:
		case UP_ARROW:
		case NUMPAD_8:
			return UP;
		case DOWN_KEY:
		case DOWN_ARROW:
		case NUMPAD_2:
			return DOWN;
		case UPLEFT_KEY:
		case NUMPAD_7:
			return UPLEFT;
		case UPRIGHT_KEY:
		case NUMPAD_9:
			return UPRIGHT;
		case DOWNLEFT_KEY:
		case NUMPAD_1:
			return DOWNLEFT;
		case DOWNRIGHT_KEY:
		case NUMPAD_3:
			return DOWNRIGHT;
		default:
			return -1;
	}
}

void startFighting(enum directions dir, boolean tillDeath) {
	short x, y, expectedDamage;
	creature *monst;
	
	x = player.xLoc + nbDirs[dir][0];
	y = player.yLoc + nbDirs[dir][1];
	
	monst = monsterAtLoc(x, y);
	
	// weighted average: expect 75% of the maximum damage from the monster
	expectedDamage = monst->info.damage.upperBound;
	if (rogue.depthLevel == 1) {
		expectedDamage /= 2;
	}
	
	if (rogue.easyMode) {
		expectedDamage /= 2;
	}
	
	rogue.blockCombatText = true;
	rogue.disturbed = false;
	
	if (monst->creatureState == MONSTER_ALLY) {
		monst->creatureState = MONSTER_TRACKING_SCENT;
	}
	
	do {
		if (!playerMoves(dir)) {
			break;
		}
		if (pauseBrogue(1)) {
			break;
		}
	} while (!rogue.disturbed && !rogue.gameHasEnded && (tillDeath || player.currentHP > expectedDamage)
			 && pmap[x][y].flags & HAS_MONSTER && monsterAtLoc(x, y) == monst);
	
	rogue.blockCombatText = false;
}

void autoFight(boolean tillDeath) {
	short x, y, dir;
	creature *monst;
	
	if (player.status.blind) {
		message("Not while you're blind.", true, false);
		return;
	}
	if (player.status.hallucinating && !tillDeath) {
		message("Not while you're hallucinating.", true, false);
		return;
	}
	if (player.status.confused) {
		message("Not while you're confused.", true, false);
		return;
	}
	
	confirmMessages();
	temporaryMessage("Fight what? (<hjklyubn> to select direction)", false);
	dir = directionOfKeypress(nextKeyPress());
	confirmMessages();
	
	if (dir == -1) {
		return;
	}
	
	x = player.xLoc + nbDirs[dir][0];
	y = player.yLoc + nbDirs[dir][1];
	
	monst = monsterAtLoc(x, y);
	
	if (!monst
		|| monst->info.flags & MONST_INVISIBLE
		|| monst->bookkeepingFlags & MONST_SUBMERGED
		|| !(pmap[x][y].flags & (VISIBLE | CLAIRVOYANT_VISIBLE))) {
		message("I see no monster there.", true, false);
		return;
	}
	
	startFighting(dir, tillDeath);
}

void playerTurnEnded() {
	short soonestTurn, damage, turnsRequiredToShore, turnsToShore;
	char buf[COLS], buf2[COLS];
	creature *monst, *monst2, *previousMonster, *nextMonst;
	boolean fastForward = false;
	
	do {
		
		if (rogue.gameHasEnded) {
			return;
		}
		
		rogue.turnNumber++;
		
		rogue.scentTurnNumber += 3; // this must happen per subjective player time
		if (rogue.scentTurnNumber > 60000) {
			resetTurnNumber();
		}
		
		//updateFlavorText();
		
		if (player.currentHP < player.info.maxHP && !player.status.poisoned) {
			if ((player.turnsUntilRegen -= 1000) <= 0) {
				player.currentHP++;
				player.turnsUntilRegen += player.info.turnsBetweenRegen;
			}
			if (player.regenPerTurn) {
				player.currentHP += player.regenPerTurn;
			}
		}
		
		if (rogue.awarenessBonus > 0) {
			search(rogue.awarenessBonus);
		}
		
		for (monst = monsters->nextCreature; monst != NULL; monst = nextMonst) {
			nextMonst = monst->nextCreature;
			if ((monst->bookkeepingFlags & MONST_BOUND_TO_LEADER)
				&& (!monst->leader || !(monst->bookkeepingFlags & MONST_FOLLOWER))) {
				killCreature(monst);
				if (canSeeMonster(monst)) {
					monsterName(buf2, monst, true);
					sprintf(buf, "%s dissipates into thin air", buf2);
					combatMessage(buf);
				}
			}
		}
		
		if (player.status.burning > 0) {
			damage = rand_range(1, 3);
			if (!(player.info.flags & MONST_IMMUNE_TO_FIRE) && inflictDamage(NULL, &player, damage, &orange)) {
				gameOver("Burned to death", true);
			}
			if (!--player.status.burning) {
				player.status.burning++; // ugh
				extinguishFireOnCreature(&player);
			}
		}
		
		if (player.status.poisoned > 0) {
			player.status.poisoned--;
			if (inflictDamage(NULL, &player, 1, &green)) {
				gameOver("Died from poison", true);
			}
		}
		
		if (player.ticksUntilTurn == 0) { // attacking adds ticks elsewhere
			player.ticksUntilTurn += player.movementSpeed;
		}
		
		updateScent();
		rogue.updatedSafetyMapThisTurn = false;
		for (monst = monsters->nextCreature; monst != NULL; monst = monst->nextCreature) {
			if (D_SAFETY_VISION || monst->creatureState == MONSTER_FLEEING && pmap[monst->xLoc][monst->yLoc].flags & IN_FIELD_OF_VIEW) {	
				updateSafetyMap(); // only if there is a fleeing monster who can see the player
				break;
			}
		}
		
		if (D_BULLET_TIME && !rogue.justRested) {
			player.ticksUntilTurn = 0;
		}
		
		applyGradualTileEffectsToCreature(&player, player.ticksUntilTurn);
		
		if (rogue.gameHasEnded) {
			return;
		}
		
		rogue.heardCombatThisTurn = false;
		
		while (player.ticksUntilTurn > 0) {
			
			soonestTurn = 10000;
			
			for(monst = monsters->nextCreature; monst != NULL; monst = monst->nextCreature) {
				soonestTurn = min(soonestTurn, monst->ticksUntilTurn);
			}
			
			soonestTurn = min(soonestTurn, player.ticksUntilTurn);
			
			soonestTurn = min(soonestTurn, rogue.ticksTillUpdateEnvironment);
			
			for(monst = monsters->nextCreature; monst != NULL; monst = monst->nextCreature) {
				monst->ticksUntilTurn -= soonestTurn;
			}
			
			rogue.ticksTillUpdateEnvironment -= soonestTurn;
			
			if (rogue.ticksTillUpdateEnvironment <= 0) {
				rogue.ticksTillUpdateEnvironment += 100;
				
				// stuff that happens periodically according to an objective time measurement goes here:
				rechargeStaffs(); // staffs recharge every so often
				rogue.monsterSpawnFuse--; // monsters spawn in the level every so often
				
				for(monst = monsters->nextCreature; monst != NULL;) {
					nextMonst = monst->nextCreature;
					applyInstantTileEffectsToCreature(monst);
					monst = nextMonst; // this weirdness is in case the monster dies in the previous step
				}
				
				for(monst = monsters->nextCreature; monst != NULL;) {
					nextMonst = monst->nextCreature;
					decrementMonsterStatus(monst);
					monst = nextMonst;
				}
				
				// monsters with a dungeon feature spawn it every so often
				for (monst = monsters->nextCreature; monst != NULL;) {
					nextMonst = monst->nextCreature;
					if (monst->info.DFChance && rand_percent(monst->info.DFChance)) {
						spawnDungeonFeature(monst->xLoc, monst->yLoc, &dungeonFeatureCatalog[monst->info.DFType], true);
					}
					monst = nextMonst;
				}
				
				decrementPlayerStatus();
				applyInstantTileEffectsToCreature(&player);
				if (rogue.gameHasEnded) { // poison gas, lava, trapdoor, etc.
					return;
				}
				monstersApproachStairs();
				updateEnvironment(); // Update fire and gas, items floating around in water, etc.
			}
			
			previousMonster = monsters;
			for(monst = monsters->nextCreature; (monst != NULL) && (rogue.gameHasEnded == false); monst = monst->nextCreature) {
				if (monst->ticksUntilTurn <= 0) {
					
					monstersTurn(monst);
					
					for(monst2 = monsters->nextCreature; monst2 != NULL; monst2 = monst2->nextCreature) {
						if (monst2 == monst) { // monst still alive and on the level
							applyGradualTileEffectsToCreature(monst, monst->ticksUntilTurn);
							break;
						}
					}
					
					monst = monsters;// loop through from the beginning to be safe
				}
			}
			
			player.ticksUntilTurn -= soonestTurn;
			
			
			if (rogue.gameHasEnded) {
				return;
			}
		}
		// DEBUG displayLevel();
		//checkForDungeonErrors();
		
		updateVision();
		
		for(monst = monsters->nextCreature; monst != NULL; monst = monst->nextCreature) {
			if (canSeeMonster(monst) && !(monst->bookkeepingFlags & MONST_WAS_VISIBLE)) {
				monst->bookkeepingFlags |= MONST_WAS_VISIBLE;
				if (monst->creatureState != MONSTER_ALLY) {
					rogue.disturbed = true;
					if (rogue.cautiousMode) {
						monsterName(buf2, monst, false);
						sprintf(buf, "you see a %s.", buf2);
						message(buf, true, true);
					}
				}
				if (cellHasTerrainFlag(monst->xLoc, monst->yLoc, OBSTRUCTS_PASSABILITY)
					&& cellHasTerrainFlag(monst->xLoc, monst->yLoc, IS_SECRET)) {
					discover(monst->xLoc, monst->yLoc);
				}
				if (rogue.weapon && rogue.weapon->flags & ITEM_RUNIC
					&& rogue.weapon->enchant2 == W_SLAYING
					&& !(rogue.weapon->flags & ITEM_RUNIC_HINTED)
					&& rogue.weapon->vorpalEnemy == monst->info.monsterID) {
					rogue.weapon->flags |= ITEM_RUNIC_HINTED;
					itemName(rogue.weapon, buf2, false, false);
					sprintf(buf, "the runes on your %s gleam balefully.", buf2);
					message(buf, true, true);
				}
				if (rogue.armor && rogue.armor->flags & ITEM_RUNIC
					&& rogue.armor->enchant2 == A_IMMUNITY
					&& !(rogue.armor->flags & ITEM_RUNIC_HINTED)
					&& rogue.armor->vorpalEnemy == monst->info.monsterID) {
					rogue.armor->flags |= ITEM_RUNIC_HINTED;
					itemName(rogue.armor, buf2, false, false);
					sprintf(buf, "the runes on your %s glow protectively.", buf2);
					message(buf, true, true);
				}
			} else if (!canSeeMonster(monst) && monst->bookkeepingFlags & MONST_WAS_VISIBLE) {
				monst->bookkeepingFlags &= ~MONST_WAS_VISIBLE;
			}
		}
		
		if (rogue.gainedLevel) {
			rogue.gainedLevel = false;
			sprintf(buf, "welcome to level %i.", rogue.experienceLevel);
			message(buf, true, false);
		}
		
		displayCombatText();
		
		if (player.status.paralyzed) {
			if (!fastForward) {
				fastForward = pauseBrogue(25);
			}
		}

		checkNutrition();
		shuffleTerrainColors(100, false);
		
	} while (player.status.paralyzed);
	
	rogue.justRested = false;
	updateFlavorText();
	
	// "point of no return" check
	if ((player.status.levitating && cellHasTerrainFlag(player.xLoc, player.yLoc, LAVA_INSTA_DEATH | IS_DEEP_WATER | TRAP_DESCENT))
		|| (player.status.immuneToFire && cellHasTerrainFlag(player.xLoc, player.yLoc, LAVA_INSTA_DEATH))) {
		if (!rogue.receivedLevitationWarning) {
			turnsRequiredToShore = rogue.mapToShore[player.xLoc][player.yLoc] * player.movementSpeed / 100;
			if (cellHasTerrainFlag(player.xLoc, player.yLoc, LAVA_INSTA_DEATH)) {
				turnsToShore = max(player.status.levitating, player.status.immuneToFire) * 100 / player.movementSpeed;
			} else {
				turnsToShore = player.status.levitating * 100 / player.movementSpeed;
			}
			if (turnsRequiredToShore == turnsToShore || turnsRequiredToShore + 1 == turnsToShore) {
				message("better head back to solid ground!", true, true);
				rogue.receivedLevitationWarning = true;
			} else if (turnsRequiredToShore > turnsToShore) {
				message("you're past the point of no return!", true, true);
				rogue.receivedLevitationWarning = true;
			}
		}
	} else {
		rogue.receivedLevitationWarning = false;
	}
	
	emptyGraveyard();
}

void resetTurnNumber() { // don't want player.scentTurnNumber to roll over the unsigned short maxint!
	short i, j;
	rogue.scentTurnNumber -= 50000;
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			tmap[i][j].scent = max(0, tmap[i][j].scent - 50000);
		}
	}
}

boolean isDisturbed(short x, short y) {
	short i;
	creature *monst;
	for (i=0; i<8; i++) {
		monst = monsterAtLoc(x + nbDirs[i][0], y + nbDirs[i][1]);
		if (pmap[x + nbDirs[i][0]][y + nbDirs[i][1]].flags & (HAS_ITEM)) {
			return true; // do not trigger for submerged or invisible or unseen monsters
		}
		if (monst && !(monst->creatureState == MONSTER_ALLY)
			&& (canSeeMonster(monst) || player.status.telepathic)) { // do not trigger for submerged or invisible or unseen monsters
			return true;
		}
	}
	return false;
}

void discover(short x, short y) {
	enum dungeonLayers layer;
	dungeonFeature *feat;
	if (cellHasTerrainFlag(x, y, IS_SECRET)) {
		
		for (layer = 0; layer < NUMBER_TERRAIN_LAYERS; layer++) {
			if (tileCatalog[pmap[x][y].layers[layer]].flags & IS_SECRET) {
				feat = &dungeonFeatureCatalog[tileCatalog[pmap[x][y].layers[layer]].discoverType];
				pmap[x][y].layers[layer] = (layer == DUNGEON ? FLOOR : NOTHING);
				spawnDungeonFeature(x, y, feat, true);
			}
		}
		refreshDungeonCell(x, y);
	}
}

// returns true if found anything
boolean search(short searchStrength) {
	short i, j, radius, x, y;
	radius = searchStrength / 10;
	x = player.xLoc;
	y = player.yLoc;
	
	for (i = x - radius; i <= x + radius; i++) {
		for (j = y - radius; j <= y + radius; j++) {
			if (coordinatesAreInMap(i, j)
				&& pmap[i][j].flags & VISIBLE
				&& tileCatalog[pmap[i][j].layers[DUNGEON]].flags & IS_SECRET
				&& rand_percent(min(100, searchStrength - distanceBetween(x, y, i, j) * 10))) {
				
				discover(i, j);
				
				return true;
			}
		}	
	}
	return false;
}

boolean useStairs(short stairDirection) {
	if (stairDirection == 1) {
		if (D_WORMHOLING || cellHasTerrainFlag(player.xLoc, player.yLoc, PERMITS_DESCENT)) {
			rogue.depthLevel++;
			startLevel(rogue.depthLevel - 1, stairDirection);
			message("You descend.", true, false);
			return true;
		} else if (pmap[rogue.downLoc[0]][rogue.downLoc[1]].flags & (DISCOVERED | MAGIC_MAPPED)) {
			travel(rogue.downLoc[0], rogue.downLoc[1], false);
			return false;
		} else {
			message("I see no way down.", true, false);
			return false;
		}
	} else {
		if (D_WORMHOLING || cellHasTerrainFlag(player.xLoc, player.yLoc, PERMITS_ASCENT)) {
			if (rogue.depthLevel > 1 || numberOfMatchingPackItems(AMULET, 0, 0, false)) {
				rogue.depthLevel--;
				if (rogue.depthLevel == 0) {
					victory();
					return true;
				}
				startLevel(rogue.depthLevel + 1, stairDirection);
				message("You ascend.", true, false);
				return true;
			} else {
				confirmMessages();
				message("your way is magically blocked!", true, false);
				return false;
			}
		} else if (pmap[rogue.upLoc[0]][rogue.upLoc[1]].flags & (DISCOVERED | MAGIC_MAPPED)) {
			travel(rogue.upLoc[0], rogue.upLoc[1], false);
			return false;
		} else {
			message("I see no way up.", true, false);
			return false;
		}
	}
}

void updateFieldOfViewDisplay(boolean updateDancingTerrain) {
	short i, j;
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			if (pmap[i][j].flags & IN_FIELD_OF_VIEW
				&& (tmap[i][j].light[0] + tmap[i][j].light[1] + tmap[i][j].light[2] > VISIBILITY_THRESHOLD)
				&& !(pmap[i][j].flags & CLAIRVOYANT_DARKENED)
				&& !player.status.blind) {
				pmap[i][j].flags |= VISIBLE;
			}
			
			if (pmap[i][j].flags & VISIBLE && !(pmap[i][j].flags & WAS_VISIBLE) ) { // if the cell became visible this move
				pmap[i][j].flags |= DISCOVERED;
				pmap[i][j].flags &= ~STABLE_MEMORY;
				refreshDungeonCell(i, j);
			} else if (!(pmap[i][j].flags & VISIBLE) && pmap[i][j].flags & WAS_VISIBLE) { // if the cell ceased being visible this move
				refreshDungeonCell(i, j);
			} else if (!(pmap[i][j].flags & CLAIRVOYANT_VISIBLE) && pmap[i][j].flags & WAS_CLAIRVOYANT_VISIBLE) { // ceased being clairvoyantly visible
				refreshDungeonCell(i, j);
			} else if (!(pmap[i][j].flags & WAS_CLAIRVOYANT_VISIBLE) && pmap[i][j].flags & CLAIRVOYANT_VISIBLE) { // became clairvoyantly visible
				pmap[i][j].flags &= ~STABLE_MEMORY;
				refreshDungeonCell(i, j);
			} else if (pmap[i][j].flags & VISIBLE &&
					   (tmap[i][j].light[0] != tmap[i][j].oldLight[0] ||
						tmap[i][j].light[1] != tmap[i][j].oldLight[1] ||
						tmap[i][j].light[2] != tmap[i][j].oldLight[2])) { // if the cell's light color changed this move
						   refreshDungeonCell(i, j);
					   } else if (updateDancingTerrain
								  && pmap[i][j].flags & (VISIBLE | CLAIRVOYANT_VISIBLE)
								  && (!rogue.automationActive || !(rogue.turnNumber % 10))
								  && (tileCatalog[pmap[i][j].layers[DUNGEON]].foreColor->colorDances
									  || tileCatalog[pmap[i][j].layers[DUNGEON]].backColor->colorDances
									  || (tileCatalog[pmap[i][j].layers[LIQUID]].backColor) && tileCatalog[pmap[i][j].layers[LIQUID]].backColor->colorDances
									  || (tileCatalog[pmap[i][j].layers[LIQUID]].foreColor) && tileCatalog[pmap[i][j].layers[LIQUID]].foreColor->colorDances
									  || (tileCatalog[pmap[i][j].layers[SURFACE]].backColor) && tileCatalog[pmap[i][j].layers[SURFACE]].backColor->colorDances
									  || (tileCatalog[pmap[i][j].layers[SURFACE]].foreColor) && tileCatalog[pmap[i][j].layers[SURFACE]].foreColor->colorDances
									  || (tileCatalog[pmap[i][j].layers[GAS]].backColor) && tileCatalog[pmap[i][j].layers[GAS]].backColor->colorDances
									  || (tileCatalog[pmap[i][j].layers[GAS]].foreColor) && tileCatalog[pmap[i][j].layers[GAS]].foreColor->colorDances
									  || (player.status.hallucinating && pmap[i][j].flags & (VISIBLE)))) {
									  
									  pmap[i][j].flags &= ~STABLE_MEMORY;
									  refreshDungeonCell(i, j);
						}
		}
	}
}

/*void updateFieldOfView(short xLoc, short yLoc, short radius, boolean paintScent, boolean setFieldOfView,
					   boolean passThroughCreatures, short theColor[3], short fadeToPercent) {
	short i, j;
	char grid[DCOLS][DROWS];
	
	if (setFieldOfView) {
		for (i=0; i<DCOLS; i++) {
			for (j=0; j<DROWS; j++) {
				pmap[i][j].flags &= ~WAS_VISIBLE;
				if (pmap[i][j].flags & VISIBLE) {
					pmap[i][j].flags &= ~VISIBLE;
					pmap[i][j].flags |= WAS_VISIBLE;
				}
				pmap[i][j].flags &= ~IN_FIELD_OF_VIEW;
			}
		}
	}
	
	for (i=1; i<=8; i++) {
		scanOctantFOV(xLoc, yLoc, i, radius, 1, LOS_SLOPE_GRANULARITY * -1,	0, paintScent, setFieldOfView,
					  passThroughCreatures, false, theColor, fadeToPercent);
	}
	
	if (setFieldOfView) {
		pmap[xLoc][yLoc].flags |= IN_FIELD_OF_VIEW;
	}
	if (paintScent) {
		addScentToCell(xLoc, yLoc, 0);
	}
}*/

//	Octants:
//		\7|8/
//		6\|/1
//		--@--
//		5/|\2
//		/4|3\

void betweenOctant1andN(short *x, short *y, short x0, short y0, short n) {
	short x1 = *x, y1 = *y;
	short dx = x1 - x0, dy = y1 - y0;
	switch (n) {
		case 1:
			return;
		case 2:
			*y = y0 - dy;
			return;
		case 5:
			*x = x0 - dx;
			*y = y0 - dy;
			return;
		case 6:
			*x = x0 - dx;
			return;
		case 8:
			*x = x0 - dy;
			*y = y0 - dx;
			return;
		case 3:
			*x = x0 - dy;
			*y = y0 + dx;
			return;
		case 7:
			*x = x0 + dy;
			*y = y0 - dx;
			return;
		case 4:
			*x = x0 + dy;
			*y = y0 + dx;
			return;
	}
}

// Returns a boolean grid indicating whether each square is in the field of view of (xLoc, yLoc).
// forbiddenTerrain is the set of terrain flags that will block vision (but note that the blocking cell itself is
// illuminated); forbiddenFlags is the set of map flags that will block vision.
// If cautiousOnWalls is set, we will not illuminate blocking tiles unless the tile one space closer to the origin
// is visible to the player; this is to prevent lights from illuminating a wall when the player is on the other
// side of the wall.
void getFOVMask(boolean grid[DCOLS][DROWS], short xLoc, short yLoc, short maxRadius,
				unsigned long forbiddenTerrain,	unsigned long forbiddenFlags, boolean cautiousOnWalls) {
	short i;
	
	for (i=1; i<=8; i++) {
		scanOctantFOV(grid, xLoc, yLoc, i, maxRadius, 1, LOS_SLOPE_GRANULARITY * -1, 0,
					  forbiddenTerrain, forbiddenFlags, cautiousOnWalls);
	}
}

void scanOctantFOV(boolean grid[DCOLS][DROWS], short xLoc, short yLoc, short octant, short maxRadius,
				   short columnsRightFromOrigin, long startSlope, long endSlope, unsigned long forbiddenTerrain,
				   unsigned long forbiddenFlags, boolean cautiousOnWalls) {
	
	if (columnsRightFromOrigin >= maxRadius) return;
	
	short i, a, b, iStart, iEnd, x, y, x2, y2; // x and y are temporary variables on which we do the octant transform
	long newStartSlope, newEndSlope;
	boolean cellObstructed;
	
	newStartSlope = startSlope;
	
	a = ((LOS_SLOPE_GRANULARITY / -2 + 1) + startSlope * columnsRightFromOrigin) / LOS_SLOPE_GRANULARITY;
	b = ((LOS_SLOPE_GRANULARITY / -2 + 1) + endSlope * columnsRightFromOrigin) / LOS_SLOPE_GRANULARITY;
	
	iStart = min(a, b);
	iEnd = max(a, b);
	
	// restrict vision to a circle of radius maxRadius
	if ((columnsRightFromOrigin*columnsRightFromOrigin + iEnd*iEnd) >= maxRadius*maxRadius) {
		return;
	}
	if ((columnsRightFromOrigin*columnsRightFromOrigin + iStart*iStart) >= maxRadius*maxRadius) {
		iStart = (int) (-1 * sqrt(maxRadius*maxRadius - columnsRightFromOrigin*columnsRightFromOrigin));
	}
	
	x = xLoc + columnsRightFromOrigin;
	y = yLoc + iStart;
	betweenOctant1andN(&x, &y, xLoc, yLoc, octant);
	
	boolean currentlyLit = coordinatesAreInMap(x, y) && !(cellHasTerrainFlag(x, y, forbiddenTerrain) ||
														  (pmap[x][y].flags & forbiddenFlags));
	
	for (i = iStart; i <= iEnd; i++) {
		
		x = xLoc + columnsRightFromOrigin;
		y = yLoc + i;
		betweenOctant1andN(&x, &y, xLoc, yLoc, octant);
		
		if (x < 0 || x >= DCOLS || y < 0 || y >= DROWS) {
			// We're off the map -- here there be memory corruption. Turn back.
			// This should happen only if a light source is placed on the outer edge of the tmap -- which is permitted,
			// because this fix will handle that situation gracefully.
			return;
		}

		cellObstructed = (cellHasTerrainFlag(x, y, forbiddenTerrain) || (pmap[x][y].flags & forbiddenFlags));
		
		// if we're cautious on walls and this is a wall:
		if (cautiousOnWalls && cellObstructed) {
			
			// (x2, y2) is the tile one space closer to the origin from the tile we're on:
			x2 = xLoc + columnsRightFromOrigin - 1;
			y2 = yLoc + i;
			if (i < 0) {
				y2++;
			} else if (i > 0) {
				y2--;
			}
			betweenOctant1andN(&x2, &y2, xLoc, yLoc, octant);
			
			if (pmap[x2][y2].flags & IN_FIELD_OF_VIEW) {
				// previous tile is visible, so illuminate
				grid[x][y] = 1;
			}
			
			//if ((player.xLoc - x) * (xLoc - x) + (player.yLoc - y) * (yLoc - y) >= 0) { // dot product
			//	grid[x][y] = 1;
			//}
		} else {
			// illuminate
			grid[x][y] = 1;
		}
		
		if (!cellObstructed && !currentlyLit) { // next column slope starts here
			
			newStartSlope = (long int) ((LOS_SLOPE_GRANULARITY * (i) - LOS_SLOPE_GRANULARITY / 2)
			/ (columnsRightFromOrigin + 0.5));
			
			currentlyLit = true;
		
		} else if (cellObstructed && currentlyLit) { // next column slope ends here
			
			newEndSlope = (long int) ((LOS_SLOPE_GRANULARITY * (i) - LOS_SLOPE_GRANULARITY / 2)
							/ (columnsRightFromOrigin - 0.5));
			
			// run next column
			scanOctantFOV(grid, xLoc, yLoc, octant, maxRadius, columnsRightFromOrigin + 1, newStartSlope, newEndSlope,
						  forbiddenTerrain, forbiddenFlags, cautiousOnWalls);
			
			currentlyLit = false;
		}
	}
	
	if (currentlyLit) { // got to the bottom of the scan while lit
	
		newEndSlope = endSlope;
		
		// run next column
		scanOctantFOV(grid, xLoc, yLoc, octant, maxRadius, columnsRightFromOrigin + 1, newStartSlope, newEndSlope,
					  forbiddenTerrain, forbiddenFlags, cautiousOnWalls);
	}
}

void addScentToCell(short x, short y, short distance) {
	if (!cellHasTerrainFlag(x, y, OBSTRUCTS_SCENT) || !cellHasTerrainFlag(x, y, OBSTRUCTS_PASSABILITY)) {
		tmap[x][y].scent = max(rogue.scentTurnNumber - distance, tmap[x][y].scent);
	}
}
