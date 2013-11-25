/*
 *  Items.c
 *  Brogue
 *
 *  Created by Brian Walker on 1/17/09.
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

// Allocates space, generates a specified item (or random category/kind if -1)
// and returns a pointer to that item. Note that the item
// is not given a location here and is not inserted into the item chain!
item *generateItem(short theCategory, short theKind) {
	item *theItem;
	theItem = (item *) malloc(sizeof(item));
	memset(theItem, '\0', sizeof(item) );
	theItem->category = 0;
	theItem->kind = 0;
	theItem->flags = 0;
	theItem->displayChar = '&';
	theItem->foreColor = &yellow;
	theItem->inventoryColor = &white;
	theItem->armor = 0;
	theItem->strengthRequired = 0;
	theItem->enchant1 = 0;
	theItem->enchant2 = 0;
	theItem->vorpalEnemy = 0;
	theItem->charges = 0;
	theItem->quantity = 1;
	theItem->quiverNumber = 0;
	theItem->inscription[0] = '\0';
	makeItemInto(theItem, theCategory, theKind);
	return theItem;
}

unsigned short pickItemCategory() {
	short i, sum, randIndex;
	short probabilities[11] =						{50,	40,		40,		4,		4,		13,		10,		5,		4,		0,		0};
	unsigned short correspondingCategories[11] =	{GOLD,	SCROLL,	POTION,	STAFF,	WAND,	WEAPON,	ARMOR,	FOOD,	RING,	AMULET,	GEM};
	
	sum = 0;
	
	for (i=0; i<11; i++) {
		sum += probabilities[i];
	}
	
	randIndex = rand_range(1, sum);
	
	for (i=0; ; i++) {
		if (randIndex <= probabilities[i]) {
			return correspondingCategories[i];
		}
		randIndex -= probabilities[i];
	}
}

// Sets an item to the given type and category (or chooses randomly if -1) with all other stats
item *makeItemInto(item *theItem, short itemCategory, short itemKind) {
	itemTable *theEntry;
	
	if (itemCategory < 0) {
		itemCategory = pickItemCategory();
	}
	
	theItem->category = itemCategory;
	
	switch (itemCategory) {
		case FOOD:
			if (itemKind < 0) {
				itemKind = chooseKind(foodTable, NUMBER_FOOD_KINDS);
			}
			theEntry = &foodTable[itemKind];
			theItem->displayChar = FOOD_CHAR;
			theItem->flags |= ITEM_IDENTIFIED;
			break;
		case WEAPON:
			if (itemKind < 0) {
				itemKind = chooseKind(weaponTable, NUMBER_WEAPON_KINDS);
			}
			theEntry = &weaponTable[itemKind];
			theItem->damage = weaponTable[itemKind].range;
			theItem->strengthRequired = weaponTable[itemKind].strengthRequired;
			theItem->displayChar = WEAPON_CHAR;
			if (rand_percent(40)) {
				theItem->enchant1 += rand_range(1, 3);
				if (rand_percent(50)) {
					// cursed
					theItem->enchant1 *= -1;
					theItem->flags |= ITEM_CURSED;
					if (rand_percent(33)) { // give it a bad runic
						theItem->enchant2 = rand_range(NUMBER_GOOD_WEAPON_ENCHANT_KINDS, NUMBER_WEAPON_RUNIC_KINDS - 1);
						theItem->flags |= ITEM_RUNIC;
					}
				} else if (rand_range(0, 20) > (theItem->damage.lowerBound + theItem->damage.upperBound) / 2) {
					// give it a good runic
					theItem->enchant2 = rand_range(0, NUMBER_GOOD_WEAPON_ENCHANT_KINDS - 1);
					theItem->flags |= ITEM_RUNIC;
					if (theItem->enchant2 == W_SLAYING) {
						theItem->vorpalEnemy = chooseVorpalEnemy();
					}
				}
			}
			if (itemKind == DART || itemKind == INCENDIARY_DART || itemKind == JAVELIN) {
				theItem->quantity = rand_range(5, 20);
				theItem->quiverNumber = rand_range(1, 60000);
				theItem->flags &= ~(ITEM_CURSED | ITEM_RUNIC); // throwing weapons can't be cursed or runic
			}
			theItem->charges = 20;
			break;
		case ARMOR:
			if (itemKind < 0) {
				itemKind = chooseKind(armorTable, NUMBER_ARMOR_KINDS);
			}
			theEntry = &armorTable[itemKind];
			theItem->armor = randClump(armorTable[itemKind].range);
			theItem->strengthRequired = armorTable[itemKind].strengthRequired;
			theItem->displayChar = ARMOR_CHAR;
			theItem->charges = 1000; // this many turns until it reveals its enchants and whether runic
			if (rand_percent(40)) {
				theItem->enchant1 += rand_range(1, 3);
				if (rand_percent(50)) {
					// cursed
					theItem->enchant1 *= -1;
					theItem->flags |= ITEM_CURSED;
					if (rand_percent(33)) { // give it a bad runic
						theItem->enchant2 = rand_range(NUMBER_GOOD_ARMOR_ENCHANT_KINDS, NUMBER_ARMOR_ENCHANT_KINDS - 1);
						theItem->flags |= ITEM_RUNIC;
					}
				} else if (rand_range(0, 95) > theItem->armor) { // give it a good runic
					theItem->enchant2 = rand_range(0, NUMBER_GOOD_ARMOR_ENCHANT_KINDS - 1);
					theItem->flags |= ITEM_RUNIC;
					if (theItem->enchant2 == A_IMMUNITY) {
						theItem->vorpalEnemy = chooseVorpalEnemy();
					}
				}
			}
			break;
		case SCROLL:
			if (itemKind < 0) {
				itemKind = chooseKind(scrollTable, NUMBER_SCROLL_KINDS);
			}
			theEntry = &scrollTable[itemKind];
			theItem->displayChar = SCROLL_CHAR;
			theItem->flags |= ITEM_FLAMMABLE;
			break;
		case POTION:
			if (itemKind < 0) {
				itemKind = chooseKind(potionTable, NUMBER_POTION_KINDS);
			}
			theEntry = &potionTable[itemKind];
			theItem->displayChar = POTION_CHAR;
			break;
		case STAFF:
			if (itemKind < 0) {
				itemKind = chooseKind(staffTable, NUMBER_STAFF_KINDS);
			}
			theEntry = &staffTable[itemKind];
			theItem->displayChar = STAFF_CHAR;
			theItem->charges = 2;
			if (rand_percent(50)) {
				theItem->charges++;
				if (rand_percent(15)) {
					theItem->charges++;
				}
			}
			theItem->enchant1 = theItem->charges;
			theItem->enchant2 = (itemKind == STAFF_BLINKING || itemKind == STAFF_OBSTRUCTION ? 1000 : 500); // start with no recharging mojo
			break;
		case WAND:
			if (itemKind < 0) {
				itemKind = chooseKind(wandTable, NUMBER_WAND_KINDS);
			}
			theEntry = &wandTable[itemKind];
			theItem->displayChar = WAND_CHAR;
			theItem->charges = randClump(wandTable[itemKind].range);
			break;
		case RING:
			if (itemKind < 0) {
				itemKind = chooseKind(ringTable, NUMBER_RING_KINDS);
			}
			theEntry = &ringTable[itemKind];
			theItem->displayChar = RING_CHAR;
			theItem->enchant1 = randClump(ringTable[itemKind].range);
			if (rand_percent(16)) {
				// cursed
				theItem->enchant1 *= -1;
				theItem->flags |= ITEM_CURSED;
			}
			break;
		case GOLD:
			theEntry = NULL;
			theItem->displayChar = GOLD_CHAR;
			theItem->quantity = rand_range(50 + rogue.depthLevel * 10, 100 + rogue.depthLevel * 15);
			break;
		case AMULET:
			theEntry = NULL;
			theItem->displayChar = AMULET_CHAR;
			itemKind = 0;
			theItem->flags |= ITEM_IDENTIFIED;
			break;
		case GEM:
			theEntry = NULL;
			theItem->displayChar = GEM_CHAR;
			itemKind = 0;
			theItem->flags |= ITEM_IDENTIFIED;
			break;
		default:
			theEntry = NULL;
			message("something has gone terribly wrong!", true, true);
			break;
	}
	if (theItem
		&& !(theItem->flags & ITEM_IDENTIFIED)
		&& (!(theItem->category & (POTION | SCROLL) ) || (theEntry && !theEntry->identified))) {
		theItem->flags |= ITEM_CAN_BE_IDENTIFIED;
	}
	theItem->kind = itemKind;
	
	return theItem;
}

short chooseKind(itemTable *theTable, short numKinds) {
	short i, totalFrequencies = 0, randomFrequency;
	for (i=0; i<numKinds; i++) {
		totalFrequencies += max(0, theTable[i].frequency);
	}
	randomFrequency = rand_range(1, totalFrequencies);
	for (i=0; randomFrequency > theTable[i].frequency; i++) {
		randomFrequency -= max(0, theTable[i].frequency);
	}
	return i;
}

// Places an item at (x,y) if provided or else a random location if they're 0. Inserts item into the floor list.
item *placeItem(item *theItem, short x, short y) {
	short loc[2];
	enum dungeonLayers layer;
	char theItemName[DCOLS], buf[DCOLS];
	if (x <= 0 || y <= 0) {
		randomMatchingLocation(loc, FLOOR, NOTHING, -1);
		theItem->xLoc = loc[0];
		theItem->yLoc = loc[1];
	} else {
		theItem->xLoc = x;
		theItem->yLoc = y;
	}
	theItem->nextItem = floorItems->nextItem;
	floorItems->nextItem = theItem;
	pmap[theItem->xLoc][theItem->yLoc].flags |= HAS_ITEM;
	if ((theItem->flags & ITEM_MAGIC_DETECTED) && itemMagicChar(theItem)) {
		pmap[theItem->xLoc][theItem->yLoc].flags |= ITEM_DETECTED;
	}
	if (cellHasTerrainFlag(x, y, IS_DF_TRAP)
		&& !(pmap[x][y].flags & PRESSURE_PLATE_DEPRESSED)) {
		pmap[x][y].flags |= PRESSURE_PLATE_DEPRESSED;
		if (playerCanSee(x, y)) {
			if (cellHasTerrainFlag(x, y, IS_SECRET)) {
				discover(x, y);
				refreshDungeonCell(x, y);
			}
			itemName(theItem, theItemName, false, false);
			sprintf(buf, "a hidden pressure plate under the %s clicks!", theItemName);
			message(buf, true, true);
		}
		for (layer = 0; layer < NUMBER_TERRAIN_LAYERS; layer++) {
			if (tileCatalog[pmap[x][y].layers[layer]].flags & IS_DF_TRAP) {
				spawnDungeonFeature(x, y, &(dungeonFeatureCatalog[tileCatalog[pmap[x][y].layers[layer]].fireType]), true);
				break;
			}
		}
	}
	return theItem;
}

void fillItemSpawnHeatMap(unsigned short heatMap[DCOLS][DROWS], unsigned short heatLevel, short x, short y) {
	enum directions dir;
	short newX, newY;
	
	if (pmap[x][y].layers[DUNGEON] == DOOR) {
		heatLevel += 10;
	} else if (pmap[x][y].layers[DUNGEON] == SECRET_DOOR) {
		heatLevel += 3000;
	}
	if (heatMap[x][y] > heatLevel) {
		heatMap[x][y] = heatLevel;
	}
	for (dir = 0; dir < 4; dir++) {
		newX = x + nbDirs[dir][0];
		newY = y + nbDirs[dir][1];
		if (coordinatesAreInMap(newX, newY)
			&& (!cellHasTerrainFlag(newX, newY, OBSTRUCTS_PASSABILITY | IS_DEEP_WATER | LAVA_INSTA_DEATH | TRAP_DESCENT)
				|| cellHasTerrainFlag(newX, newY, IS_SECRET))
			&& heatLevel < heatMap[newX][newY]) {
			fillItemSpawnHeatMap(heatMap, heatLevel, newX, newY);
		}
	}
}

void getItemSpawnLoc(unsigned short heatMap[DCOLS][DROWS], short spawnLoc[2], unsigned long *totalHeat) {
	unsigned long randIndex;
	unsigned short currentHeat;
	short i, j, k, l;
	
	randIndex = rand_range(1, *totalHeat);
	
	//printf("\nrandIndex: %i", randIndex);
	
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			currentHeat = (cellHasTerrainFlag(i, j, OBSTRUCTS_ITEMS)
						   || pmap[i][j].layers[LIQUID] != NOTHING) ? 0 : heatMap[i][j];
			if (randIndex <= currentHeat) { // this is the spot!
				spawnLoc[0] = i;
				spawnLoc[1] = j;
				*totalHeat -= currentHeat;
				heatMap[i][j] = 0;
				
				// lower the heat near the chosen location
				for (k = -5; k <= 5; k++) {
					for (l = -5; l <= 5; l++) {
						if (coordinatesAreInMap(i+k, j+l) && heatMap[i+k][j+l] == currentHeat) {
							heatMap[i+k][j+l] /= 10;
							*totalHeat -= (currentHeat - heatMap[i+k][j+l]);
						}
					}
				}
				return;
			}
			randIndex -= currentHeat;
		}
	}
}

// Generates and places items for the level. Must pass the location of the up-stairway on the level.
void populateItems(short upstairsX, short upstairsY) {
	if (!ITEMS_ENABLED) {
		return;
	}
	item *theItem;
	unsigned short itemSpawnHeatMap[DCOLS][DROWS];
	short i, j, numberOfItems, spawnLoc[2];
	unsigned long totalHeat;
	short theCategory, theKind, randomIndex;
	
#ifdef AUDIT_RNG
	char RNGmessage[100];
#endif
	
	if (rogue.depthLevel > AMULET_LEVEL) {
		numberOfItems = 3;
	} else {
		rogue.strengthPotionFrequency += 17;
		rogue.enchantScrollFrequency += 30;
		numberOfItems = rand_range(6, 7);
		while (rand_percent(60)) {
			numberOfItems++;
		}
	}
	
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			itemSpawnHeatMap[i][j] = 50000;
		}
	}
	fillItemSpawnHeatMap(itemSpawnHeatMap, 5, upstairsX, upstairsY);
	totalHeat = 0;
	
#ifdef AUDIT_RNG
	sprintf(RNGmessage, "\n\nInitial heat map for level %i:\n", rogue.currentTurnNumber);
	RNGLog(RNGmessage);
#endif
	
	for (j=0; j<DROWS; j++) {
		for (i=0; i<DCOLS; i++) {
			if (cellHasTerrainFlag(i, j, OBSTRUCTS_ITEMS | TRAP_DESCENT | IS_DF_TRAP)
				|| pmap[i][j].layers[LIQUID] != NOTHING
				|| (pmap[i][j].flags & (IS_CHOKEPOINT | IN_LOOP))) {
				itemSpawnHeatMap[i][j] = 0;
			}
			if (itemSpawnHeatMap[i][j] == 50000) {
				itemSpawnHeatMap[i][j] = 0;
				pmap[i][j].layers[DUNGEON] = PERM_WALL; // due to a bug that creates occasional isolated one-cell islands
			}
#ifdef AUDIT_RNG
			sprintf(RNGmessage, "%u%s%s\t%s",
					itemSpawnHeatMap[i][j],
					((pmap[i][j].flags & IS_CHOKEPOINT) ? " (C)": ""),
					((pmap[i][j].flags & IN_LOOP) ? " (L)": ""),
					(i == DCOLS-1 ? "\n" : ""));
			RNGLog(RNGmessage);
#endif
			totalHeat += itemSpawnHeatMap[i][j];
		}
	}
	
	for (i=0; i<numberOfItems; i++) {
		theCategory = theKind = -1;
		
		scrollTable[SCROLL_ENCHANT_ITEM].frequency = rogue.enchantScrollFrequency;
		potionTable[POTION_GAIN_STRENGTH].frequency = rogue.strengthPotionFrequency;
		
		// guarantee a certain nutrition minimum of the equivalent of one ration every three levels
		if ((rogue.foodSpawned + foodTable[RATION].strengthRequired / 2) * 3
			<= pow(rogue.depthLevel, 1.3) * foodTable[RATION].strengthRequired * 0.55) {
			theCategory = FOOD;
			if (rogue.depthLevel > AMULET_LEVEL) {
				numberOfItems++;
			}
		} else if (rogue.depthLevel > AMULET_LEVEL) {
			theCategory = GEM;
		} else if ((rogue.depthLevel >= rogue.luckyLevels[0] && rogue.goodItemsGenerated < 1)
				   || (rogue.depthLevel >= rogue.luckyLevels[1] && rogue.goodItemsGenerated < 2)
				   || (rogue.depthLevel >= rogue.luckyLevels[2] && rogue.goodItemsGenerated < 3)) {
			// guarantee at least n good items by the nth lucky level
			randomIndex = rand_range(0, NUMBER_GOOD_ITEMS - 1);
			theCategory = goodItems[randomIndex][0];
			theKind = goodItems[randomIndex][1];
		}
		
		theItem = generateItem(theCategory, theKind);
		if (theItem->category & FOOD) {
			rogue.foodSpawned += foodTable[theItem->kind].strengthRequired;
			randomMatchingLocation(spawnLoc, FLOOR, NOTHING, -1); // food doesn't follow the heat map
			totalHeat -= itemSpawnHeatMap[spawnLoc[0]][spawnLoc[1]];
			itemSpawnHeatMap[spawnLoc[0]][spawnLoc[1]] = 0;
		} else if ((theItem->category & POTION) && theItem->kind == POTION_GAIN_STRENGTH) {
			randomMatchingLocation(spawnLoc, FLOOR, NOTHING, -1); // gain strength doesn't follow the heat map
			totalHeat -= itemSpawnHeatMap[spawnLoc[0]][spawnLoc[1]];
			itemSpawnHeatMap[spawnLoc[0]][spawnLoc[1]] = 0;
		} else {
			getItemSpawnLoc(itemSpawnHeatMap, spawnLoc, &totalHeat);
		}
		
		// regulate the frequency of enchantment scrolls and gain strength potions:
		if (theItem->category & SCROLL && theItem->kind == SCROLL_ENCHANT_ITEM) {
			scrollTable[SCROLL_ENCHANT_ITEM].frequency -= 50; // it can go negative
			rogue.enchantScrollFrequency -= 50;
			//DEBUG printf("\ndepth %i: %i enchant scrolls generated so far", rogue.depthLevel, ++enchantScrolls);
		} else if (theItem->category & POTION && theItem->kind == POTION_GAIN_STRENGTH) {
			potionTable[POTION_GAIN_STRENGTH].frequency -= 50;
			rogue.strengthPotionFrequency -= 50;
			//DEBUG printf("\ndepth %i: %i strength potions generated so far", rogue.depthLevel, ++strPots);
		} else if (!(theItem->category & (GEM | AMULET))){
			// scrollTable[SCROLL_ENCHANT_ITEM].frequency += 2;
			// (now handled above, once per level)
		}
		
		// if it's a good item, remember that we generated one
		for (j=0; j<NUMBER_GOOD_ITEMS; j++) {
			if (theItem->category == goodItems[j][0] && theItem->kind == goodItems[j][1]) {
				rogue.goodItemsGenerated++;
				break;
			}
		}
		
		placeItem(theItem, spawnLoc[0], spawnLoc[1]); // random valid location
	}
	
	scrollTable[SCROLL_ENCHANT_ITEM].frequency = 0;
	potionTable[POTION_GAIN_STRENGTH].frequency = 0;
}

// name of this function is a bit misleading -- basically returns true iff the item will stack without consuming an extra slot
// i.e. if it's a throwing weapon with a sibling already in your pack. False for potions and scrolls.
boolean itemWillStackWithPack(item *theItem) {
	item *tempItem;
	if (!(theItem->quiverNumber)) {
		return false;
	} else {
		for (tempItem = packItems->nextItem;
			 tempItem != NULL && tempItem->quiverNumber != theItem->quiverNumber;
			 tempItem = tempItem->nextItem);
		return (tempItem ? true : false);
	}
}

// adds the item at (x,y) to the pack
void pickUpItemAt(short x, short y) {
	item *theItem, *previousItem;
	char buf[COLS], buf2[COLS];
	
	rogue.disturbed = true;
	
	// find the item
	for (theItem = floorItems->nextItem; theItem && (theItem->xLoc != x || theItem->yLoc != y)
		 && theItem->nextItem != NULL; theItem = theItem->nextItem);
	
	if (!theItem) {
		message("Error: Expected item; item not found.", true, true);
		return;
	}
	
	theItem->flags |= ITEM_NAMED;
	
	if (numberOfItemsInPack() < MAX_PACK_ITEMS || theItem->category & GOLD || itemWillStackWithPack(theItem)) {
		
		if (theItem->flags & ITEM_NO_PICKUP) {
			// auto-identify the scroll here
			scrollTable[theItem->kind].identified = true;
			
			itemName(theItem, buf2, true, false); // include suffix but not article
			sprintf(buf, "the %s is stuck to the ground.", buf2);
			message(buf, true, false);
			return;
		}
		
		// remove from floor chain
		pmap[x][y].flags &= ~(HAS_ITEM | ITEM_DETECTED);
		
		for (previousItem = floorItems; previousItem->nextItem != theItem; previousItem = previousItem->nextItem);
		
		previousItem->nextItem = theItem->nextItem;
		
		if (theItem->category & GOLD) {
			rogue.gold += theItem->quantity;
			// updateStatBar(GOLD_STAT);
			sprintf(buf, "you found %i pieces of gold", theItem->quantity);
			message(buf, true, false);
			deleteItem(theItem);
			return;
		}
		
		if (theItem->category & AMULET && numberOfMatchingPackItems(AMULET, 0, 0, false)) {
			message("you already have the Amulet of Yendor.", true, false); 
			deleteItem(theItem);
			return;
		}
		
		theItem = addItemToPack(theItem);
		
		if (theItem->category & SCROLL && theItem->kind == SCROLL_SANCTUARY) {
			theItem->flags |= ITEM_NO_PICKUP;
		}
		
		itemName(theItem, buf2, true, true); // include suffix, article
		
		sprintf(buf, "you now have %s (%c).", buf2, theItem->inventoryLetter);
		message(buf, true, false);
	} else {
		itemName(theItem, buf2, false, true); // include article
		sprintf(buf, "Your pack is too full to pick up %s.", buf2);
		message(buf, true, false);
	}
}

item *addItemToPack(item *theItem) {
	item *previousItem, *tempItem;
	char itemLetter;
	
	theItem->flags |= ITEM_NAMED;
	
	// can the item stack with another in the inventory?
	if (theItem->category & (FOOD|POTION|SCROLL|GEM)) {
		for (tempItem = packItems->nextItem; tempItem != NULL; tempItem = tempItem->nextItem) {
			if (theItem->category == tempItem->category && theItem->kind == tempItem->kind) {
				// we found a match! Increment the quantity of the old item...
				tempItem->quantity++;
				
				// let magic detection and other flags propagate to the stack...
				tempItem->flags |= (theItem->flags & (ITEM_MAGIC_DETECTED | ITEM_IDENTIFIED | ITEM_PROTECTED | ITEM_NAMED | ITEM_RUNIC
													  | ITEM_RUNIC_HINTED | ITEM_CAN_BE_IDENTIFIED | ITEM_MAX_CHARGES_KNOWN));
				updateItemKnownMagicStatus(tempItem);
				
				// keep the higher enchantment and lower strength requirement...
				if (theItem->enchant1 > tempItem->enchant1) {
					tempItem->enchant1 = theItem->enchant1;
				}
				if (theItem->strengthRequired < tempItem->strengthRequired) {
					tempItem->strengthRequired = theItem->strengthRequired;
				}
				if (theItem->inscription && !tempItem->inscription) {
					strcpy(tempItem->inscription, theItem->inscription);
				}
				
				// and delete the new item.
				deleteItem(theItem);
				
				// Pass back the incremented (old) item. No need to add it to the pack since it's already there.
				return tempItem;
			}
		}
	} else if (theItem->category & WEAPON && theItem->quiverNumber > 0) {
		for (tempItem = packItems->nextItem; tempItem != NULL; tempItem = tempItem->nextItem) {
			if (theItem->category == tempItem->category && theItem->kind == tempItem->kind
				&& theItem->quiverNumber == tempItem->quiverNumber) {
				
				// we found a match! Increase the quantity of the old item...
				tempItem->quantity += theItem->quantity;
				
				// let magic detection and other flags propagate to the stack...
				tempItem->flags |= (theItem->flags & (ITEM_MAGIC_DETECTED | ITEM_IDENTIFIED | ITEM_PROTECTED | ITEM_NAMED | ITEM_RUNIC
													  | ITEM_RUNIC_HINTED | ITEM_CAN_BE_IDENTIFIED | ITEM_MAX_CHARGES_KNOWN));
				updateItemKnownMagicStatus(tempItem);
				
				// keep the higher enchantment and lower strength requirement...
				if (theItem->enchant1 > tempItem->enchant1) {
					tempItem->enchant1 = theItem->enchant1;
				}
				if (theItem->strengthRequired < tempItem->strengthRequired) {
					tempItem->strengthRequired = theItem->strengthRequired;
				}
				if (theItem->inscription && !tempItem->inscription) {
					strcpy(tempItem->inscription, theItem->inscription);
				}
				
				// and delete the new item.
				deleteItem(theItem);
				
				// Pass back the incremented (old) item. No need to add it to the pack since it's already there.
				return tempItem;
			}
		}
	}
	
	// assign a reference letter to the item
	itemLetter = nextAvailableInventoryCharacter();
	if (itemLetter) {
		theItem->inventoryLetter = itemLetter;
	}
	
	// append to pack chain
	for (previousItem = packItems;
		 previousItem->nextItem != NULL && previousItem->nextItem->category <= theItem->category;
		 previousItem = previousItem->nextItem);
	theItem->nextItem = previousItem->nextItem;
	previousItem->nextItem = theItem;
	
	return theItem;
}

short numberOfItemsInPack() {
	short theCount = 0;
	item *theItem;
	for(theItem = packItems->nextItem; theItem != NULL; theItem = theItem->nextItem) {
		theCount += (theItem->category & WEAPON ? 1 : theItem->quantity);
	}
	return theCount;
}

char nextAvailableInventoryCharacter() {
	boolean charTaken[26];
	short i;
	item *theItem;
	char c;
	for(i=0; i<26; i++) {
		charTaken[i] = false;
	}
	for (theItem = packItems->nextItem; theItem != NULL; theItem = theItem->nextItem) {
		c = theItem->inventoryLetter;
		if (c >= 'a' && c <= 'z') {
			charTaken[c - 'a'] = true;
		}
	}
	for(i=0; i<26; i++) {
		if (!charTaken[i]) {
			return ('a' + i);
		}
	}
	return 0;
}

boolean inscribeItem(item *theItem) {
	char itemText[30], buf[COLS], nameOfItem[COLS];
	
	itemName(theItem, nameOfItem, true, true);
	sprintf(buf, "inscribe: %s \"", nameOfItem);
	if (getInputTextString(itemText, buf, 29, "", "\"", TEXT_INPUT_NORMAL)) {
		strcpy(theItem->inscription, itemText);
		confirmMessages();
		itemName(theItem, nameOfItem, true, true);
		nameOfItem[strlen(nameOfItem) - 1] = '\0';
		sprintf(buf, "it's %s.\"", nameOfItem);
		message(buf, true, false);
		return true;
	} else {
		confirmMessages();
		return false;
	}
}

void call() {
	item *theItem;
	char itemText[30], buf[COLS];
	short c;
	unsigned char command[100];
	
	c = 0;
	command[c++] = CALL_KEY;
	theItem = promptForItemOfType((WEAPON|ARMOR|SCROLL|RING|POTION|STAFF|WAND), 0, 0,
								  "Call what? (a-z, shift for more info; or <esc> to cancel)");
	if (theItem == NULL) {
		return;
	}
	
	command[c++] = theItem->inventoryLetter;
	
	confirmMessages();
	
	if (theItem->flags & ITEM_IDENTIFIED || theItem->category & (WEAPON|ARMOR|FOOD|GOLD|AMULET|GEM)) {
		if (theItem->category & (WEAPON | ARMOR | STAFF | WAND | RING) && theItem->flags & ITEM_CAN_BE_IDENTIFIED) {
			if (inscribeItem(theItem)) {
				command[c++] = '\0';
				strcat((char *) command, theItem->inscription);
				recordKeystrokeSequence(command);
				recordKeystroke(RETURN_KEY, false, false);
			}
		} else {
			message("you already know what that is.", true, false);
		}
		return;
	}
	
	if (theItem->flags & ITEM_CAN_BE_IDENTIFIED
		&& theItem->category & (WEAPON | ARMOR | STAFF | WAND | RING)) {
		if (confirm("inscribe this particular item instead of all similar items? (y/n)", true)) {
			command[c++] = 'y';
			if (inscribeItem(theItem)) {
				command[c++] = '\0';
				strcat((char *) command, theItem->inscription);
				recordKeystrokeSequence(command);
				recordKeystroke(RETURN_KEY, false, false);
			}
			return;
		} else {
			command[c++] = 'n';
		}
	}
	
	if (getInputTextString(itemText, "call them: \"", 29, "", "\"", TEXT_INPUT_NORMAL)) {
		switch (theItem->category) {
			case SCROLL:
				strcpy(scrollTable[theItem->kind].callTitle, itemText);
				scrollTable[theItem->kind].called = true;
				break;
			case POTION:
				strcpy(potionTable[theItem->kind].callTitle, itemText);
				potionTable[theItem->kind].called = true;
				break;
			case WAND:
				strcpy(wandTable[theItem->kind].callTitle, itemText);
				wandTable[theItem->kind].called = true;
				break;
			case STAFF:
				strcpy(staffTable[theItem->kind].callTitle, itemText);
				staffTable[theItem->kind].called = true;
				break;
			case RING:
				strcpy(ringTable[theItem->kind].callTitle, itemText);
				ringTable[theItem->kind].called = true;
				break;
			default:
				break;
		}
		confirmMessages();
		itemName(theItem, buf, false, true);
		message(buf, true, false);
		command[c++] = '\0';
		strcat((char *) command, itemText);
		recordKeystrokeSequence(command);
		recordKeystroke(RETURN_KEY, false, false);
	} else {
		confirmMessages();
	}
}

void itemName(item *theItem, char *root, boolean includeDetails, boolean includeArticle) {
	char buf[DCOLS], pluralization[10], article[10] = "";
	
	sprintf(pluralization, (theItem->quantity > 1 ? "s" : ""));
	
	switch (theItem -> category) {
		case FOOD:
			if (theItem -> kind == FRUIT) {
				sprintf(root, "mango%s", pluralization);
			} else {
				if (theItem->quantity == 1) {
					sprintf(article, "some ");
					sprintf(root, "food");
				} else {
					sprintf(root, "ration%s of food", pluralization);
				}
			}
			break;
		case WEAPON:
			sprintf(root, "%s%s", weaponTable[theItem->kind].name, pluralization);
			if (includeDetails) {
				if ((theItem->flags & ITEM_IDENTIFIED) || rogue.playbackOmniscience) {
					sprintf(buf, "%s%i %s", (theItem->enchant1 < 0 ? "" : "+"), theItem->enchant1, root);
					strcpy(root, buf);
				}
				
				if (theItem->flags & ITEM_RUNIC) {
					if ((theItem->flags & ITEM_RUNIC_IDENTIFIED) || rogue.playbackOmniscience) {
						if (theItem->enchant2 == W_SLAYING) {
							sprintf(root, "%s of %s slaying", root, monsterCatalog[theItem->vorpalEnemy].monsterName);
						} else {
							sprintf(root, "%s of %s", root, weaponRunicNames[theItem->enchant2]);
						}
					} else if (theItem->flags & (ITEM_IDENTIFIED | ITEM_RUNIC_HINTED)) {
						strcat(root, " (unknown runic)");
					}
				}
				sprintf(root, "%s <%i>", root, theItem->strengthRequired);
			}
			break;
		case ARMOR:
			sprintf(root, "%s", armorTable[theItem->kind].name);
			if (includeDetails) {
				
				if ((theItem->flags & ITEM_RUNIC)
					&& ((theItem->flags & ITEM_RUNIC_IDENTIFIED)
						|| (rogue.playbackOmniscience))) {
						if (theItem->enchant2 == A_IMMUNITY) {
							sprintf(root, "%s of %s immunity", root, monsterCatalog[theItem->vorpalEnemy].monsterName);
						} else {
							sprintf(root, "%s of %s", root, armorRunicNames[theItem->enchant2]);
						}
					}
				
				if ((theItem->flags & ITEM_IDENTIFIED) || rogue.playbackOmniscience) {
					if (theItem->enchant1 == 0) {
						sprintf(buf, "%s [%i]<%i>", root, theItem->armor/10, theItem->strengthRequired);
					} else {
						sprintf(buf, "%s%i %s [%i]<%i>", (theItem->enchant1 < 0 ? "" : "+"), theItem->enchant1,
								root, theItem->armor/10 + theItem->enchant1, theItem->strengthRequired);
					}
					strcpy(root, buf);
				} else {
					sprintf(root, "%s <%i>", root, theItem->strengthRequired);
				}
				
				if ((theItem->flags & ITEM_RUNIC)
					&& (theItem->flags & (ITEM_IDENTIFIED | ITEM_RUNIC_HINTED))
					&& !(theItem->flags & ITEM_RUNIC_IDENTIFIED)) {
					strcat(root, " (unknown runic)");
				}
			}
			
			break;
		case SCROLL:
			if (scrollTable[theItem->kind].identified || rogue.playbackOmniscience) {
				sprintf(root, "scroll%s of %s", pluralization, scrollTable[theItem->kind].name);
			} else if (scrollTable[theItem->kind].called) {
				sprintf(root, "scroll%s called %s", pluralization, scrollTable[theItem->kind].callTitle);
			} else {
				sprintf(root, "scroll%s entitled \"%s\"", pluralization, scrollTable[theItem->kind].flavor);
			}
			break;
		case POTION:
			if (potionTable[theItem->kind].identified || rogue.playbackOmniscience) {
				sprintf(root, "potion%s of %s", pluralization, potionTable[theItem->kind].name);
			} else if (potionTable[theItem->kind].called) {
				sprintf(root, "potion%s called %s", pluralization, potionTable[theItem->kind].callTitle);
			} else {
				sprintf(root, "%s potion%s", potionTable[theItem->kind].flavor, pluralization);
			}
			break;
		case WAND:
			if (wandTable[theItem->kind].identified || rogue.playbackOmniscience) {
				sprintf(root, "wand%s of %s",
						pluralization,
						wandTable[theItem->kind].name);
			} else if (wandTable[theItem->kind].called) {
				sprintf(root, "wand%s called %s",
						pluralization,
						wandTable[theItem->kind].callTitle);
			} else {
				sprintf(root, "%s wand%s", wandTable[theItem->kind].flavor, pluralization);
			}
			if (includeDetails) {
				if (theItem->flags & (ITEM_IDENTIFIED | ITEM_MAX_CHARGES_KNOWN) || rogue.playbackOmniscience) {
					sprintf(root, "%s [%i]",
							root,
							theItem->charges);
				} else if (theItem->enchant2 > 2) {
					sprintf(root, "%s (used %i times)",
							root,
							theItem->enchant2);
				} else if (theItem->enchant2) {
					sprintf(root, "%s (used %s)",
							root,
							(theItem->enchant2 == 2 ? "twice" : "once"));
				}
			}
			break;
		case STAFF:
			if (staffTable[theItem->kind].identified || rogue.playbackOmniscience) {
				sprintf(root, "staff%s of %s", pluralization, staffTable[theItem->kind].name);
			} else if (staffTable[theItem->kind].called) {
				sprintf(root, "staff%s called %s", pluralization, staffTable[theItem->kind].callTitle);
			} else {
				sprintf(root, "%s staff%s", staffTable[theItem->kind].flavor, pluralization);
			}
			if (includeDetails) {
				if ((theItem->flags & ITEM_IDENTIFIED) || rogue.playbackOmniscience) {
					sprintf(root, "%s [%i/%i]", root, theItem->charges, theItem->enchant1);
				} else if (theItem->flags & ITEM_MAX_CHARGES_KNOWN) {
					sprintf(root, "%s [?/%i]", root, theItem->enchant1);
				}
			}
			break;
		case RING:
			if (ringTable[theItem->kind].identified || rogue.playbackOmniscience) {
				sprintf(root, "ring%s of %s", pluralization, ringTable[theItem->kind].name);
			} else if (ringTable[theItem->kind].called) {
				sprintf(root, "ring%s called %s", pluralization, ringTable[theItem->kind].callTitle);
			} else {
				sprintf(root, "%s ring%s", ringTable[theItem->kind].flavor, pluralization);
			}
			if (includeDetails && ((theItem->flags & ITEM_IDENTIFIED) || rogue.playbackOmniscience)) {
				sprintf(buf, "%s%i %s", (theItem->enchant1 < 0 ? "" : "+"), theItem->enchant1, root);
				strcpy(root, buf);
			}
			break;
		case GOLD:
			sprintf(root, "gold piece%s", pluralization);
			break;
		case AMULET:
			sprintf(root, "Amulet%s of Yendor", pluralization);
			break;
		case GEM:
			sprintf(root, "lumenstone%s", pluralization);
			break;
		default:
			sprintf(root, "unknown item%s", pluralization);
			break;
	}
	
	if (includeArticle) {
		// prepend number if quantity is over 1
		if (theItem->quantity > 1) {
			sprintf(article, "%i ", theItem->quantity);
		} else if (theItem->category & AMULET) {
			sprintf(article, "the ");
		} else if (!(theItem->category & ARMOR) && !(theItem->category & FOOD && theItem->kind == RATION)) {
			// otherwise prepend a/an if the item is not armor and not a ration of food;
			// armor gets no article, and "some food" was taken care of above.
			sprintf(article, "a%s ", (isVowel(root[0]) ? "n" : ""));
		}
	}
	// strcat(buf, suffixID);
	if (includeArticle) {
		sprintf(buf, "%s%s", article, root);
		strcpy(root, buf);
	}
	
	if (includeDetails && theItem->inscription[0]) {
		if (theItem->flags & ITEM_CAN_BE_IDENTIFIED) {
			sprintf(buf, "%s \"%s\"", root, theItem->inscription);
			strcpy(root, buf);
		} else {
			theItem->inscription[0] = '\0';
		}
	}
	return;
}

itemTable *tableForItemCategory(enum itemCategory theCat) {
	switch (theCat) {
		case FOOD:
			return foodTable;
		case WEAPON:
			return weaponTable;
		case ARMOR:
			return armorTable;
		case POTION:
			return potionTable;
		case SCROLL:
			return scrollTable;
		case RING:
			return ringTable;
		case WAND:
			return wandTable;
		case STAFF:
			return staffTable;
		default:
			return NULL;
	}
}

// Call this when the item (might) already have the ITEM_MAGIC_DETECTED flag set. This will
// delete the flag if the knowledge is redundant.
// DISABLED.
void updateItemKnownMagicStatus(item *theItem) {
	return;
	if (theItem->flags & ITEM_MAGIC_DETECTED) {
		switch (theItem->category) {
			case SCROLL:
			case POTION:
			case WAND:
			case STAFF:
				if (tableForItemCategory(theItem->category)[theItem->kind].identified) {
					theItem->flags &= ~ITEM_MAGIC_DETECTED;
				}
				break;
			case WEAPON:
			case ARMOR:
			case RING:
				if (theItem->flags & ITEM_IDENTIFIED) {
					theItem->flags &= ~ITEM_MAGIC_DETECTED;
				}
			default:
				break;
		}
	}
}

boolean isVowel(char theChar) {
	return (theChar == 'a' || theChar == 'e' || theChar == 'i' || theChar == 'o' || theChar == 'u' ||
			theChar == 'A' || theChar == 'E' || theChar == 'I' || theChar == 'O' || theChar == 'U');
}

void itemDetails(char *buf, item *theItem) {
	char buf2[1000], theName[100];
	boolean singular, carried;
	item *tempItem;
	short enchant, damagePercent, nextLevelState = 0;
	const char weaponRunicEffectDescriptions[NUMBER_WEAPON_RUNIC_KINDS][30] = {
		"poisoned",
		"instantly killed",
		"paralyzed",
		"cleansed of magic",
		"slowed",
		"confused",
		"instantly killed", // never used
		"healed",
		"cloned"
	};
	
	singular = (theItem->quantity == 1 ? true : false);
	itemName(theItem, theName, false, false);
	carried = false;
	for (tempItem = packItems->nextItem; tempItem != NULL; tempItem = tempItem->nextItem) {
		if (tempItem == theItem) {
			carried = true;
			break;
		}
	}
	
	// introductory text
	if (tableForItemCategory(theItem->category)
		&& tableForItemCategory(theItem->category)[theItem->kind].identified) {
		
		strcpy(buf, tableForItemCategory(theItem->category)[theItem->kind].description);
	} else {
		switch (theItem->category) {
			case POTION:
				sprintf(buf, "%s flask%s contain%s a swirling %s liquid. \
Who knows what %s will do when drunk or thrown?",
						(singular ? "This" : "These"),
						(singular ? "" : "s"),
						(singular ? "s" : ""),
						tableForItemCategory(theItem->category)[theItem->kind].flavor,
						(singular ? "it" : "they"));
				break;
			case SCROLL:
				sprintf(buf, "%s parchment%s %s covered with magical writing, and bear%s a title of \"%s.\" \
Who knows what %s will do when read aloud?",
						(singular ? "This" : "These"),
						(singular ? "" : "s"),
						(singular ? "is" : "are"),
						(singular ? "s" : ""),
						tableForItemCategory(theItem->category)[theItem->kind].flavor,
						(singular ? "it" : "they"));
				break;
			case STAFF:
				sprintf(buf, "This gnarled %s staff pulsates with an arcane power. \
Who knows what it will do when used?",
						tableForItemCategory(theItem->category)[theItem->kind].flavor);
				break;
			case WAND:
				sprintf(buf, "This thin %s wand is warm to the touch. \
Who knows what it will do when used?",
						tableForItemCategory(theItem->category)[theItem->kind].flavor);
				break;
			case RING:
				sprintf(buf, "This metal band is adorned with a large %s gem that pulsates with arcane power. \
Who knows what effect it has when worn?",
						tableForItemCategory(theItem->category)[theItem->kind].flavor);
				break;
			case AMULET:
				strcpy(buf, "Legends are told about this mysterious golden amulet, \
and hundreds of adventurers have perished in its pursuit. \
Unfathomable riches await the adventurer skillful and ambitious \
enough to return it to the light of day.");
			case GEM:
				sprintf(buf, "Swirling lights of breathtaking beauty fluoresce beneath \
the stone%s smoothe surface. Lumenstones are thought to contain \
mysterious properties of untold power, but for you, \
they mean one thing: riches.",
						(singular ? "'s" : "s'"));
				break;
			case GOLD: // can't happen
				sprintf(buf, "A pile of %i shining gold coins.", theItem->quantity);
				break;
			default:
				break;
		}
	}
	
	// detailed description
	switch (theItem->category) {
			
		case FOOD:
			sprintf(buf2, "\n\nYou are %shungry enough to get the full benefit of a %s.",
					((STOMACH_SIZE - player.status.nutrition) >= foodTable[theItem->kind].strengthRequired ? "" : "not yet "),
					(theItem->kind == RATION ? "ration of food" : theName));
			strcat(buf, buf2);
			break;
			
		case WEAPON:
		case ARMOR:
			// enchanted? strength modifier?
			if ((theItem->flags & ITEM_IDENTIFIED) || rogue.playbackOmniscience) {
				if (theItem->enchant1) {
					sprintf(buf2, "\n\nThe %s bear%s an intrinsic %s%i",
							theName,
							(singular ? "s" : ""),
							(theItem->enchant1 > 0 ? "enchantment of +" : "penalty of "),
							theItem->enchant1);
				} else {
					sprintf(buf2, "\n\nThe %s bear%s no intrinsic enchantment",
							theName,
							(singular ? "s" : ""));
				}
				strcat(buf, buf2);
				if (strengthModifier(theItem)) {
					sprintf(buf2, ", %s %s %s %s%.1f because of your %s strength. ",
							(theItem->enchant1 ? "and" : "but"),
							(singular ? "carries" : "carry"),
							(theItem->enchant1 && (theItem->enchant1 > 0) == (strengthModifier(theItem) > 0) ? "an additional" : "a"),
							(strengthModifier(theItem) > 0 ? "bonus of +" : "penalty of "),
							strengthModifier(theItem),
							(strengthModifier(theItem) > 0 ? "excess" : "inadequate"));
					strcat(buf, buf2);
				} else {
					strcat(buf, ". ");
				}
			} else {
				if ((theItem->enchant1 > 0) && (theItem->flags & ITEM_MAGIC_DETECTED)) {
					sprintf(buf2, "\n\nYou can feel an aura of benevolent magic radiating from the %s. ",
							theName);
					strcat(buf, buf2);
				}
				if (strengthModifier(theItem)) {
					sprintf(buf2, "\n\nThe %s %s%s a %s%.1f because of your %s strength. ",
							theName,
							((theItem->enchant1 > 0) && (theItem->flags & ITEM_MAGIC_DETECTED) ? "also " : ""),
							(singular ? "carries" : "carry"),
							(strengthModifier(theItem) > 0 ? "bonus of +" : "penalty of "),
							strengthModifier(theItem),
							(strengthModifier(theItem) > 0 ? "excess" : "inadequate"));
					strcat(buf, buf2);
				}
			}
			
			// protected?
			if (theItem->flags & ITEM_PROTECTED) {
				sprintf(buf2, "The %s cannot be corroded by acid.",
						theName);
				strcat(buf, buf2);
			}
			
			if (theItem->category & WEAPON) {
				
				// runic?
				if (theItem->flags & ITEM_RUNIC) {
					if ((theItem->flags & ITEM_RUNIC_IDENTIFIED) || rogue.playbackOmniscience) {
						sprintf(buf2, "\n\nGlowing runes of %s adorn the %s. ",
								weaponRunicNames[theItem->enchant2],
								theName);
						strcat(buf, buf2);
						
						// W_POISON, W_QUIETUS, W_PARALYSIS, W_NEGATION, W_SLOWING, W_CONFUSION, W_SLAYING, W_MERCY, W_PLENTY
						
						if (theItem->enchant2 == W_SLAYING) {
							sprintf(buf2, "It will never fail to slay a %s in a single stroke. ",
									monsterCatalog[theItem->vorpalEnemy].monsterName);
							strcat(buf, buf2);
						} else {
							if ((theItem->flags & ITEM_IDENTIFIED) || rogue.playbackOmniscience) {
								sprintf(buf2, "%i%% of the time that",
										runicWeaponChance(theItem, false, 0));
								strcat(buf, buf2);
							} else {
								strcat(buf, "Sometimes, when");
							}
							sprintf(buf2, " it hits an enemy, the enemy will be %s",
									weaponRunicEffectDescriptions[theItem->enchant2]);
							strcat(buf, buf2);
							
							if ((theItem->flags & ITEM_IDENTIFIED) || rogue.playbackOmniscience) {
								enchant = netEnchant(theItem);
								switch (theItem->enchant2) {
									case W_POISON:
										sprintf(buf2, " for %i turns. ",
												weaponPoisonDuration(enchant));
										nextLevelState = weaponPoisonDuration(enchant + 1);
										break;
									case W_PARALYSIS:
										sprintf(buf2, " for %i turns. ",
												weaponParalysisDuration(enchant));
										nextLevelState = weaponPoisonDuration(enchant + 1);
										break;
									case W_SLOWING:
										sprintf(buf2, " for %i turns. ",
												enchant * 3);
										nextLevelState = (enchant + 1) * 3;
										break;
									case W_CONFUSION:
										sprintf(buf2, " for %i turns. ",
												weaponConfusionDuration(enchant));
										nextLevelState = weaponConfusionDuration(enchant + 1);;
										break;
									case W_MERCY:
										strcpy(buf2, " by 50%% of its maximum health. ");
										break;
									default:
										strcpy(buf2, ". ");
										break;
								}
								strcat(buf, buf2);
								
								if (((theItem->flags & ITEM_IDENTIFIED) || rogue.playbackOmniscience)
									&& runicWeaponChance(theItem, false, 0) < runicWeaponChance(theItem, true, enchant + 1)){
									sprintf(buf2, "(If the %s is enchanted, the chance will increase to %i%%",
											theName,
											runicWeaponChance(theItem, true, enchant + 1));
									strcat(buf, buf2);
									if (nextLevelState) {
										sprintf(buf2, " and the duration will increase to %i turns.)",
												nextLevelState);
									} else {
										strcpy(buf2, ".)");
									}
									strcat(buf, buf2);
								}
							} else {
								strcat(buf, ". ");
							}
						}
						
					} else if (theItem->flags & ITEM_IDENTIFIED) {
						sprintf(buf2, "\n\nGlowing runes of an unrecognizable language run down the length of the %s. ",
								theName);
						strcat(buf, buf2);
					}
				}
				
				// equipped? cursed?
				if (theItem->flags & ITEM_EQUIPPED) {
					sprintf(buf2, "\n\nYou hold the %s at the ready%s. ",
							theName,
							((theItem->flags & ITEM_CURSED) ? ", and because it is cursed, you are powerless to let go" : ""));
					strcat(buf, buf2);
				} else if (((theItem->flags & (ITEM_IDENTIFIED | ITEM_MAGIC_DETECTED)) || rogue.playbackOmniscience)
						   && (theItem->flags & ITEM_CURSED)) {
					sprintf(buf2, "\n\nYou can feel a malevolent magic lurking within the %s. ", theName);
					strcat(buf, buf2);
				}
				
			} else if (theItem->category & ARMOR) {
				
				// runic?
				if (theItem->flags & ITEM_RUNIC) {
					if ((theItem->flags & ITEM_RUNIC_IDENTIFIED) || rogue.playbackOmniscience) {
						sprintf(buf2, "\n\nGlowing runes of %s adorn the %s. ",
								armorRunicNames[theItem->enchant2],
								theName);
						strcat(buf, buf2);
						
						// A_TOUGHNESS, A_WISDOM, A_ABSORPTION, A_REPRISAL, A_IMMUNITY, A_BURDEN, A_VULNERABILITY,
						enchant = netEnchant(theItem);
						switch (theItem->enchant2) {
							case A_TOUGHNESS:
								sprintf(buf2, "When worn, all damage inflicted on you will be reduced by %i%%. (If the %s is enchanted, the reduction will increase to %i%%.) ",
										(int) (500 * theItem->enchant1 / (player.info.maxHP + 5 * theItem->enchant1)),
										theName,
										(int) (500 * (theItem->enchant1 + 1) / (player.info.maxHP + 5 * (theItem->enchant1 + 1))));
								break;
							case A_WISDOM:
								sprintf(buf2, "When worn, your staffs will recharge at %i%% of their normal rate. (If the %s is enchanted, the rate will increase to %i%% of the normal rate.) ",
										(int) (100 * pow(1.3, min(27, theItem->enchant1))),
										theName,
										(int) (100 * pow(1.3, min(27, (theItem->enchant1 + 1)))));
								break;
							case A_ABSORPTION:
								sprintf(buf2, "It will reduce the damage of inbound attacks by a random amount between 0 and %i%% of your maximum health. (If the %s is enchanted, this maximum percentage will increase to %i%%.) ",
										(int) (100 * enchant / player.info.maxHP),
										theName,
										(int) (100 * (enchant + 1) / player.info.maxHP));
								break;
							case A_REPRISAL:
								sprintf(buf2, "Any monster who attacks you will itself be wounded by %i%% of the damage that it inflicts. (If the %s is enchanted, this maximum percentage will increase to %i%%.) ",
										max(10, (int) (enchant * 10)),
										theName,
										max(10, (int) ((enchant + 1) * 10)));
								break;
							case A_IMMUNITY:
								sprintf(buf2, "It offers complete protection from any attacking %s. ",
										monsterCatalog[theItem->vorpalEnemy].monsterName);
								break;
							case A_BURDEN:
								strcpy(buf2, "10% of the time it absorbs a blow, it will permanently become heavier. ");
								break;
							case A_VULNERABILITY:
								strcpy(buf2, "While it is worn, inbound attacks will inflict twice as much damage. ");
								break;
							default:
								break;
						}
						strcat(buf, buf2);
					} else if (theItem->flags & ITEM_IDENTIFIED) {
						sprintf(buf2, "\n\nGlowing runes of an unrecognizable language spiral around the %s. ",
								theName);
						strcat(buf, buf2);
					}
				}
				
				// equipped? cursed?
				if (theItem->flags & ITEM_EQUIPPED) {
					sprintf(buf2, "\n\nYou are wearing the %s%s. ",
							theName,
							((theItem->flags & ITEM_CURSED) ? ", and because it is cursed, you are powerless to remove it" : ""));
					strcat(buf, buf2);
				} else if (((theItem->flags & (ITEM_IDENTIFIED | ITEM_MAGIC_DETECTED)) || rogue.playbackOmniscience)
						   && (theItem->flags & ITEM_CURSED)) {
					sprintf(buf2, "\n\nYou can feel a malevolent magic lurking within the %s. ", theName);
					strcat(buf, buf2);
				}
				
			}
			break;
			
		case STAFF:
			enchant = theItem->enchant1;
			
			// charges
			if ((theItem->flags & ITEM_IDENTIFIED)  || rogue.playbackOmniscience) {
				sprintf(buf2, "\n\nThe %s has %i charges remaining out of a maximum of %i charges, and like all staffs, recovers its charges gradually over time. ",
						theName,
						theItem->charges,
						enchant);
				strcat(buf, buf2);
			} else if (theItem->flags & ITEM_MAX_CHARGES_KNOWN) {
				sprintf(buf2, "\n\nThe %s has a maximum of %i charges, and like all staffs, recovers its charges gradually over time. ",
						theName,
						enchant);
				strcat(buf, buf2);
			}
			
			// effect description
			if ((theItem->flags & (ITEM_IDENTIFIED | ITEM_MAX_CHARGES_KNOWN)
				 && staffTable[theItem->kind].identified)
				|| rogue.playbackOmniscience) {
				switch (theItem->kind) {
						// STAFF_LIGHTNING, STAFF_FIRE, STAFF_POISON, STAFF_TUNNELING, STAFF_BLINKING, STAFF_ENTRANCEMENT, STAFF_HEALING,
						// STAFF_HASTE, STAFF_OBSTRUCTION, STAFF_DISCORD, STAFF_CONJURATION
					case STAFF_LIGHTNING:
						sprintf(buf2, "The lightning that arcs from this staff deals a random amount of damage to each creature that it hits equal, on average, to %i%% of your maximum health. (If the staff is enchanted, this will increase to %i%%.)",
								(int) (100 * (staffDamageLow(enchant) + staffDamageHigh(enchant)) / 2 / player.info.maxHP),
								(int) (100 * (staffDamageLow(enchant + 1) + staffDamageHigh(enchant + 1)) / 2 / player.info.maxHP));
						break;
					case STAFF_FIRE:
						damagePercent = (int) (100 * (staffDamageLow(enchant) + staffDamageHigh(enchant)) / 2 / player.info.maxHP);
						sprintf(buf2, "The flames that leap from this staff deal a random amount of damage equal, on average, to %i%% of your maximum health. (If the staff is enchanted, this will increase to %i%%.) They also set creatures and flammable terrain on fire.",
								(int) (100 * (staffDamageLow(enchant) + staffDamageHigh(enchant)) / 2 / player.info.maxHP),
								(int) (100 * (staffDamageLow(enchant + 1) + staffDamageHigh(enchant + 1)) / 2 / player.info.maxHP));
						break;
					case STAFF_POISON:
						sprintf(buf2, "The eerie green light that shines from this staff will poison any creature that it hits for %i turns. (If the staff is enchanted, this will increase to %i turns.)",
								staffPoison(enchant),
								staffPoison(enchant + 1));
						break;
					case STAFF_TUNNELING:
						sprintf(buf2, "The bolt from this staff will dissolve %i layers of obstruction. (If the staff is enchanted, this will increase to %i layers.)",
								enchant,
								enchant + 1);
						break;
					case STAFF_BLINKING:
						sprintf(buf2, "This staff enables you to teleport up to %i meters. (If the staff is enchanted, this will increase to %i meters.) It recharges half as quickly as most other kinds of staffs.",
								staffBlinkDistance(enchant),
								staffBlinkDistance(enchant + 1));
						break;
					case STAFF_ENTRANCEMENT:
						sprintf(buf2, "The psionic beam that this staff emits will compel its target to mimic your movements for at least %i turns. (If the staff is enchanted, this will increase to %i turns.)",
								enchant,
								enchant + 1);
						break;
					case STAFF_HEALING:
						if (enchant < 10) {
							sprintf(buf2, "This staff's light will heal its target by %i%% of its maximum health. (If the staff is enchanted, this will increase to %i%%.)",
									enchant * 10,
									(enchant + 1) * 10);
						} else {
							strcpy(buf2, "This staff's light will completely heal its target.");	
						}
						break;
					case STAFF_HASTE:
						sprintf(buf2, "This staff's bolt of light will cause its target to move twice as fast for %i turns. (If the staff is enchanted, this will increase to %i turns.)",
								staffHasteDuration(enchant),
								staffHasteDuration(enchant + 1));
						break;
					case STAFF_OBSTRUCTION:
						strcpy(buf2, "This staff recharges half as quickly as most other kinds of staffs.");
						break;
					case STAFF_DISCORD:
						sprintf(buf2, "Creatures targeted by this staff will suffer its effects for %i turns. (If the staff is enchanted, this will increase to %i turns.)",
								staffDiscordDuration(enchant),
								staffDiscordDuration(enchant + 1));
						break;
					case STAFF_CONJURATION:
						sprintf(buf2, "%i phantom blades will be called into service. (If the staff is enchanted, this will increase to %i blades.)",
								staffBladeCount(enchant),
								staffBladeCount(enchant + 1));
						break;					
					default:
						strcpy(buf2, "No one knows what this staff does.");
						break;
				}
				strcat(buf, "\n\n");
				strcat(buf, buf2);
			}
			break;
			
		case WAND:
			strcat(buf, "\n\n");
			if (theItem->flags & ITEM_IDENTIFIED || rogue.playbackOmniscience) {
				if (theItem->charges) {
					sprintf(buf2, "%i charge%s remain. A charge can be added to the wand with a scroll of recharging or a scroll of enchantment.",
							theItem->charges,
							(theItem->charges == 1 ? "" : "s"));
				} else {
					strcpy(buf2, "No charges remain. A charge can be added to the wand with a scroll of recharging or a scroll of enchantment.");
				}
			} else {
				if (theItem->enchant2) {
					sprintf(buf2, "You have used this wand %i time%s, but do not know how many charges, if any, remain.",
							theItem->enchant2,
							(theItem->enchant2 == 1 ? "" : "s"));
				} else {
					strcpy(buf2, "You have not yet used this wand.");
				}
				
				if (wandTable[theItem->kind].identified) {
					strcat(buf, buf2);
					sprintf(buf2, " Wands of this type can be found with %i to %i charges.",
							wandTable[theItem->kind].range.lowerBound,
							wandTable[theItem->kind].range.upperBound);
				}
			}
			strcat(buf, buf2);
			break;
			
		case RING:
			// RING_CLAIRVOYANCE, RING_STEALTH, RING_REGENERATION, RING_TRANSFERENCE, RING_LIGHT, RING_AWARENESS, RING_REFLECTION
			if (((theItem->flags & ITEM_IDENTIFIED) || rogue.playbackOmniscience)
				&& theItem->enchant1) {
				switch (theItem->kind) {
					case RING_CLAIRVOYANCE:
						if (theItem->enchant1 > 0) {
							sprintf(buf2, "\n\nThis ring provides magical sight with a radius of %i. (If the ring is enchanted, this will increase to %i.)",
									theItem->enchant1 + 1,
									theItem->enchant1 + 2);
						} else {
							sprintf(buf2, "\n\nThis ring magically blinds you to a radius of %i. (If the ring is enchanted, this will decrease to %i.)",
									(theItem->enchant1 * -1) + 1,
									(theItem->enchant1 * -1));
						}
						strcat(buf, buf2);
						break;
					case RING_REGENERATION:
						sprintf(buf2, "\n\nWith this ring equipped, you will regenerate all of your health in %li turns (instead of %li). (If the ring is enchanted, this will decrease to %i turns.)",
								(long) (1000 * TURNS_FOR_FULL_REGEN * pow(0.75, theItem->enchant1) / 1000),
								(long) TURNS_FOR_FULL_REGEN,
								(long) (1000 * TURNS_FOR_FULL_REGEN * pow(0.75, theItem->enchant1 + 1) / 1000));
						strcat(buf, buf2);
						break;
					case RING_TRANSFERENCE:
						sprintf(buf2, "\n\nEach blow you land will %s you by %i%% of the damage you inflict. (If the ring is enchanted, this will %i to %i%%.)",
								(theItem->enchant1 > 0 ? "heal" : "harm"),
								abs(theItem->enchant1) * 10,
								(theItem->enchant1 > 0 ? "increase" : "decrease"),
								abs(theItem->enchant1 + 1) * 10);
						strcat(buf, buf2);
						break;
					case RING_REFLECTION:
						if (theItem->enchant1 > 0) {
							short reflectChance = 100 - (short) (100 * pow(0.85, theItem->enchant1));
							short reflectChance2 = 100 - (short) (100 * pow(0.85, theItem->enchant1 + 1));
							sprintf(buf2, "\n\nWhen worn, you will deflect %i%% of incoming spells and projectiles -- including directly back at their source %i%% of the time. (If the ring is enchanted, these will increase to %i%% and %i%%.)",
									reflectChance,
									reflectChance * reflectChance / 100,
									reflectChance2,
									reflectChance2 * reflectChance2 / 100);
						} else {
							short reflectChance = 100 - (short) (100 * pow(0.85, -1 * theItem->enchant1));
							short reflectChance2 = 100 - (short) (100 * pow(0.85, -1 * (theItem->enchant1 + 1)));
							sprintf(buf2, "\n\nWhen worn, %i%% of your own spells and projectiles will deflect from their target -- including directly back at you %i%% of the time. (If the ring is enchanted, these will decrease to %i%% and %i%%.)",
									reflectChance,
									reflectChance * reflectChance / 100,
									reflectChance2,
									reflectChance2 * reflectChance2 / 100);
						}
						strcat(buf, buf2);
						break;
					default:
						break;
				}
			}
			
			// equipped? cursed?
			if (theItem->flags & ITEM_EQUIPPED) {
				sprintf(buf2, "\n\nThe %s is on your finger%s. ",
						theName,
						((theItem->flags & ITEM_CURSED) ? ", and because it is cursed, you are powerless to remove it" : ""));
				strcat(buf, buf2);
			} else if (((theItem->flags & (ITEM_IDENTIFIED | ITEM_MAGIC_DETECTED)) || rogue.playbackOmniscience)
					   && (theItem->flags & ITEM_CURSED)) {
				sprintf(buf2, "\n\nYou can feel a malevolent magic lurking within the %s. ", theName);
				strcat(buf, buf2);
			}
			break;
			
		default:
			break;
	}
	
	if (carried) {
		sprintf(buf2, "\n\n%s can be ",
				(singular ? "This item" : "These items"));
		strcat(buf, buf2);
		if (theItem->category & (SCROLL | POTION | WAND | STAFF)) {
			strcat(buf, "(a)pplied, ");
		}
		if (theItem->category & (ARMOR | WEAPON | RING)) {
			if (theItem->flags & ITEM_EQUIPPED) {
				strcat(buf, "(r)emoved, ");
			} else {
				strcat(buf, "(e)quipped, ");
			}
		}
		strcat(buf, "(d)ropped or (t)hrown.");
	}
}

char displayInventory(unsigned short categoryMask,
					  unsigned long requiredFlags,
					  unsigned long forbiddenFlags,
					  boolean waitForAcknowledge) {
	item *theItem;
	short i, j, maxLength = 0, itemNumber = 0;
	char descriptionList[DROWS][DCOLS];
	color itemColor[DROWS];
	color itemNameColor[DROWS];
	char displayedItemLettersList[DROWS];
	item *itemList[DROWS];
	char buf[DCOLS];
	char itemDescription[COLS * 100];
	char theKey;
	rogueEvent theEvent;
	color backgroundColor, foregroundColor;
	uchar bufChar;
	boolean magicDetected, repeatDisplay;
	short highlightItemLine, itemSpaceRemaining;
	cellDisplayBuffer dbuf[COLS][ROWS];
	cellDisplayBuffer rbuf[COLS][ROWS];
	short oldRNG;
	
	oldRNG = rogue.RNG;
	rogue.RNG = RNG_COSMETIC;
	
	clearDisplayBuffer(dbuf);
	
	if (packItems->nextItem == NULL) {
		confirmMessages();
		message("Your pack is empty!", true, false);
		rogue.RNG = oldRNG;
		return 0;
	}
	
	magicDetected = false;
	for (theItem = packItems->nextItem; theItem != NULL; theItem = theItem->nextItem) {
		updateItemKnownMagicStatus(theItem);
		if ((theItem->flags & ITEM_MAGIC_DETECTED) && !(theItem->flags & ITEM_IDENTIFIED)) {
			magicDetected = true;
		}
	}
	
	for (theItem = packItems->nextItem; theItem != NULL; theItem = theItem->nextItem) {
		
		itemColor[itemNumber] = *theItem->foreColor;
		itemNameColor[itemNumber] = *theItem->inventoryColor;
		
		if (theItem->category & categoryMask &&
			!(~(theItem->flags) & requiredFlags) &&
			!(theItem->flags & forbiddenFlags)) {
			
		} else {
			applyColorAverage(&itemColor[itemNumber], &black, 50);
			applyColorAverage(&itemNameColor[itemNumber], &black, 50);
		}
		
		itemName(theItem, buf, true, true); // name of item with suffix, article
		upperCase(buf);
		sprintf(descriptionList[itemNumber], " %c%s%s x %s%s",
				theItem->inventoryLetter,
				(theItem->flags & ITEM_PROTECTED ? "}" : ")"),
				(magicDetected ? "  " : ""),
				buf,
				(theItem->flags & ITEM_EQUIPPED ? (theItem->category & WEAPON ? " (in hand)" : " (being worn)") : ""));
		maxLength = max(maxLength, (strlen(descriptionList[itemNumber])));
		displayedItemLettersList[itemNumber] = theItem->inventoryLetter;
		itemList[itemNumber] = theItem;
		itemNumber++;
	}
	if (!itemNumber) {
		confirmMessages();
		message("Nothing of that type!", true, false);
		rogue.RNG = oldRNG;
		return 0;
	}
	if (waitForAcknowledge) {
		itemNameColor[itemNumber] = gray;
		itemSpaceRemaining = MAX_PACK_ITEMS - numberOfItemsInPack();
		if (itemSpaceRemaining) {
			sprintf(descriptionList[itemNumber++], "%s    You have room for %i more item%s.",
					(magicDetected ? "  " : ""),
					itemSpaceRemaining,
					(itemSpaceRemaining > 1 ? "s" : ""));
		} else {
			sprintf(descriptionList[itemNumber++], "%s    Your pack is full.", (magicDetected ? "  " : ""));	
		}
		maxLength = max(maxLength, (strlen(descriptionList[itemNumber - 1])));
		
		itemNameColor[itemNumber] = white;
		sprintf(descriptionList[itemNumber++], "%s -- press shift-(A-Z) for more info -- ", (magicDetected ? "  " : ""));
		maxLength = max(maxLength, (strlen(descriptionList[itemNumber - 1])));
	}
		
	for (i=0; i < itemNumber; i++) {
		for (j=0; descriptionList[i][j] != '\0' && j <= DCOLS; j++) {
			getCellAppearance(DCOLS - maxLength + j, i, &bufChar, &foregroundColor, &backgroundColor);
			//applyColorAverage(&backgroundColor, &black, 85);
			
			if (j == 4 && magicDetected
				&& (!waitForAcknowledge || i < itemNumber - 2)
				&& (itemList[i]->flags & ITEM_MAGIC_DETECTED)
				&& !(itemList[i]->flags & ITEM_IDENTIFIED)
				&& !(itemList[i]->category & (AMULET|FOOD|GEM))) {
				plotCharToBuffer((itemMagicChar(itemList[i]) ? itemMagicChar(itemList[i]) : '-'),
								 DCOLS - maxLength + j + STAT_BAR_WIDTH,
								 i + MESSAGE_LINES,
								 &itemColor[i],
								 //&backgroundColor,
								 &black,
								 dbuf);
			} else if (j == 4 + (magicDetected ? 2 : 0) && (!waitForAcknowledge || i < itemNumber - 2)) { // item character
				plotCharToBuffer(itemList[i]->displayChar,
								 DCOLS - maxLength + j + STAT_BAR_WIDTH,
								 i + MESSAGE_LINES,
								 &itemColor[i],
								 //&backgroundColor,
								 &black,
								 dbuf);
			} else {
				plotCharToBuffer(descriptionList[i][j],
								 DCOLS - maxLength + j + STAT_BAR_WIDTH,
								 i + MESSAGE_LINES,
								 &itemNameColor[i],
								 //&backgroundColor,
								 &black,
								 dbuf);
			}
			
			dbuf[DCOLS - maxLength + j + STAT_BAR_WIDTH][i + MESSAGE_LINES].opacity = 85;
		}
		for (j = strlen(descriptionList[i]); j < maxLength; j++) {
			// fill with dark color from the end of the line to the edge of the screen
			plotCharToBuffer(' ',
							 DCOLS - maxLength + j + STAT_BAR_WIDTH,
							 i + MESSAGE_LINES,
							 &black,
							 &black,
							 dbuf);
			
			dbuf[DCOLS - maxLength + j + STAT_BAR_WIDTH][i + MESSAGE_LINES].opacity = 85;
		}
	}
	
	overlayDisplayBuffer(dbuf, rbuf);
	
	do {
		repeatDisplay = false;
		
		highlightItemLine = -1;
		theKey = 0;
		
		do {
			nextBrogueEvent(&theEvent, false, false);
		} while (theEvent.eventType != KEYSTROKE && !( theEvent.eventType == MOUSE_UP && theEvent.param1 >= DCOLS - maxLength + STAT_BAR_WIDTH
													  && theEvent.param2 >= MESSAGE_LINES && theEvent.param2 < itemNumber + MESSAGE_LINES - 2));
		if (theEvent.eventType == KEYSTROKE) {
			theKey = theEvent.param1;
			if (theKey >= 'A' && theKey <= 'Z') {
				theKey -= 'A' - 'a';
			}
		} else if (theEvent.eventType == MOUSE_UP) {
			theKey = displayedItemLettersList[theEvent.param2 - MESSAGE_LINES];
		}
		
		for (theItem = packItems->nextItem, i = 0; theItem != NULL; theItem = theItem->nextItem) {
			if (theItem->inventoryLetter == theKey) {
				highlightItemLine = i;
				break;
			}
			i++;
		}
		
		// was an item selected?
		if (highlightItemLine > -1 && (!waitForAcknowledge || theEvent.shiftKey || theEvent.controlKey)) {
			// yes. Briefly highlight the selected item.
			for (i = 0; descriptionList[highlightItemLine][i] != '\0'; i++) {
				getCellAppearance(i + DCOLS - maxLength, highlightItemLine,
								  &bufChar, &foregroundColor, &backgroundColor);
				applyColorAverage(&backgroundColor, &black, 85);
				applyColorAverage(&backgroundColor, &gray, 25);
				if (descriptionList[highlightItemLine][i] == ' ') {
					bufChar = ' ';
				} else {
					bufChar = displayBuffer[i + COLS - maxLength][highlightItemLine + MESSAGE_LINES].character;
				}
				plotCharWithColor(bufChar, i + COLS - maxLength, highlightItemLine + MESSAGE_LINES, yellow, backgroundColor);
			}
			
			if (theEvent.shiftKey || theEvent.controlKey) {
				repeatDisplay = true;
				// display information window about item
				itemDetails(itemDescription, theItem);
				printTextBox(itemDescription, max(2, DCOLS - maxLength + STAT_BAR_WIDTH - 42), MESSAGE_LINES + 2, 40, &white, &darkGray, NULL);
				waitForKeystrokeOrMouseClick();
				overlayDisplayBuffer(rbuf, NULL); // remove the item info window
				overlayDisplayBuffer(dbuf, NULL); // redisplay the inventory
			} else {
				pauseBrogue(rogue.playbackMode ? rogue.playbackDelayPerTurn * 10 : 50);
			}
		}
	} while (repeatDisplay); // so you can get info on multiple items sequentially
	
	overlayDisplayBuffer(rbuf, NULL); // restore the original screen

	rogue.RNG = oldRNG;
	return theKey;
}

short numberOfMatchingPackItems(unsigned short categoryMask,
								unsigned long requiredFlags, unsigned long forbiddenFlags,
								boolean displayErrors) {
	item *theItem;
	short matchingItemCount = 0;
	
	if (packItems->nextItem == NULL) {
		if (displayErrors) {
			confirmMessages();
			message("Your pack is empty!", true, false);
		}
		return 0;
	}
	
	for (theItem = packItems->nextItem; theItem != NULL; theItem = theItem->nextItem) {
		
		if (theItem->category & categoryMask &&
			!(~(theItem->flags) & requiredFlags) &&
			!(theItem->flags & forbiddenFlags)) {
			
			matchingItemCount++;
		}
	}
	
	if (matchingItemCount == 0) {
		if (displayErrors) {
			confirmMessages();
			message("You have nothing suitable.", true, false);
		}
		return 0;
	}
	
	return matchingItemCount;
}

void updateEncumbrance() {
	short moveSpeed, attackSpeed;
	
	moveSpeed = player.info.movementSpeed;
	attackSpeed = player.info.attackSpeed;
	
	/*if (rogue.weapon && rogue.weapon->strengthRequired > rogue.strength) {
	 attackSpeed += 25 * (rogue.weapon->strengthRequired - rogue.strength);
	 }
	 
	 if (rogue.armor && rogue.armor->strengthRequired > rogue.strength) {
	 moveSpeed += 25 * (rogue.armor->strengthRequired - rogue.strength);
	 }*/
	
	if (player.status.hasted) {
		moveSpeed /= 2;
		attackSpeed /= 2;
	} else if (player.status.slowed) {
		moveSpeed *= 2;
		attackSpeed *= 2;
	}
	
	player.movementSpeed = moveSpeed;
	player.attackSpeed = attackSpeed;
	
	recalculateEquipmentBonuses();
}

void strengthCheck(item *theItem) {
	char buf1[COLS], buf2[COLS*2];
	short strengthDeficiency;
	
	updateEncumbrance();
	if (theItem) {
		if (theItem->category & WEAPON && theItem->strengthRequired > rogue.strength) {
			strengthDeficiency = theItem->strengthRequired - rogue.strength;
			strcpy(buf1, "");
			itemName(theItem, buf1, false, false);
			sprintf(buf2, "You can barely lift the %s; %i more strength would be ideal.", buf1, strengthDeficiency);
			message(buf2, true, false);
		}
		
		if (theItem->category & ARMOR && theItem->strengthRequired > rogue.strength) {
			strengthDeficiency = theItem->strengthRequired - rogue.strength;
			strcpy(buf1, "");
			itemName(theItem, buf1, false, false);
			sprintf(buf2, "You stagger under the weight of the %s; %i more strength would be ideal.",
					buf1, strengthDeficiency);
			message(buf2, true, false);
		}
	}
}

void equip() {
	char buf1[COLS], buf2[COLS];
	unsigned char command[10];
	short c = 0;
	item *theItem, *theItem2;
	
	command[c++] = EQUIP_KEY;
	theItem = promptForItemOfType((WEAPON|ARMOR|RING), 0, ITEM_EQUIPPED, "Equip what? (a-z, shift for more info; or <esc> to cancel)");
	if (theItem == NULL) {
		return;
	}
	
	command[c++] = theItem->inventoryLetter;
	
	if (theItem->category & (WEAPON|ARMOR|RING)) {
		
		if (theItem->category & RING) {
			if (theItem->flags & ITEM_EQUIPPED) {
				confirmMessages();
				message("you are already wearing that ring.", true, false);
				return;
			} else if (rogue.ringLeft && rogue.ringRight) {
				confirmMessages();
				theItem2 = promptForItemOfType((RING), ITEM_EQUIPPED, 0, "You are already wearing two rings; remove which first?");
				if (!theItem2 || theItem2->category != RING || !(theItem2->flags & ITEM_EQUIPPED)) {
					message("Invalid entry.", true, false);
					return;
				} else {
					if (!unequipItem(theItem2, false)) {
						itemName(theItem2, buf1, false, false);
						sprintf(buf2, "You can't remove your %s: it appears to be cursed.", buf1);
						confirmMessages();
						message(buf2, true, false);
						return;
					}
					command[c++] = theItem2->inventoryLetter;
				}
			}
		}
		
		if (theItem->flags & ITEM_EQUIPPED) {
			confirmMessages();
			message("already equipped.", true, false);
			return;
		}
		
		if (!equipItem(theItem, false)) { // this is where the item gets equipped
			// equip failed because current item is cursed
			if (theItem->category & WEAPON) {
				itemName(rogue.weapon, buf1, false, false);
			} else if (theItem->category & ARMOR) {
				itemName(rogue.armor, buf1, false, false);
			} else {
				sprintf(buf1, "one");
			}
			sprintf(buf2, "You can't; the %s you are using appears to be cursed.", buf1);
			confirmMessages();
			message(buf2, true, false);
			return;
		}	
		itemName(theItem, buf2, true, true);
		sprintf(buf1, "Now %s %s.", (theItem->category & WEAPON ? "wielding" : "wearing"), buf2);
		confirmMessages();
		message(buf1, true, false);
		
		strengthCheck(theItem);
		
		if (theItem->flags & ITEM_CURSED) {
			itemName(theItem, buf2, false, false);
			switch(theItem->category) {
				case WEAPON:
					sprintf(buf1, "you wince as your grip involuntarily tightens around your %s.", buf2);
					break;
				case ARMOR:
					sprintf(buf1, "your %s constricts around you painfully.", buf2);
					break;
				case RING:
					sprintf(buf1, "your %s tightens around your finger painfully.", buf2);
					break;
				default:
					sprintf(buf1, "your %s seizes you with a malevolent force.", buf2);
					break;
			}
			message(buf1, true, false);
		}
		command[c] = '\0';
		recordKeystrokeSequence(command);
		playerTurnEnded();
	} else {
		confirmMessages();
		message("You can't equip that.", true, false);
	}
}

void aggravateMonsters() {
	creature *monst, *pickedMonst = NULL;
	short i, j, **grid;
	for (monst=monsters->nextCreature; monst != NULL; monst = monst->nextCreature) {
		if (monst->creatureState == MONSTER_SLEEPING) {
			wakeUp(monst);
		}
		if (monst->creatureState != MONSTER_ALLY) {
			monst->creatureState = MONSTER_TRACKING_SCENT;
		}
		if (!(monst->info.flags & (MONST_FLIES | MONST_IMMUNE_TO_FIRE | MONST_RESTRICTED_TO_LIQUID | MONST_IMMUNE_TO_WATER))) {
			pickedMonst = monst;
		}
	}
	
	if (pickedMonst) {
		grid = allocDynamicGrid();
		
		calculateDistances(grid, player.xLoc, player.yLoc, 0, pickedMonst);
		
		for (i=0; i<DCOLS; i++) {
			for (j=0; j<DROWS; j++) {
				if (grid[i][j] >= 0 && grid[i][j] < 30000) {
					tmap[i][j].scent = 0;
					addScentToCell(i, j, 2 * grid[i][j]);
				}
			}
		}
		
		freeDynamicGrid(grid);
	}
}

void goBlind() {
	short i, j;
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			pmap[i][j].flags &= ~(VISIBLE | IN_FIELD_OF_VIEW);
			refreshDungeonCell(i, j);
		}
	}
}

// returns the number of items on the list; includes (-1, -1) as an additional terminus indicator after the end of the list
short getLineCoordinates(short listOfCoordinates[][2], short originLoc[2], short targetLoc[2]) {
	float targetVector[2], error[2];
	short largerTargetComponent, currentVector[2], previousVector[2], quadrantTransform[2], i;
	short currentLoc[2], previousLoc[2];
	short cellNumber = 0;
	
	if (originLoc[0] == targetLoc[0] && originLoc[1] == targetLoc[1]) {
		return 0;
	}
	
	// Neither vector is negative. We keep track of negatives with quadrantTransform.
	for (i=0; i<= 1; i++) {
		targetVector[i] = targetLoc[i] - originLoc[i];
		if (targetVector[i] < 0) {
			targetVector[i] *= -1;
			quadrantTransform[i] = -1;
		} else {
			quadrantTransform[i] = 1;
		}
		currentVector[i] = previousVector[i] = error[i] = 0;
		currentLoc[i] = originLoc[i];
	}
	
	// normalize target vector such that one dimension equals 1 and the other is in [0, 1].
	largerTargetComponent = max(targetVector[0], targetVector[1]);
	targetVector[0] /= largerTargetComponent;
	targetVector[1] /= largerTargetComponent;
	
	do {
		for (i=0; i<= 1; i++) {
			
			previousLoc[i] = currentLoc[i];
			
			currentVector[i] += targetVector[i];
			error[i] += (targetVector[i] == 1 ? 0 : targetVector[i]);
			
			if (error[i] >= 0.5) {
				currentVector[i]++;
				error[i] -= 1;
			}
			
			currentLoc[i] = quadrantTransform[i]*currentVector[i] + originLoc[i];
			
			listOfCoordinates[cellNumber][i] = currentLoc[i];
		}
		
		//DEBUG printf("\ncell %i: (%i, %i)", cellNumber, listOfCoordinates[cellNumber][0], listOfCoordinates[cellNumber][1]);
		cellNumber++;
		
	} while (coordinatesAreInMap(currentLoc[0], currentLoc[1]));
	
	cellNumber--;
	
	listOfCoordinates[cellNumber][0] = listOfCoordinates[cellNumber][1] = -1; // demarcates the end of the list
	return cellNumber;
}

void getImpactLoc(short returnLoc[2], short originLoc[2], short targetLoc[2],
				  short maxDistance, boolean returnLastEmptySpace) {
	float targetVector[2], error[2];
	short largerTargetComponent, currentVector[2], previousVector[2], quadrantTransform[2], i;
	short currentLoc[2], previousLoc[2];
	creature *monst;
	
	monst = NULL;
	
	// Neither vector is negative. We keep track of negatives with quadrantTransform.
	for (i=0; i<= 1; i++) {
		targetVector[i] = targetLoc[i] - originLoc[i];
		if (targetVector[i] < 0) {
			targetVector[i] *= -1;
			quadrantTransform[i] = -1;
		} else {
			quadrantTransform[i] = 1;
		}
		currentVector[i] = previousVector[i] = error[i] = 0;
		currentLoc[i] = originLoc[i];
	}
	
	// normalize target vector such that one dimension equals 1 and the other is in [0, 1].
	largerTargetComponent = max(targetVector[0], targetVector[1]);
	targetVector[0] /= largerTargetComponent;
	targetVector[1] /= largerTargetComponent;
	
	do {
		for (i=0; i<= 1; i++) {
			
			previousLoc[i] = currentLoc[i];
			
			currentVector[i] += targetVector[i];
			error[i] += (targetVector[i] == 1 ? 0 : targetVector[i]);
			
			if (error[i] >= 0.5) {
				currentVector[i]++;
				error[i] -= 1;
			}
			
			currentLoc[i] = quadrantTransform[i]*currentVector[i] + originLoc[i];
		}
		
		if (currentLoc[0] >= DCOLS-1 || currentLoc[0] <= 0 || currentLoc[1] >= DROWS-1 || currentLoc[1] <= 0) {
			break;
		}
		
		if (pmap[currentLoc[0]][currentLoc[1]].flags & HAS_MONSTER) {
			monst = monsterAtLoc(currentLoc[0], currentLoc[1]);
		}
		
	} while ((!(pmap[currentLoc[0]][currentLoc[1]].flags & HAS_MONSTER)
			  || !monst
			  || ((monst->info.flags & MONST_INVISIBLE) || (monst->bookkeepingFlags & MONST_SUBMERGED)))
			 && !(pmap[currentLoc[0]][currentLoc[1]].flags & HAS_PLAYER)
			 && !cellHasTerrainFlag(currentLoc[0], currentLoc[1], (OBSTRUCTS_VISION | OBSTRUCTS_PASSABILITY))
			 && max(currentVector[0], currentVector[1]) <= maxDistance);
	
	if (returnLastEmptySpace) {
		returnLoc[0] = previousLoc[0];
		returnLoc[1] = previousLoc[1];
	} else {
		returnLoc[0] = currentLoc[0];
		returnLoc[1] = currentLoc[1];
	}
}

boolean tunnelize(short x, short y) {
	enum dungeonLayers layer;
	boolean didSomething = false;
	creature *monst;
	
	for (layer = 0; layer < NUMBER_TERRAIN_LAYERS; layer++) {
		if (tileCatalog[pmap[x][y].layers[layer]].flags & (OBSTRUCTS_PASSABILITY | OBSTRUCTS_VISION)) {
			pmap[x][y].layers[layer] = (layer == DUNGEON ? FLOOR : NOTHING);
			didSomething = true;
		}
	}
	if (didSomething) {
		spawnDungeonFeature(x, y, &dungeonFeatureCatalog[DF_RUBBLE], true);
		if (pmap[x][y].flags & HAS_MONSTER) {
			monst = monsterAtLoc(x, y);
			if (monst->info.flags & MONST_ATTACKABLE_THRU_WALLS) {
				inflictDamage(&player, monst, monst->currentHP, NULL);
			}
		}
	}
	return didSomething;
}

void cancelMagic(creature *monst) {
	// works on inanimates
	monst->info.abilityFlags = 0; // canceled monsters lose all special abilities
	monst->status.immuneToFire = 0;
	monst->status.slowed = 0;
	monst->status.hasted = 0;
	monst->status.entranced = 0;
	monst->status.discordant = 0;
	if (monst == &player) {
		monst->status.telepathic = min(monst->status.telepathic, 1);
		monst->status.magicalFear = min(monst->status.magicalFear, 1);
		monst->status.levitating = min(monst->status.levitating, 1);
		if (monst->status.darkness) {
			monst->status.darkness = 0;
			updateMinersLightRadius();
			updateVision();
		}
	} else {
		monst->status.telepathic = 0;
		monst->status.magicalFear = 0;
		monst->status.levitating = 0;
	}
	monst->info.flags &= ~MONST_IMMUNE_TO_FIRE;
	monst->movementSpeed = monst->info.movementSpeed;
	monst->attackSpeed = monst->info.attackSpeed;
	if (monst->info.flags & MA_POISONS) {
		monst->info.damage.lowerBound = monst->info.damage.upperBound = monst->info.damage.clumpFactor = 1;
	}
	if (monst != &player && monst->info.flags & CANCELLABLE_TRAITS) {
		if (monst->info.flags & MONST_FIERY && monst->status.burning) {
			extinguishFireOnCreature(monst);
		}
		monst->info.flags &= ~CANCELLABLE_TRAITS;
		refreshDungeonCell(monst->xLoc, monst->yLoc);
		refreshSideBar(NULL);
	}
	applyInstantTileEffectsToCreature(monst); // in case it should immediately die or fall into a chasm
}

void slow(creature *monst, short turns) {
	if (!(monst->info.flags & MONST_INANIMATE)) {
		monst->status.slowed = monst->maxStatus.slowed = turns;
		monst->status.hasted = 0;
		if (monst == &player) {
			updateEncumbrance();
			message("you feel yourself slow down.", true, false);
		} else {
			monst->movementSpeed = monst->info.movementSpeed * 2;
			monst->attackSpeed = monst->info.attackSpeed * 2;
		}
	}
}

void haste(creature *monst, short turns) {
	if (monst && !(monst->info.flags & MONST_INANIMATE)) {
		monst->status.slowed = 0;
		monst->status.hasted = monst->maxStatus.hasted = turns;
		if (monst == &player) {
			updateEncumbrance();
			message("you feel yourself speed up.", true, false);
		} else {
			monst->movementSpeed = monst->info.movementSpeed / 2;
			monst->attackSpeed = monst->info.attackSpeed / 2;
		}
	}
}

void heal(creature *monst, short percent) {	
	char buf[COLS], monstName[COLS];
	if (!(monst->info.flags & MONST_INANIMATE)) {
		monst->currentHP = min(monst->info.maxHP, monst->currentHP + percent * monst->info.maxHP / 100);
		if (canSeeMonster(monst) && monst != &player) {
			monsterName(monstName, monst, true);
			sprintf(buf, "%s looks healthier!", monstName);
			message(buf, true, false);
		}
	}
}

boolean projectileReflects(creature *attacker, creature *defender) {	
	short prob, netReflectionLevel;
	
	// immunity armor always reflects its vorpal enemy's projectiles
	if (defender == &player && rogue.armor && (rogue.armor->flags & ITEM_RUNIC) && rogue.armor->enchant2 == A_IMMUNITY
		&& rogue.armor->vorpalEnemy == attacker->info.monsterID) {
		return true;
	}
	
	netReflectionLevel = (defender == &player ? rogue.reflectionBonus : 0);
	
	if (attacker == &player && rogue.reflectionBonus < 0) {
		netReflectionLevel -= rogue.reflectionBonus; // (subtracting a negative)
	}
	
	if (defender && (defender->info.flags & MONST_REFLECT_4)) {
		netReflectionLevel += 4;
	}
	
	if (netReflectionLevel <= 0) {
		return false;
	}
	
	prob = 100 - (short) (100 * pow(0.85, netReflectionLevel));
	
	return rand_percent(prob);
}

// returns the path length of the reflected path, alters listOfCoordinates to describe reflected path
short reflectBolt(short targetX, short targetY, short listOfCoordinates[][2], short kinkCell, boolean retracePath) {
	short k, target[2], origin[2], newPath[DCOLS][2], newPathLength, failsafe, finalLength;
	boolean needRandomTarget;
	
	needRandomTarget = (targetX < 0 || targetY < 0
						|| (targetX == listOfCoordinates[kinkCell][0] && targetY == listOfCoordinates[kinkCell][1]));
	
	if (retracePath) {
		// if reflecting back at caster, follow precise trajectory until we reach the caster
		for (k = 1; k <= kinkCell && kinkCell + k < MAX_BOLT_LENGTH; k++) {
			listOfCoordinates[kinkCell + k][0] = listOfCoordinates[kinkCell - k][0];
			listOfCoordinates[kinkCell + k][1] = listOfCoordinates[kinkCell - k][1];
		}
		
		// Calculate a new "extension" path, with an origin at the caster, and a destination at
		// the caster's location translated by the vector from the reflection point to the caster.
		// 
		// For example, if the player is at (0,0), and the caster is at (2,3), then the newpath
		// is from (2,3) to (4,6):
		// (2,3) + ((2,3) - (0,0)) = (4,6).
		
		origin[0] = listOfCoordinates[2 * kinkCell][0];
		origin[1] = listOfCoordinates[2 * kinkCell][1];
		target[0] = targetX + (targetX - listOfCoordinates[kinkCell][0]);
		target[1] = targetY + (targetY - listOfCoordinates[kinkCell][1]);
		newPathLength = getLineCoordinates(newPath, origin, target);
		
		for (k=0; k<=newPathLength; k++) { // USED TO BE K<NEWPATHLENGTH
			listOfCoordinates[2 * kinkCell + k + 1][0] = newPath[k][0];
			listOfCoordinates[2 * kinkCell + k + 1][1] = newPath[k][1];
		}
		finalLength = 2 * kinkCell + newPathLength + 1;
	} else {
		failsafe = 50;
		do {
			if (needRandomTarget) {
				// pick random target
				perimeterCoords(target, rand_range(0, 40));
				target[0] += listOfCoordinates[kinkCell][0];
				target[1] += listOfCoordinates[kinkCell][1];
			} else {
				target[0] = targetX;
				target[1] = targetY;
			}
			
			newPathLength = getLineCoordinates(newPath, listOfCoordinates[kinkCell], target);
			
			if (!cellHasTerrainFlag(newPath[0][0], newPath[0][1], (OBSTRUCTS_VISION | OBSTRUCTS_PASSABILITY))) {
				needRandomTarget = false;
			}
			
		} while (needRandomTarget && --failsafe);
		
		for (k = 0; k < newPathLength; k++) {
			listOfCoordinates[kinkCell + k + 1][0] = newPath[k][0];
			listOfCoordinates[kinkCell + k + 1][1] = newPath[k][1];
		}
		
		finalLength = kinkCell + newPathLength + 1;
	}
	
	listOfCoordinates[finalLength][0] = -1;
	listOfCoordinates[finalLength][1] = -1;
	return finalLength;
}

// returns whether the bolt effect should autoID any staff or wand it came from, if it came from a staff or wand
boolean zap(short originLoc[2], short targetLoc[2], enum boltType bolt, short boltLevel, boolean hideDetails) {
	short listOfCoordinates[MAX_BOLT_LENGTH][2];
	short i, j, k, x, y, numCells, blinkDistance, boltLength, initialBoltLength, newLoc[2];
	float healthFraction;
	short previousExperience, poisonDamage;
	creature *monst = NULL, *shootingMonst, *newMonst;
	char buf[COLS], monstName[COLS];
	boolean autoID = false;
	boolean fastForward = false;
	boolean beckonedMonster = false;
	boolean alreadyReflected = false;
	boolean boltInView;
	color *boltColor;
	dungeonFeature feat;
	enum directions dir;
	
	x = originLoc[0];
	y = originLoc[1];
	
	initialBoltLength = boltLength = 5 * boltLevel;
	
	lightSource boltLights[initialBoltLength];
	color boltLightColors[initialBoltLength];
	
	numCells = getLineCoordinates(listOfCoordinates, originLoc, targetLoc);
	
	shootingMonst = monsterAtLoc(originLoc[0], originLoc[1]);
	
	if (!hideDetails) {
		boltColor = boltColors[bolt];
	} else {
		boltColor = &gray;
	}
	
	if (bolt == BOLT_BLINKING) {
		if (cellHasTerrainFlag(listOfCoordinates[0][0], listOfCoordinates[0][1], (OBSTRUCTS_PASSABILITY | OBSTRUCTS_VISION))
			|| (pmap[listOfCoordinates[0][0]][listOfCoordinates[0][1]].flags & (HAS_PLAYER | HAS_MONSTER)
				&& !(monsterAtLoc(listOfCoordinates[0][0], listOfCoordinates[0][1])->bookkeepingFlags & MONST_SUBMERGED))) {
				// shooting blink point-blank into an obstruction does nothing.
				return false;
			}
		pmap[originLoc[0]][originLoc[1]].flags &= ~(HAS_PLAYER | HAS_MONSTER);
		refreshDungeonCell(originLoc[0], originLoc[1]);
		blinkDistance = boltLevel * 2 + 1;
	}
	
	for (i=0; i<initialBoltLength; i++) {
		boltLightColors[i] = *boltColor;
		boltLights[i] = lightCatalog[BOLT_LIGHT_SOURCE];
		boltLights[i].lightColor = &boltLightColors[i];
		boltLights[i].lightRadius.lowerBound = boltLights[i].lightRadius.upperBound = 100 * (3 + boltLevel) * (initialBoltLength - i) / initialBoltLength;
	}
	
	if (bolt == BOLT_TUNNELING) {
		tunnelize(originLoc[0], originLoc[1]);
	}
	
	for (i=0; i<numCells; i++) {
		
		x = listOfCoordinates[i][0];
		y = listOfCoordinates[i][1];
		
		monst = monsterAtLoc(x, y);
		
		boltInView = false;
		
		if (bolt == BOLT_BLINKING && shootingMonst == &player) {
			player.xLoc = x;
			player.yLoc = y;
			updateVision();
		}
		
		demoteVisibility();
		updateLighting();
		for (k = min(i, boltLength + 2); k >= 0; k--) {
			if (k < initialBoltLength) {// && (!fastForward || k == 0)) {
				boltLights[k].xLoc = listOfCoordinates[i-k][0];
				boltLights[k].yLoc = listOfCoordinates[i-k][1];
				paintLight(&boltLights[k]);
			}
		}
		updateFieldOfViewDisplay(false);
		
		for (k = min(i, boltLength + 2); k >= 0; k--) {
			if (pmap[listOfCoordinates[i-k][0]][listOfCoordinates[i-k][1]].flags & (IN_FIELD_OF_VIEW | CLAIRVOYANT_VISIBLE)) {
				hiliteCell(listOfCoordinates[i-k][0], listOfCoordinates[i-k][1], boltColor, max(0, 100 - k * 100 / (boltLength)));
				if (pmap[listOfCoordinates[i-k][0]][listOfCoordinates[i-k][1]].flags & (IN_FIELD_OF_VIEW | CLAIRVOYANT_VISIBLE)) {
					boltInView = true;
				}
			}
		}
		
		if (!fastForward && boltInView) {
			fastForward = pauseBrogue(10);
		}
		
		if (monst && projectileReflects(shootingMonst, monst) && i < DCOLS*2) {
			if (projectileReflects(shootingMonst, monst)) { // if it scores another reflection roll, reflect at caster
				numCells = reflectBolt(originLoc[0], originLoc[1], listOfCoordinates, i, !alreadyReflected);
			} else {
				numCells = reflectBolt(-1, -1, listOfCoordinates, i, false); // otherwise reflect randomly
			}
			
			alreadyReflected = true;
			
			monsterName(monstName, monst, true);
			sprintf(buf, "%s deflect%s the bolt", monstName, (monst == &player ? "" : "s"));
			combatMessage(buf);
			continue;
		}
		
		if (!monst && bolt == BOLT_FIRE) {
			if (exposeTileToFire(x, y, true)) {
				updateVision();
				autoID = true;
			}
		}
		
		if (bolt == BOLT_BLINKING) {
			boltLevel = (blinkDistance - i) / 2 + 1;
			boltLength = boltLevel * 5;
			for (j=0; j<i; j++) {
				refreshDungeonCell(listOfCoordinates[j][0], listOfCoordinates[j][1]);
			}
			if (i >= blinkDistance) {
				break;
			}
		}
		
		// does the bolt bounce off the wall?
		if (i + 1 < numCells
			&& cellHasTerrainFlag(listOfCoordinates[i+1][0], listOfCoordinates[i+1][1],
								  (OBSTRUCTS_VISION | OBSTRUCTS_PASSABILITY))
			&& projectileReflects(shootingMonst, NULL)
			&& i < DCOLS*2) {
			
			sprintf(buf, "the bolt reflects off of %s", tileText(listOfCoordinates[i+1][0], listOfCoordinates[i+1][1]));
			
			if (projectileReflects(shootingMonst, NULL)) { // if it scores another reflection roll, reflect at caster
				numCells = reflectBolt(originLoc[0], originLoc[1], listOfCoordinates, i, !alreadyReflected);
			} else {
				numCells = reflectBolt(-1, -1, listOfCoordinates, i, false); // otherwise reflect randomly
			}
			
			alreadyReflected = true;
			
			combatMessage(buf);
			continue;
		}
		
		// some bolts halt the square before they hit something
		if ((bolt == BOLT_BLINKING || bolt == BOLT_OBSTRUCTION)
			&& i + 1 < numCells
			&& (cellHasTerrainFlag(listOfCoordinates[i+1][0], listOfCoordinates[i+1][1],
								   (OBSTRUCTS_VISION | OBSTRUCTS_PASSABILITY))
				|| (pmap[listOfCoordinates[i+1][0]][listOfCoordinates[i+1][1]].flags & (HAS_PLAYER | HAS_MONSTER)))) {
				if (pmap[listOfCoordinates[i+1][0]][listOfCoordinates[i+1][1]].flags & HAS_MONSTER) {
					monst = monsterAtLoc(listOfCoordinates[i+1][0], listOfCoordinates[i+1][1]);
					if (!(monst->bookkeepingFlags & MONST_SUBMERGED)) {
						break;
					}
				} else {
					break;
				}
			}
		
		// lightning hits monsters
		if (monst && (pmap[x][y].flags & (HAS_PLAYER | HAS_MONSTER)) && (bolt == BOLT_LIGHTNING) && (!monst || !(monst->bookkeepingFlags & MONST_SUBMERGED))) {
			monsterName(monstName, monst, true);
			
			autoID = true;
			
			if (monst->status.magicalFear) {
				monst->status.magicalFear = 1;
			}
			monst->status.entranced = 0;
			
			if (inflictDamage(shootingMonst, monst, staffDamage(boltLevel), &lightningColor)) {
				// killed monster
				if (player.currentHP <= 0) {
					if (shootingMonst == &player) {
						gameOver("killed by a reflected lightning bolt", true);
					}
					return false;
				}
				sprintf(buf, "the lightning bolt kills %s", monstName);
				combatMessage(buf);
			} else {
				// monster lives
				if (monst->creatureMode != MODE_PERM_FLEEING
					&& monst->creatureState != MONSTER_ALLY
					&& (monst->creatureState != MONSTER_FLEEING || monst->status.magicalFear)) {
					monst->creatureState = MONSTER_TRACKING_SCENT;
					monst->status.magicalFear = 0;
				}
				sprintf(buf, "the lightning bolt hits %s", monstName);
				combatMessage(buf);
				if (monst->info.abilityFlags & MA_CLONE_SELF_ON_DEFEND) {
					fadeInMonster(cloneMonster(monst));
					if (canSeeMonster(monst)) {
						sprintf(buf, "%s splits in two!", monstName);
						message(buf, true, false);
					}
				}
			}
		}
		
		if (cellHasTerrainFlag(x, y, (OBSTRUCTS_PASSABILITY | OBSTRUCTS_VISION))
			|| ((pmap[x][y].flags & HAS_PLAYER || (pmap[x][y].flags & (HAS_MONSTER) && monst && !(monst->bookkeepingFlags & MONST_SUBMERGED)))
				&& bolt != BOLT_LIGHTNING)) {
				
				if (bolt == BOLT_TUNNELING && x > 0 && y > 0 && x < DCOLS - 1 && y < DROWS - 1) {
					tunnelize(x, y);
					if (i > 0 && x != listOfCoordinates[i-1][0] && y != listOfCoordinates[i-1][1]) {
						if (rand_percent(50)) {
							tunnelize(listOfCoordinates[i-1][0], y);
						} else {
							tunnelize(x, listOfCoordinates[i-1][1]);
						}
					} else if (i == 0 && x > 0 && y > 0 && x < DCOLS - 1 && y < DROWS - 1) {
						if (rand_percent(50)) {
							tunnelize(originLoc[0], y);
						} else {
							tunnelize(x, originLoc[1]);
						}
					}
					updateVision();
					autoID = true;
					boltLength = --boltLevel * 5;
					for (j=0; j<i; j++) {
						refreshDungeonCell(listOfCoordinates[j][0], listOfCoordinates[j][1]);
					}
					if (!boltLevel) {
						refreshDungeonCell(listOfCoordinates[i-1][0], listOfCoordinates[i-1][1]);
						refreshDungeonCell(x, y);
						break;
					}
				} else {
					break;
				}
			}
	}
	
	if (bolt == BOLT_BLINKING) {
		if (pmap[x][y].flags & HAS_MONSTER) { // submerged monster
			monst = monsterAtLoc(x, y);
			dir = randValidDirectionFrom(monst, x, y, true);
			moveMonster(monst, nbDirs[dir][0], nbDirs[dir][1]);
		}
		pmap[x][y].flags |= (shootingMonst == &player ? HAS_PLAYER : HAS_MONSTER);
		shootingMonst->xLoc = x;
		shootingMonst->yLoc = y;
		applyInstantTileEffectsToCreature(shootingMonst);
		
		if (shootingMonst == &player) {
			updateVision();
		}
		autoID = true;
	} else if (bolt == BOLT_BECKONING) {
		if (monst && !(monst->info.flags & MONST_INANIMATE)
			&& distanceBetween(originLoc[0], originLoc[1], monst->xLoc, monst->yLoc) > 1) {
			beckonedMonster = true;
			fastForward = true;
		}
	}
	
	if (pmap[x][y].flags & (HAS_MONSTER | HAS_PLAYER)) {
		monst = monsterAtLoc(x, y);
		monsterName(monstName, monst, true);
	} else {
		monst = NULL;
	}
	
	switch(bolt) {
		case BOLT_TELEPORT:
			if (monst && !(monst->info.flags & MONST_INANIMATE)) {
				if (monst->bookkeepingFlags & MONST_CAPTIVE) {
					freeCaptive(monst);
				}
				teleport(monst);
			}
			break;
		case BOLT_BECKONING:
			if (beckonedMonster && monst) {
				if (canSeeMonster(monst)) {
					autoID = true;
				}
				if (monst->bookkeepingFlags & MONST_CAPTIVE) {
					freeCaptive(monst);
				}
				newLoc[0] = monst->xLoc;
				newLoc[1] = monst->yLoc;
				zap(newLoc, originLoc, BOLT_BLINKING, max(1, (distanceBetween(originLoc[0], originLoc[1], newLoc[0], newLoc[1]) - 2) / 2), false);
				if (monst->ticksUntilTurn < player.attackSpeed+1) {
					monst->ticksUntilTurn = player.attackSpeed+1;
				}
				if (canSeeMonster(monst)) {
					autoID = true;
				}
			}
			break;
		case BOLT_SLOW:
			if (monst) {
				slow(monst, boltLevel);
				flashMonster(monst, boltColors[BOLT_SLOW], 100);
				autoID = true;
			}
			break;
		case BOLT_HASTE:
			if (monst) {
				haste(monst, staffHasteDuration(boltLevel));
				flashMonster(monst, boltColors[BOLT_HASTE], 100);
				autoID = true;
			}
			break;
		case BOLT_POLYMORPH:
			if (monst && monst != &player && !(monst->info.flags & MONST_INANIMATE)) {
				unAlly(monst); // not your ally anymore!
				healthFraction = monst->currentHP / monst->info.maxHP;
				previousExperience = monst->info.expForKilling;
				do {
					monst->info = monsterCatalog[rand_range(1, NUMBER_MONSTER_KINDS - 1)]; // abra kadabra
				} while (monst->info.flags & MONST_INANIMATE);
				monst->currentHP = max(1, healthFraction * monst->info.maxHP);
				monst->info.expForKilling = healthFraction * monst->info.expForKilling + (1 - healthFraction) * previousExperience;
				
				monst->movementSpeed = monst->info.movementSpeed;
				monst->attackSpeed = monst->info.attackSpeed;
				if (monst->status.hasted) {
					monst->movementSpeed /= 2;
					monst->attackSpeed /= 2;
				}
				if (monst->status.slowed) {
					monst->movementSpeed *= 2;
					monst->attackSpeed *= 2;
				}
				
				if (monst->intrinsicLight) {
					deleteLight(monst->intrinsicLight);
				}
				
				clearStatus(monst);
				
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
				
				if (monst->bookkeepingFlags & MONST_CAPTIVE) {
					demoteMonsterFromLeadership(monst);
					monst->creatureState = MONSTER_TRACKING_SCENT;
					monst->bookkeepingFlags &= ~MONST_CAPTIVE;
				}
				
				refreshDungeonCell(monst->xLoc, monst->yLoc);
				flashMonster(monst, boltColors[BOLT_POLYMORPH], 100);
				if (!(monst->info.flags & MONST_INVISIBLE)) {
					autoID = true;
				}
			}
			break;
		case BOLT_INVISIBILITY:
			if (monst && monst != &player && !(monst->info.flags & MONST_INANIMATE)) {
				monst->info.flags |= MONST_INVISIBLE;
				refreshDungeonCell(monst->xLoc, monst->yLoc);
				if (!hideDetails) {
					flashMonster(monst, boltColors[BOLT_INVISIBILITY], 100);	
				}
			}
			break;
		case BOLT_DOMINATION:
			if (monst && monst != &player && !(monst->info.flags & MONST_INANIMATE)) {
				if (monst->currentHP * 5 < monst->info.maxHP
					|| rand_range(0, monst->info.maxHP) > monst->currentHP) {
					// domination succeeded
					freeCaptive(monst);
					refreshDungeonCell(monst->xLoc, monst->yLoc);
					if (canSeeMonster(monst)) {
						autoID = true;
						sprintf(buf, "%s is bound to your will!", monstName);
						message(buf, true, false);
						flashMonster(monst, boltColors[BOLT_DOMINATION], 100);
					}
				} else if (canSeeMonster(monst)) {
					sprintf(buf, "%s resists the bolt of domination.", monstName);
					message(buf, true, false);
				}
			}
			break;
		case BOLT_CANCELLATION:
			if (monst) { // works on inanimates
				cancelMagic(monst);
				flashMonster(monst, boltColors[BOLT_CANCELLATION], 100);
			}
			break;
		case BOLT_LIGHTNING:
			// already handled above
			break;
		case BOLT_POISON:
			if (monst && !(monst->info.flags & MONST_INANIMATE)) {
				poisonDamage = staffPoison(boltLevel);
				monst->status.poisoned = monst->maxStatus.poisoned = max(poisonDamage, monst->status.poisoned);
				if (canSeeMonster(monst)) {
					flashMonster(monst, boltColors[BOLT_POISON], 100);
					autoID = true;
					if (monst != &player) {
						sprintf(buf, "%s %s very sick", monstName, (monst == &player ? "feel" : "looks"));
						combatMessage(buf);
					}
				}
			}
			break;
		case BOLT_FIRE:
			if (monst) {
				autoID = true;
				
				if (monst->status.magicalFear) {
					monst->status.magicalFear = 1;
				}
				monst->status.entranced = 0;
				
				if (monst->info.flags & MONST_IMMUNE_TO_FIRE) {
					sprintf(buf, "%s ignore%s the firebolt", monstName, (monst == &player ? "" : "s"));
					combatMessage(buf);
				} else if (inflictDamage(shootingMonst, monst, staffDamage(boltLevel), &orange)) {
					// killed creature
					
					if (player.currentHP <= 0) {
						if (shootingMonst == &player) {
							gameOver("killed by a reflected firebolt", true);
						}
						return false;
					}
					
					sprintf(buf, "the firebolt kills %s", monstName);
					combatMessage(buf);
				} else {
					// monster lives
					if (monst->creatureMode != MODE_PERM_FLEEING
						&& monst->creatureState != MONSTER_ALLY
						&& (monst->creatureState != MONSTER_FLEEING || monst->status.magicalFear)) {
						monst->creatureState = MONSTER_TRACKING_SCENT;
					}
					sprintf(buf, "the firebolt hits %s", monstName);
					combatMessage(buf);
					exposeCreatureToFire(monst);
					if (monst->info.abilityFlags & MA_CLONE_SELF_ON_DEFEND) {
						fadeInMonster(cloneMonster(monst));
						if (canSeeMonster(monst)) {
							sprintf(buf, "%s splits in two!", monstName);
							message(buf, true, false);
						}
					}
				}
			}
			exposeTileToFire(x, y, true); // burninate
			break;
		case BOLT_BLINKING:
			if (shootingMonst == &player) {
				// handled above for visual effect (i.e. before contrail fades)
				// increase scent turn number so monsters don't sniff around at the old cell like idiots
				rogue.scentTurnNumber += 30;
				// get any items at the destination location
				if (pmap[player.xLoc][player.yLoc].flags & HAS_ITEM) {
					pickUpItemAt(player.xLoc, player.yLoc);
				}
			}
			break;
		case BOLT_ENTRANCEMENT:
			if (monst && monst == &player) {
				flashMonster(monst, &confusionGasColor, 100);
				monst->status.confused = boltLevel * 3;
				monst->maxStatus.confused = max(monst->status.confused, monst->maxStatus.confused);
				message("the bolt hits you and you suddently feel disoriented.", true, true);
				autoID = true;
			} else if (monst && !(monst->info.flags & MONST_INANIMATE)) {
				monst->status.entranced = monst->maxStatus.entranced = max(24 * (40 * (boltLevel - 3) / 7 + 20) / monst->info.maxHP, boltLevel);
				wakeUp(monst);
				if (canSeeMonster(monst)) {
					flashMonster(monst, boltColors[BOLT_ENTRANCEMENT], 100);
					autoID = true;
					sprintf(buf, "%s is entranced!", monstName);
					message(buf, true, false);
				}
			}
			break;
		case BOLT_HEALING:
			if (monst) {
				heal(monst, boltLevel * 10);
				if (canSeeMonster(monst)) {
					autoID = true;
				}
			}
			break;
		case BOLT_OBSTRUCTION:
			feat = dungeonFeatureCatalog[DF_FORCEFIELD];
			feat.probabilityDecrement.lowerBound = feat.probabilityDecrement.upperBound = max(1, 75 * pow(0.8, boltLevel));
			spawnDungeonFeature(x, y, &feat, true);
			autoID = true;
			break;
		case BOLT_TUNNELING:
			if (autoID) {
				setUpWaypoints(); // recompute based on the new situation
			}
			break;
		case BOLT_PLENTY:
			if (monst && !(monst->info.flags & MONST_INANIMATE)) {
				newMonst = cloneMonster(monst);
				if (newMonst) {
					flashMonster(monst, boltColors[BOLT_PLENTY], 100);
					flashMonster(newMonst, boltColors[BOLT_PLENTY], 100);
					autoID = true;
				}
			}
			break;
		case BOLT_DISCORD:
			if (monst && !(monst->info.flags & MONST_INANIMATE)) {
				monst->status.discordant = monst->maxStatus.discordant = max(staffDiscordDuration(boltLevel), monst->status.discordant);
				if (canSeeMonster(monst)) {
					flashMonster(monst, boltColors[BOLT_DISCORD], 100);
					autoID = true;
				}
			}
			break;
		case BOLT_CONJURATION:
			
			for (j = 0; j < (staffBladeCount(boltLevel)); j++) {
				getQualifyingLocNear(newLoc, x, y, DCOLS,
									 PATHING_BLOCKER & ~(LAVA_INSTA_DEATH | IS_DEEP_WATER | TRAP_DESCENT),
									 (HAS_PLAYER | HAS_MONSTER), false);
				monst = generateMonster(MK_SPECTRAL_BLADE, true);
				monst->xLoc = newLoc[0];
				monst->yLoc = newLoc[1];
				monst->bookkeepingFlags |= (MONST_FOLLOWER | MONST_BOUND_TO_LEADER | MONST_DOES_NOT_TRACK_LEADER);
				monst->bookkeepingFlags &= ~MONST_JUST_SUMMONED;
				monst->leader = &player;
				monst->creatureState = MONSTER_ALLY;
				monst->ticksUntilTurn = monst->info.attackSpeed;
				pmap[monst->xLoc][monst->yLoc].flags |= HAS_MONSTER;
				refreshDungeonCell(monst->xLoc, monst->yLoc);
				//fadeInMonster(monst);
			}
			monst = NULL;
			autoID = true;
			break;
		default:
			break;
	}
	
	if (boltLength > 0) {
		// j is where the front tip of the bolt would be if it hadn't collided at i
		for (j=i; j < i + boltLength + 2; j++) { // note that j can imply a bolt position off the map
			
			boltInView = false;
			
			// dynamic lighting
			//if (!fastForward) {
			demoteVisibility();
			updateLighting();
			for (k = min(j, boltLength + 2); k >= j-i; k--) {
				if (k < initialBoltLength) {
					boltLights[k].xLoc = listOfCoordinates[j-k][0];
					boltLights[k].yLoc = listOfCoordinates[j-k][1];
					paintLight(&boltLights[k]);
				}
			}
			updateFieldOfViewDisplay(false);
			//}
			
			// beam graphic
			// k iterates from the rear tip of the visible portion of the bolt to the front
			for (k = min(j, boltLength + 2); k >= j-i; k--) {
				if (pmap[listOfCoordinates[j-k][0]][listOfCoordinates[j-k][1]].flags & (IN_FIELD_OF_VIEW | CLAIRVOYANT_VISIBLE)) {
					hiliteCell(listOfCoordinates[j-k][0], listOfCoordinates[j-k][1], boltColor, max(0, 100 - k * 100 / (boltLength)));
					if (pmap[listOfCoordinates[j-k][0]][listOfCoordinates[j-k][1]].flags & (IN_FIELD_OF_VIEW | CLAIRVOYANT_VISIBLE)) {
						boltInView = true;
					}
				}
			}
			
			
			if (!fastForward && boltInView) {
				fastForward = pauseBrogue(10);
			}
		}
	}
	
	return autoID;
}

creature *nextTargetAfter(short targetX, short targetY, boolean targetAllies, boolean requireOpenPath) {
	creature *currentTarget, *monst, *returnMonst = NULL;
	short currentDistance, shortestDistance;
	
	currentTarget = monsterAtLoc(targetX, targetY);
	
	if (!currentTarget || currentTarget == &player) {
		currentTarget = monsters;
		currentDistance = 0;
	} else {
		currentDistance = distanceBetween(player.xLoc, player.yLoc, targetX, targetY);
	}
	
	// first try to find a monster with the same distance later in the chain.
	for (monst = currentTarget->nextCreature; monst != NULL; monst = monst->nextCreature) {
		if (distanceBetween(player.xLoc, player.yLoc, monst->xLoc, monst->yLoc) == currentDistance
			&& canSeeMonster(monst)
			&& (targetAllies == (monst->creatureState == MONSTER_ALLY))
			&& (!requireOpenPath || openPathBetween(player.xLoc, player.yLoc, monst->xLoc, monst->yLoc))) {
			
			// got one!
			returnMonst = monst;
			break;
		}
	}
	
	if (!returnMonst) {
		// okay, instead pick the qualifying monster (excluding the current target)
		// with the shortest distance greater than currentDistance.
		shortestDistance = max(DCOLS, DROWS);
		for (monst = currentTarget->nextCreature;; monst = monst->nextCreature) {
			if (monst == NULL) {
				monst = monsters;
			} else if (distanceBetween(player.xLoc, player.yLoc, monst->xLoc, monst->yLoc) < shortestDistance
					   && distanceBetween(player.xLoc, player.yLoc, monst->xLoc, monst->yLoc) > currentDistance
					   && canSeeMonster(monst)
					   && (targetAllies == (monst->creatureState == MONSTER_ALLY))
					   && (!requireOpenPath || openPathBetween(player.xLoc, player.yLoc, monst->xLoc, monst->yLoc))) {
				// potentially this one
				shortestDistance = distanceBetween(player.xLoc, player.yLoc, monst->xLoc, monst->yLoc);
				returnMonst = monst;
			}
			if (monst == currentTarget) {
				break;
			}
		}
	}
	
	if (!returnMonst) {
		// okay, instead pick the qualifying monster (excluding the current target)
		// with the shortest distance period.
		shortestDistance = max(DCOLS, DROWS);
		for (monst = monsters->nextCreature; monst != NULL; monst = monst->nextCreature) {
			if (distanceBetween(player.xLoc, player.yLoc, monst->xLoc, monst->yLoc) < shortestDistance
				&& canSeeMonster(monst)
				&& (targetAllies == (monst->creatureState == MONSTER_ALLY))
				&& (!requireOpenPath || openPathBetween(player.xLoc, player.yLoc, monst->xLoc, monst->yLoc))) {
				// potentially this one
				shortestDistance = distanceBetween(player.xLoc, player.yLoc, monst->xLoc, monst->yLoc);
				returnMonst = monst;
			}
		}
	}
	
	if (returnMonst) {
		plotCharWithColor(returnMonst->info.displayChar, returnMonst->xLoc + STAT_BAR_WIDTH,
						  returnMonst->yLoc + MESSAGE_LINES, *(returnMonst->info.foreColor), white);
	}
	
	return returnMonst;
}

void hiliteTrajectory(short coordinateList[DCOLS][2], short numCells, boolean eraseHiliting, boolean passThroughMonsters) {
	short x, y, i;
	creature *monst;
	for (i=0; i<numCells; i++) {
		x = coordinateList[i][0];
		y = coordinateList[i][1];
		hiliteCell(x, y, &hiliteColor, eraseHiliting ? 0 : 50);
		
		if (cellHasTerrainFlag(x, y, (OBSTRUCTS_VISION | OBSTRUCTS_PASSABILITY))
			|| pmap[x][y].flags & (HAS_PLAYER)) {
			break;
		} else if (!(pmap[x][y].flags & DISCOVERED)) {
			break;
		} else if (!passThroughMonsters && pmap[x][y].flags & (HAS_MONSTER)
				   && (playerCanSee(x, y) || player.status.telepathic)) {
			monst = monsterAtLoc(x, y);
			if (!(monst->bookkeepingFlags & MONST_SUBMERGED)
				&& (!(monst->info.flags & MONST_INVISIBLE) || player.status.telepathic)) {
				break;
			}
		}
		
	}
}

void moveCursor(boolean *targetConfirmed, boolean *canceled, boolean *tabKey, short targetLoc[2]) {
	uchar keystroke;
	short moveIncrement;
	boolean acceptableInput;
	rogueEvent theEvent;
	
	*targetConfirmed = *canceled = *tabKey = false;
	
	do {
		acceptableInput = false;
		nextBrogueEvent(&theEvent, false, false);
		if ((theEvent.eventType == MOUSE_UP || (theEvent.eventType == MOUSE_ENTERED_CELL
												&& (theEvent.param1 != brogueCursorX || theEvent.param2 != brogueCursorY)))
			&& coordinatesAreInMap(theEvent.param1 - STAT_BAR_WIDTH, theEvent.param2 - MESSAGE_LINES)) {
			
			brogueCursorX = theEvent.param1;
			brogueCursorY = theEvent.param2;
			
			if (theEvent.eventType == MOUSE_UP
				&& (theEvent.controlKey
					|| (targetLoc[0] == theEvent.param1 - STAT_BAR_WIDTH
						&& targetLoc[1] == theEvent.param2 - MESSAGE_LINES))) {
						*targetConfirmed = true;
					}
			targetLoc[0] = theEvent.param1 - STAT_BAR_WIDTH;
			targetLoc[1] = theEvent.param2 - MESSAGE_LINES;
			acceptableInput = true;
			
		} else if (theEvent.eventType == KEYSTROKE) {
			keystroke = theEvent.param1;
			moveIncrement = ( (theEvent.controlKey || theEvent.shiftKey) ? 5 : 1 );
			stripShiftFromMovementKeystroke(&keystroke);
			switch(keystroke) {
				case LEFT_ARROW:
				case LEFT_KEY:
				case NUMPAD_4:
					if (targetLoc[0] > 0) {
						targetLoc[0] = max(0, targetLoc[0] - moveIncrement);
						acceptableInput = true;
					}
					break;
				case RIGHT_ARROW:
				case RIGHT_KEY:
				case NUMPAD_6:
					if (targetLoc[0] < DCOLS - 1) {
						targetLoc[0] = min(DCOLS - 1, targetLoc[0] + moveIncrement);
						acceptableInput = true;
					}
					break;
				case UP_ARROW:
				case UP_KEY:
				case NUMPAD_8:
					if (targetLoc[1] > 0) {
						targetLoc[1] = max(0, targetLoc[1] - moveIncrement);
						acceptableInput = true;
					}
					break;
				case DOWN_ARROW:
				case DOWN_KEY:
				case NUMPAD_2:
					if (targetLoc[1] < DROWS - 1) {
						targetLoc[1] = min(DROWS - 1, targetLoc[1] + moveIncrement);
						acceptableInput = true;
					}
					break;
				case UPLEFT_KEY:
				case NUMPAD_7:
					if (targetLoc[0] > 0 && targetLoc[1] > 0) {
						targetLoc[0] = max(0, targetLoc[0] - moveIncrement);
						targetLoc[1] = max(0, targetLoc[1] - moveIncrement);
						acceptableInput = true;
					}
					break;
				case UPRIGHT_KEY:
				case NUMPAD_9:
					if (targetLoc[0] < DCOLS - 1 && targetLoc[1] > 0) {
						targetLoc[0] = min(DCOLS - 1, targetLoc[0] + moveIncrement);
						targetLoc[1] = max(0, targetLoc[1] - moveIncrement);
						acceptableInput = true;
					}
					break;
				case DOWNLEFT_KEY:
				case NUMPAD_1:
					if (targetLoc[0] > 0 && targetLoc[1] < DROWS - 1) {
						targetLoc[0] = max(0, targetLoc[0] - moveIncrement);
						targetLoc[1] = min(DROWS - 1, targetLoc[1] + moveIncrement);
						acceptableInput = true;
					}
					break;
				case DOWNRIGHT_KEY:
				case NUMPAD_3:
					if (targetLoc[0] < DCOLS - 1 && targetLoc[1] < DROWS - 1) {
						targetLoc[0] = min(DCOLS - 1, targetLoc[0] + moveIncrement);
						targetLoc[1] = min(DROWS - 1, targetLoc[1] + moveIncrement);
						acceptableInput = true;
					}
					break;
				case TAB_KEY:
				case NUMPAD_0:
					*tabKey = true;
					acceptableInput = true;
					break;
				case RETURN_KEY:
				case ENTER_KEY:
					*targetConfirmed = true;
					acceptableInput = true;
					break;
				case ESCAPE_KEY:
				case ACKNOWLEDGE_KEY:
					*canceled = true;
					acceptableInput = true;
					break;
				default:
					break;
			}
		}
	} while (!acceptableInput);
}

boolean chooseTarget(short returnLoc[2], short maxDistance, boolean stopAtTarget, boolean autoTarget, boolean targetAllies, boolean passThroughCreatures) {
	short originLoc[2], targetLoc[2], oldTargetLoc[2], coordinates[DCOLS][2], numCells;
	creature *monst;
	boolean canceled, targetConfirmed, tabKey, focusedOnMonster = false;
	
	originLoc[0] = player.xLoc;
	originLoc[1] = player.yLoc;
	
	targetLoc[0] = player.xLoc;
	targetLoc[1] = player.yLoc;
	
	if (autoTarget) {
		monst = nextTargetAfter(targetLoc[0], targetLoc[1], targetAllies, true);
		if (monst) {
			targetLoc[0] = monst->xLoc;
			targetLoc[1] = monst->yLoc;
			refreshSideBar(monst);
			focusedOnMonster = true;
		}
	}
	
	numCells = getLineCoordinates(coordinates, originLoc, targetLoc);
	if (maxDistance > 0) {
		numCells = min(numCells, maxDistance);
	}
	if (stopAtTarget) {
		numCells = min(numCells, distanceBetween(player.xLoc, player.yLoc, targetLoc[0], targetLoc[1]));
	}
	
	hiliteTrajectory(coordinates, numCells, false, passThroughCreatures);
	hiliteCell(targetLoc[0], targetLoc[1], &white, 75);
	
	do {
		printLocationDescription(targetLoc[0], targetLoc[1]);
		
		oldTargetLoc[0] = targetLoc[0];
		oldTargetLoc[1] = targetLoc[1];
		moveCursor(&targetConfirmed, &canceled, &tabKey, targetLoc);
		
		if (canceled) {
			refreshDungeonCell(oldTargetLoc[0], oldTargetLoc[1]);
			hiliteTrajectory(coordinates, numCells, true, passThroughCreatures);
			confirmMessages();
			return false;
		}
		
		if (tabKey) {
			monst = nextTargetAfter(targetLoc[0], targetLoc[1], targetAllies, true);
			if (monst) {
				targetLoc[0] = monst->xLoc;
				targetLoc[1] = monst->yLoc;
			}
		}
		
		monst = monsterAtLoc(targetLoc[0], targetLoc[1]);
		if (monst != NULL && monst != &player && canSeeMonster(monst)) {
			refreshSideBar(monst);
			focusedOnMonster = true;
		} else if (focusedOnMonster) {
			refreshSideBar(NULL);
			focusedOnMonster = false;
		}
		
		refreshDungeonCell(oldTargetLoc[0], oldTargetLoc[1]);
		hiliteTrajectory(coordinates, numCells, true, passThroughCreatures);
		
		if (!targetConfirmed) {
			numCells = getLineCoordinates(coordinates, originLoc, targetLoc);
			if (maxDistance > 0) {
				numCells = min(numCells, maxDistance);
			}
			
			if (stopAtTarget) {
				numCells = min(numCells, distanceBetween(player.xLoc, player.yLoc, targetLoc[0], targetLoc[1]));
			}
			hiliteTrajectory(coordinates, numCells, false, passThroughCreatures);
			hiliteCell(targetLoc[0], targetLoc[1], &white, 75);
		}
		
	} while (!targetConfirmed);
	if (maxDistance > 0) {
		numCells = min(numCells, maxDistance);
	}
	hiliteTrajectory(coordinates, numCells, true, passThroughCreatures);
	
	if (originLoc[0] == targetLoc[0] && originLoc[1] == targetLoc[1]) {
		confirmMessages();
		return false;
	}
	
	returnLoc[0] = targetLoc[0];
	returnLoc[1] = targetLoc[1];
	return true;
}

// returns whether the item disappeared
boolean hitMonsterWithProjectileWeapon(creature *thrower, creature *monst, item *theItem) {
	char buf[DCOLS], theItemName[DCOLS], targetName[DCOLS], armorRunicString[DCOLS];
	boolean thrownWeaponHit;
	item *equippedWeapon;
	short damage;
	
	if (!(theItem->category & WEAPON)) {
		return false;
	}
	
	armorRunicString[0] = '\0';
	
	itemName(theItem, theItemName, false, false);
	monsterName(targetName, monst, true);
	
	monst->status.entranced = 0;
	
	if (monst != &player
		&& monst->creatureMode != MODE_PERM_FLEEING
		&& (monst->creatureState != MONSTER_FLEEING || monst->status.magicalFear)
		&& !(monst->bookkeepingFlags & MONST_CAPTIVE)) {
		monst->creatureState = MONSTER_TRACKING_SCENT;
		if (monst->status.magicalFear) {
			monst->status.magicalFear = 1;
		}
	}
	
	if (thrower == &player) {
		equippedWeapon = rogue.weapon;
		equipItem(theItem, true);
		thrownWeaponHit = attackHit(&player, monst);
		if (equippedWeapon) {
			equipItem(equippedWeapon, true);
		} else {
			unequipItem(theItem, true);
		}
	} else {
		thrownWeaponHit = attackHit(thrower, monst);
	}
	
	if (thrownWeaponHit) {
		damage = (monst->info.flags & MONST_IMMUNE_TO_WEAPONS ? 0 :
				  randClump(theItem->damage)) * pow(1.065, netEnchant(theItem));
		
		if (monst == &player) {
			applyArmorRunicEffect(armorRunicString, thrower, &damage, false);
		}
		
		if (inflictDamage(thrower, monst, damage, &red)) { // monster killed
			sprintf(buf, "the %s killed %s.", theItemName, targetName);
			message(buf, true, false);
		} else {
			sprintf(buf, "the %s hit %s.", theItemName, targetName);
			if (theItem->flags & ITEM_RUNIC) {
				magicWeaponHit(monst, theItem, false);
			}
			message(buf, true, false);
			if (monst->info.abilityFlags & MA_CLONE_SELF_ON_DEFEND) {
				fadeInMonster(cloneMonster(monst));
				if (canSeeMonster(monst)) {
					sprintf(targetName, "%s splits in two!", targetName);
					message(targetName, true, false);
				}
			}
		}
		if (armorRunicString[0]) {
			message(armorRunicString, true, false);
		}
		return true;
	} else {
		sprintf(buf, "the %s missed %s.", theItemName, targetName);
		message(buf, true, false);
		return false;
	}
}

void throwItem(item *theItem, creature *thrower, short targetLoc[2], short maxDistance) {
	short listOfCoordinates[MAX_BOLT_LENGTH][2], originLoc[2];
	short i, x, y, numCells;
	creature *monst = NULL;
	char buf[COLS*3], buf2[COLS], buf3[COLS];
	uchar displayChar;
	color foreColor, backColor, multColor;
	short dropLoc[2];
	boolean hitSomethingSolid = false, fastForward = false;
	
	x = originLoc[0] = thrower->xLoc;
	y = originLoc[1] = thrower->yLoc;
	
	numCells = getLineCoordinates(listOfCoordinates, originLoc, targetLoc);
	
	thrower->ticksUntilTurn = thrower->attackSpeed;
	
	if (thrower != &player && pmap[originLoc[0]][originLoc[1]].flags & IN_FIELD_OF_VIEW) {
		monsterName(buf2, thrower, true);
		itemName(theItem, buf3, false, true);
		sprintf(buf, "%s hurls %s.", buf2, buf3);
		message(buf, true, false);
	}
	
	for (i=0; i<numCells && i < maxDistance; i++) {
		
		x = listOfCoordinates[i][0];
		y = listOfCoordinates[i][1];
		
		if (pmap[x][y].flags & (HAS_MONSTER | HAS_PLAYER)) {
			monst = monsterAtLoc(x, y);
			
			if (projectileReflects(thrower, monst) && i < DCOLS*2) {
				if (projectileReflects(thrower, monst)) { // if it scores another reflection roll, reflect at caster
					numCells = reflectBolt(originLoc[0], originLoc[1], listOfCoordinates, i, true);
				} else {
					numCells = reflectBolt(-1, -1, listOfCoordinates, i, false); // otherwise reflect randomly
				}
				
				monsterName(buf2, monst, true);
				itemName(theItem, buf3, false, false);
				sprintf(buf, "%s deflect%s the %s", buf2, (monst == &player ? "" : "s"), buf3);
				combatMessage(buf);
				continue;
			}
			
			if ((theItem->category & WEAPON)
				&& theItem->kind != INCENDIARY_DART
				&& hitMonsterWithProjectileWeapon(thrower, monst, theItem)) {
				return;
			}
			
			break;
		}
		
		if (cellHasTerrainFlag(x, y, (OBSTRUCTS_PASSABILITY | OBSTRUCTS_VISION))) {
			i--;
			if (i >= 0) {
				x = listOfCoordinates[i][0];
				y = listOfCoordinates[i][1];
			} else { // it was aimed point-blank into an obstruction
				x = thrower->xLoc;
				y = thrower->yLoc;
			}
			hitSomethingSolid = true;
			break;
		}
		
		if (playerCanSee(x, y)) { // show the graphic
			getCellAppearance(x, y, &displayChar, &foreColor, &backColor);
			foreColor = *(theItem->foreColor);
			if (pmap[x][y].flags & (VISIBLE)) {
				colorMultiplierFromDungeonLight(x, y, &multColor);
				applyColorMultiplier(&foreColor, &multColor);
			} else { // clairvoyant visible
				applyColorMultiplier(&foreColor, &clairvoyanceColor);
			}
			plotCharWithColor(theItem->displayChar, x + STAT_BAR_WIDTH, y + MESSAGE_LINES, foreColor, backColor);
			
			if (!fastForward) {
				fastForward = pauseBrogue(25);
			}
			
			refreshDungeonCell(x, y);
		}
		
		if (x == targetLoc[0] && y == targetLoc[1]) { // reached its target
			break;
		}
	}	
	
	if ((theItem->category & POTION) && (hitSomethingSolid || !cellHasTerrainFlag(x, y, TRAP_DESCENT))) {
		if (theItem->kind == POTION_CONFUSION || theItem->kind == POTION_POISON
			|| theItem->kind == POTION_PARALYSIS || theItem->kind == POTION_INCINERATION
			|| theItem->kind == POTION_DARKNESS || theItem->kind == POTION_LICHEN
			|| theItem->kind == POTION_DESCENT) {
			switch (theItem->kind) {
				case POTION_POISON:
					strcpy(buf, "the flask shatters and a deadly purple cloud billows out!");
					spawnDungeonFeature(x, y, &dungeonFeatureCatalog[DF_POISON_GAS_CLOUD], true);
					message(buf, true, false);
					break;
				case POTION_CONFUSION:
					strcpy(buf, "the flask shatters and a multi-hued cloud billows out!");
					spawnDungeonFeature(x, y, &dungeonFeatureCatalog[DF_CONFUSION_GAS_CLOUD], true);
					message(buf, true, false);
					break;
				case POTION_PARALYSIS:
					strcpy(buf, "the flask shatters and a cloud of pink gas billows out!");
					spawnDungeonFeature(x, y, &dungeonFeatureCatalog[DF_PARALYSIS_GAS_CLOUD], true);
					message(buf, true, false);
					break;
				case POTION_INCINERATION:
					lightFlash(&darkOrange, 0, IN_FIELD_OF_VIEW, 4, 4, x, y);
					strcpy(buf, "the flask shatters and its contents burst violently into flame!");
					message(buf, true, false);
					spawnDungeonFeature(x, y, &dungeonFeatureCatalog[DF_FLAMETHROWER], true);
					break;
				case POTION_DARKNESS:
					strcpy(buf, "the flask shatters and the lights in the area start fading.");
					spawnDungeonFeature(x, y, &dungeonFeatureCatalog[DF_DARKNESS_CLOUD], true);
					message(buf, true, false);
					break;
				case POTION_DESCENT:
					lightFlash(&darkBlue, 0, IN_FIELD_OF_VIEW, 3, 3, x, y);
					strcpy(buf, "as the flask shatters, the ground vanishes!");
					message(buf, true, false);
					spawnDungeonFeature(x, y, &dungeonFeatureCatalog[DF_HOLE], true);
					break;
				case POTION_LICHEN:
					strcpy(buf, "the flask shatters and deadly spores spill out!");
					message(buf, true, false);
					spawnDungeonFeature(x, y, &dungeonFeatureCatalog[DF_LICHEN_PLANTED], true);
					break;
			}
			
			identify(theItem);
			
			refreshDungeonCell(x, y);
			
			//if (pmap[x][y].flags & (HAS_MONSTER | HAS_PLAYER)) {
			//	monst = monsterAtLoc(x, y);
			//	applyInstantTileEffectsToCreature(monst);
			//}
		} else {
			if (cellHasTerrainFlag(x, y, OBSTRUCTS_PASSABILITY)) {
				strcpy(buf2, "against");
			} else if (tileCatalog[pmap[x][y].layers[highestPriorityLayer(x, y, false)]].flags & STAND_IN_TILE) {
				strcpy(buf2, "into");
			} else {
				strcpy(buf2, "on");
			}
			sprintf(buf, "the flask shatters and %s liquid splashes harmlessly %s %s.",
					potionTable[theItem->kind].flavor, buf2, tileText(x, y));
			message(buf, true, false);
		}
		return; // potions disappear when they break
	}
	if ((theItem->category & WEAPON) && theItem->kind == INCENDIARY_DART) {
		spawnDungeonFeature(x, y, &dungeonFeatureCatalog[DF_DART_EXPLOSION], true);
		return;
	}
	getQualifyingLocNear(dropLoc, x, y, DCOLS, (OBSTRUCTS_ITEMS | OBSTRUCTS_PASSABILITY), (HAS_ITEM), false);
	placeItem(theItem, dropLoc[0], dropLoc[1]);
	refreshDungeonCell(dropLoc[0], dropLoc[1]);
}

void throwCommand() {
	item *theItem, *previousItem, *thrownItem;
	char buf[COLS], buf2[COLS];
	unsigned char command[10];
	short maxDistance, zapTarget[2], originLoc[2];
	
	command[0] = THROW_KEY;
	theItem = promptForItemOfType((ALL_ITEMS), 0, 0, "Throw what? (a-z, shift for more info; or <esc> to cancel)");
	if (theItem == NULL) {
		return;
	}
	
	command[1] = theItem->inventoryLetter;
	confirmMessages();
	
	if ((theItem->flags & ITEM_EQUIPPED) && theItem->quantity <= 1) {
		itemName(theItem, buf2, false, false);
		sprintf(buf, "Are you sure you want to throw your %s? (y/n)", buf2);
		if (!confirm(buf, false)) {
			return;
		}
		if (!unequipItem(theItem, false)) {
			sprintf(buf, "You cannot unequip your %s; it appears to be cursed.", buf2);
			message(buf, true, false);
			return;
		}
	}
	
	message("Direction? (<hjklyubn>, mouse, or <tab>; <return> to confirm)", true, false);
	maxDistance = (rogue.strength + 2 * max(rogue.strength - 12, 0));
	if (chooseTarget(zapTarget, maxDistance, true, true, false, false)) {
		confirmMessages();
		
		thrownItem = generateItem(-1, -1);
		*thrownItem = *theItem; // clone the item
		thrownItem->flags &= ~ITEM_EQUIPPED;
		thrownItem->quantity = 1;
		
		itemName(thrownItem, buf2, false, false);
		originLoc[0] = player.xLoc;
		originLoc[1] = player.yLoc;
		
		throwItem(thrownItem, &player, zapTarget, maxDistance);
	} else {
		return;
	}
	
	// now decrement or delete the thrown item out of the inventory
	if (theItem->quantity > 1) {
		theItem->quantity--;
	} else {
		for (previousItem = packItems; previousItem->nextItem != theItem; previousItem = previousItem->nextItem);
		previousItem->nextItem = theItem->nextItem;
		free(theItem);
	}
	command[2] = '\0';
	recordKeystrokeSequence(command);
	recordMouseClick(zapTarget[0] + STAT_BAR_WIDTH, zapTarget[1] + MESSAGE_LINES, true, false);
	playerTurnEnded();
}

void apply(item *theItem) {
	item *previousItem;
	char buf[COLS], buf2[COLS];
	unsigned char command[10];
	short zapTarget[2], originLoc[2], maxDistance, c;
	boolean autoTarget, targetAllies, autoID, passThroughCreatures, commandsRecorded;
	
	commandsRecorded = (theItem ? true : false);
	c = 0;
	command[c++] = APPLY_KEY;
	
	if (!theItem) {
		theItem = promptForItemOfType((SCROLL|FOOD|POTION|STAFF|WAND), 0, 0, "Apply what? (a-z, shift for more info; or <esc> to cancel)");
	}
	
	if (theItem == NULL) {
		return;
	}
	command[c++] = theItem->inventoryLetter;
	confirmMessages();
	switch (theItem->category) {
		case FOOD:
			player.status.nutrition = min(foodTable[theItem->kind].strengthRequired + player.status.nutrition, STOMACH_SIZE);
			if (theItem->kind == RATION) {
				message("That food tasted delicious!", true, false);
			} else {
				message("My, what a yummy mango!", true, false);
			}
			break;
		case POTION:
			command[c] = '\0';
			recordKeystrokeSequence(command);
			commandsRecorded = true; // have to record in case further keystrokes are necessary (e.g. enchant scroll)
			potionTable[theItem->kind].identified = true;
			drinkPotion(theItem);
			break;
		case SCROLL:
			command[c] = '\0';
			recordKeystrokeSequence(command);
			commandsRecorded = true; // have to record in case further keystrokes are necessary (e.g. enchant scroll)
			scrollTable[theItem->kind].identified = true;
			readScroll(theItem);
			break;
		case STAFF:
		case WAND:
			if (theItem->charges <= 0 && theItem->flags & ITEM_IDENTIFIED) {
				itemName(theItem, buf2, false, false);
				sprintf(buf, "Your %s has no charges.", buf2);
				message(buf, true, false);
				return;
			}
			message("Direction? (<hjklyubn>, mouse, or <tab>; <return> to confirm)", true, false);
			if (theItem->category & STAFF && theItem->kind == STAFF_BLINKING
				&& theItem->flags & (ITEM_IDENTIFIED | ITEM_MAX_CHARGES_KNOWN)) {
				maxDistance = theItem->enchant1 * 2 + 2;
			} else {
				maxDistance = -1;
			}
			autoTarget = true;
			if (theItem->category & STAFF && staffTable[theItem->kind].identified &&
				(theItem->kind == STAFF_BLINKING
				 || theItem->kind == STAFF_TUNNELING)) {
					autoTarget = false;
				}
			targetAllies = false;
			if ((theItem->category & STAFF && staffTable[theItem->kind].identified &&
				 (theItem->kind == STAFF_HEALING || theItem->kind == STAFF_HASTE))
				|| (theItem->category & WAND && wandTable[theItem->kind].identified &&
					(theItem->kind == WAND_INVISIBILITY || theItem->kind == WAND_PLENTY))) {
					targetAllies = true;
				}
			passThroughCreatures = false;
			if (theItem->category & STAFF && staffTable[theItem->kind].identified &&
				theItem->kind == STAFF_LIGHTNING) {
				passThroughCreatures = true;
			}
			if (chooseTarget(zapTarget, maxDistance, false, autoTarget, targetAllies, passThroughCreatures)) {
				confirmMessages();
				
				originLoc[0] = player.xLoc;
				originLoc[1] = player.yLoc;
				
				if (theItem->charges > 0) {
					autoID = (zap(originLoc, zapTarget,
								  (theItem->kind + (theItem->category == STAFF ? NUMBER_WAND_KINDS : 0)),		// bolt type
								  (theItem->category == STAFF ? theItem->enchant1 : 10),						// bolt level
								  !((theItem->category & WAND && wandTable[theItem->kind].identified)
									|| (theItem->category & STAFF && staffTable[theItem->kind].identified))));	// hide bolt details
					if (autoID) {
						if (theItem->category & STAFF) {
							staffTable[theItem->kind].identified = true;
						} else {
							wandTable[theItem->kind].identified = true;
						}
					}
					command[c] = '\0';
					if (!commandsRecorded) {
						recordKeystrokeSequence(command);
						recordMouseClick(zapTarget[0] + STAT_BAR_WIDTH, zapTarget[1] + MESSAGE_LINES, true, false);
						commandsRecorded = true;
					}
				} else {
					itemName(theItem, buf2, false, false);
					if (theItem->category == STAFF) {
						sprintf(buf, "Your %s fizzles; it must be out of charges for now.", buf2);
					} else {
						sprintf(buf, "Your %s fizzles; it must be depleted.", buf2);
					}
					message(buf, true, false);
					theItem->flags |= ITEM_MAX_CHARGES_KNOWN;
					command[c] = '\0';
					if (!commandsRecorded) {
						recordKeystrokeSequence(command);
						recordMouseClick(zapTarget[0] + STAT_BAR_WIDTH, zapTarget[1] + MESSAGE_LINES, true, false);
						commandsRecorded = true;
					}
					playerTurnEnded();
					return;
				}
			} else {
				return;
			}
			break;
		default:
			itemName(theItem, buf2, false, true);
			sprintf(buf, "you can't apply %s.", buf2);
			message(buf, true, false);
			return;
	}
	
	if (theItem->charges > 0) {
		theItem->charges--;
		if (theItem->category == WAND) {
			theItem->enchant2++; // keeps track of how many times the wand has been discharged for the player's convenience
		}
	} else if (theItem->quantity > 1) {
		theItem->quantity--;
	} else {
		for (previousItem = packItems; previousItem->nextItem != theItem; previousItem = previousItem->nextItem);
		previousItem->nextItem = theItem->nextItem;
		free(theItem);
	}
	if (!commandsRecorded) { // to make sure we didn't already record the keystrokes above with staff/wand targeting
		command[c] = '\0';
		recordKeystrokeSequence(command);
	}
	playerTurnEnded();
}

void identify(item *theItem) {
	itemTable *theTable;
	short tableCount, i, lastItem;
	
	tableCount = 0;
	lastItem = -1;
	
	theItem->flags |= ITEM_IDENTIFIED;
	theItem->flags &= ~ITEM_CAN_BE_IDENTIFIED;
	if (theItem->flags & ITEM_RUNIC) {
		theItem->flags |= (ITEM_RUNIC_IDENTIFIED | ITEM_RUNIC_HINTED);
	}
	
	switch (theItem->category) {
		case SCROLL:
			scrollTable[theItem->kind].identified = true;
			theTable = scrollTable;
			tableCount = NUMBER_SCROLL_KINDS;
			break;
		case POTION:
			potionTable[theItem->kind].identified = true;
			theTable = potionTable;
			tableCount = NUMBER_POTION_KINDS;
			break;
		case WAND:
			wandTable[theItem->kind].identified = true;
			theTable = wandTable;
			tableCount = NUMBER_WAND_KINDS;
			break;
		case STAFF:
			staffTable[theItem->kind].identified = true;
			theTable = staffTable;
			tableCount = NUMBER_STAFF_KINDS;
			break;
		case RING:
			ringTable[theItem->kind].identified = true;
			theTable = ringTable;
			tableCount = NUMBER_RING_KINDS;
			break;
		default:
			break;
	}
	
	if (tableCount) {
		theTable[theItem->kind].identified = true;
		// TODO: this part doesn't work for some reason.
		for (i=0; i<tableCount; i++) {
			if (!(theTable[i].identified)) {
				if (lastItem != -1) {
					return; // at least two unidentified items remain
				}
				lastItem = i;
			}
		}
		if (lastItem != -1) {
			// exactly one unidentified item remains
			theTable[lastItem].identified = true;
		}
	}
}

enum monsterTypes chooseVorpalEnemy() {
	short i, index, possCount = 0, deepestLevel = 0, deepestHorde, chosenHorde, failsafe = 25;
	enum monsterTypes candidate;
	
	do {
		for (i=0; i<NUMBER_HORDES; i++) {
			if (hordeCatalog[i].minLevel >= rogue.depthLevel) {
				possCount += hordeCatalog[i].frequency;
			}
			if (hordeCatalog[i].minLevel > deepestLevel) {
				deepestHorde = i;
				deepestLevel = hordeCatalog[i].minLevel;
			}
		}
		
		if (possCount == 0) {
			chosenHorde = deepestHorde;
		} else {
			index = rand_range(1, possCount);
			for (i=0; i<NUMBER_HORDES; i++) {
				if (hordeCatalog[i].minLevel >= rogue.depthLevel) {
					if (index <= hordeCatalog[i].frequency) {
						chosenHorde = i;
						break;
					}
					index -= hordeCatalog[i].frequency;
				}
			}
		}
		
		index = rand_range(-1, hordeCatalog[chosenHorde].numberOfMemberTypes - 1);
		if (index == -1) {
			candidate = hordeCatalog[chosenHorde].leaderType;
		} else {
			candidate = hordeCatalog[chosenHorde].memberType[index];
		}
	} while (monsterCatalog[candidate].flags & MONST_INANIMATE && --failsafe > 0);
	return candidate;
}

void updateIdentifiableItems() {
	item *theItem;
	for (theItem = packItems->nextItem; theItem != NULL; theItem = theItem->nextItem) {
		if ((theItem->category & (SCROLL) && scrollTable[theItem->kind].identified == true)
			|| (theItem->category & (POTION) && potionTable[theItem->kind].identified == true)) {
			theItem->flags &= ~ITEM_CAN_BE_IDENTIFIED;
		}
	}
}

void readScroll(item *theItem) {
	short i, j, x, y, numberOfMonsters = 0;
	item *tempItem;
	creature *monst;
	boolean hadEffect = false;
	char buf[2*COLS], buf2[COLS];
	
	switch (theItem->kind) {
		case SCROLL_IDENTIFY:
			updateIdentifiableItems();
			message("this is a scroll of identify.", true, true);
			if (numberOfMatchingPackItems(ALL_ITEMS, ITEM_CAN_BE_IDENTIFIED, 0, false) == 0) {
				message("everything in your pack is already identified.", true, false);
				break;
			}
			do {
				theItem = promptForItemOfType((ALL_ITEMS), ITEM_CAN_BE_IDENTIFIED, 0, "Identify what? (a-z; shift for more info)");
				if (rogue.gameHasEnded) {
					return;
				}
				if (theItem && !(theItem->flags & ITEM_CAN_BE_IDENTIFIED)) {
					confirmMessages();
					itemName(theItem, buf2, true, true);
					sprintf(buf, "you already know %s %s.", (theItem->quantity > 1 ? "they're" : "it's"), buf2);
					message(buf, true, false);
				}
			} while (theItem == NULL || !(theItem->flags & ITEM_CAN_BE_IDENTIFIED));
			recordKeystroke(theItem->inventoryLetter, false, false);
			confirmMessages();
			identify(theItem);
			itemName(theItem, buf, true, true);
			sprintf(buf2, "%s %s.", (theItem->quantity == 1 ? "this is" : "these are"), buf);
			message(buf2, true, false);
			break;
		case SCROLL_TELEPORT:
			teleport(&player);
			break;
		case SCROLL_REMOVE_CURSE:
			for (tempItem = packItems->nextItem; tempItem != NULL; tempItem = tempItem->nextItem) {
				if (tempItem->flags & ITEM_CURSED) {
					hadEffect = true;
					tempItem->flags &= ~ITEM_CURSED;
				}
			}
			if (hadEffect) {
				message("your pack glows with a cleansing light, and a malevolent energy disperses.", true, false);
			} else {
				message("your pack glows with a cleansing light, but nothing happens.", true, false);
			}
			break;
		case SCROLL_ENCHANT_ITEM:
			message("this is a scroll of enchantment.", true, true);
			if (!numberOfMatchingPackItems(WEAPON | ARMOR | RING | STAFF | WAND, 0, 0, false)) {
				confirmMessages();
				message("you have nothing that can be enchanted.", true, false);
				break;
			}
			do {
				theItem = promptForItemOfType((WEAPON | ARMOR | RING | STAFF | WAND), 0, 0, "Enchant what? (a-z; shift for more info)");
				confirmMessages();
				if (theItem == NULL || !(theItem->category & (WEAPON | ARMOR | RING | STAFF | WAND))) {
					message("Can't enchant that.", true, true);
				}
				if (rogue.gameHasEnded) {
					return;
				}
			} while (theItem == NULL || !(theItem->category & (WEAPON | ARMOR | RING | STAFF | WAND)));
			recordKeystroke(theItem->inventoryLetter, false, false);
			confirmMessages();
			switch (theItem->category) {
				case WEAPON:
					theItem->strengthRequired = max(0, theItem->strengthRequired - 1);
					theItem->enchant1++;
					break;
				case ARMOR:
					theItem->strengthRequired = max(0, theItem->strengthRequired - 1);
					theItem->enchant1++;
					break;
				case RING:
					theItem->enchant1++;
					if (theItem->kind == RING_CLAIRVOYANCE) {
						updateClairvoyance();
						displayLevel();
					}
					break;
				case STAFF:
					theItem->enchant1++;
					theItem->charges++;
					theItem->enchant2 = 500 / theItem->enchant1;
					break;
				case WAND:
					theItem->charges += rand_range(1, 2);
					break;
				default:
					break;
			}
			if (theItem->flags & ITEM_EQUIPPED) {
				equipItem(theItem, true);
			}
			itemName(theItem, buf, false, false);
			sprintf(buf2, "your %s shines with an inner light.", buf);
			message(buf2, true, false);
			if (theItem->flags & ITEM_CURSED) {
				sprintf(buf2, "a malevolent force leaves your %s.", buf);
				message(buf2, true, false);				
				theItem->flags &= ~ITEM_CURSED;
			}
			break;
		case SCROLL_RECHARGING:
			x = y = 0; // x counts staffs, y counts wands
			for (tempItem = packItems->nextItem; tempItem != NULL; tempItem = tempItem->nextItem) {
				if (tempItem->category & STAFF) {
					x++;
					tempItem->charges = tempItem->enchant1;
					tempItem->enchant2 = 5000;
				}
				if (tempItem->category & WAND) {
					y++;
					tempItem->charges++;
				}
			}
			if (x || y) {
				sprintf(buf, "a surge of energy courses through your pack, recharging your %s%s%s.",
						(x > 1 ? "staffs" : (x == 1 ? "staff" : "")),
						(x && y ? " and " : ""),
						(y > 1 ? "wands" : (y == 1 ? "wand" : "")));
				message(buf, true, false);
			} else {
				message("a surge of energy courses through your pack, but nothing happens.", true, false);
			}
			break;
		case SCROLL_PROTECT_ARMOR:
			if ((tempItem = rogue.armor)) {
				tempItem->flags |= ITEM_PROTECTED;
				message("a protective golden light covers your armor.", true, false);
				if (tempItem->flags & ITEM_CURSED) {
					itemName(tempItem, buf, false, false);
					sprintf(buf2, "a malevolent force leaves your %s.", buf);
					message(buf2, true, false);				
					tempItem->flags &= ~ITEM_CURSED;
				}
			} else {
				message("a protective golden light surrounds you, but it quickly disperses.", true, false);
			}
			break;
		case SCROLL_PROTECT_WEAPON:
			if ((tempItem = rogue.weapon)) {
				tempItem->flags |= ITEM_PROTECTED;
				message("a protective golden light covers your weapon.", true, false);
				if (tempItem->flags & ITEM_CURSED) {
					itemName(tempItem, buf, false, false);
					sprintf(buf2, "a malevolent force leaves your %s.", buf);
					message(buf2, true, false);				
					tempItem->flags &= ~ITEM_CURSED;
				}
			} else {
				message("a protective golden light covers your hands, but it quickly disperses.", true, false);
			}
			break;
		case SCROLL_MAGIC_MAPPING:
			for (i=0; i<DCOLS; i++) {
				for (j=0; j<DROWS; j++) {
					if (cellHasTerrainFlag(i, j, IS_SECRET)) {
						discover(i, j);
						pmap[i][j].flags &= ~STABLE_MEMORY;
						refreshDungeonCell(i, j);
					}
				}
			}
			for (i=0; i<DCOLS; i++) {
				for (j=0; j<DROWS; j++) {
					if (!(pmap[i][j].flags & DISCOVERED) && pmap[i][j].layers[DUNGEON] != GRANITE) {
						pmap[i][j].flags |= MAGIC_MAPPED;
						refreshDungeonCell(i, j);
					}
				}
			}
			message("this scroll has a map on it!", true, false);
			break;
		case SCROLL_AGGRAVATE_MONSTER:
			aggravateMonsters();
			message("the scroll emits a piercing shriek that echoes throughout the dungeon!", true, false);
			break;
		case SCROLL_SUMMON_MONSTER:
			for (j=0; j<25 && numberOfMonsters < 3; j++) {
				for (i=0; i<8; i++) {
					x = player.xLoc + nbDirs[i][0];
					y = player.yLoc + nbDirs[i][1];
					if (!cellHasTerrainFlag(x, y, OBSTRUCTS_PASSABILITY) && !(pmap[x][y].flags & HAS_MONSTER)
						&& rand_percent(10) && (numberOfMonsters < 3)) {
						monst = spawnHorde(0, x, y, HORDE_LEADER_CAPTIVE | NO_PERIODIC_SPAWN);
						if (monst) {
							// refreshDungeonCell(x, y);
							// monst->creatureState = MONSTER_TRACKING_SCENT;
							// monst->ticksUntilTurn = player.movementSpeed;
							wakeUp(monst);
							fadeInMonster(monst);
							numberOfMonsters++;
						}
					}
				}
			}
			if (numberOfMonsters > 1) {
				message("the fabric of space ripples, and monsters appear!", true, false);
			} else if (numberOfMonsters == 1) {
				message("the fabric of space ripples, and a monster appears!", true, false);
			} else {
				message("the fabric of space boils violently around you, but nothing happens.", true, false);
			}
			break;
		case SCROLL_CAUSE_FEAR:
			for (monst = monsters->nextCreature; monst != NULL; monst = monst->nextCreature) {
				if (pmap[monst->xLoc][monst->yLoc].flags & IN_FIELD_OF_VIEW
					&& monst->creatureState != MONSTER_FLEEING
					&& !(monst->info.flags & MONST_INANIMATE)) {
					unAlly(monst);
					monst->status.magicalFear = monst->maxStatus.magicalFear = rand_range(150, 225);
					monst->creatureState = MONSTER_FLEEING;
					chooseNewWanderDestination(monst);
					if (canSeeMonster(monst)) {
						numberOfMonsters++;
						monsterName(buf2, monst, true);
					}
				}
			}
			if (numberOfMonsters > 1) {
				message("the scroll emits a brilliant flash of red light, and the monsters flee!", true, false);
			} else if (numberOfMonsters == 1) {
				sprintf(buf, "the scroll emits a brilliant flash of red light, and %s flees!", buf2);
				message(buf, true, false);
			} else {
				message("the scroll emits a brilliant flash of red light!", true, false);
			}
			lightFlash(&redFlashColor, 0, IN_FIELD_OF_VIEW, 15, DCOLS, player.xLoc, player.yLoc);
			break;
		case SCROLL_SANCTUARY:
			message("the scroll turns into ashes. It probably wasn't intended to be read.", true, false);
			break;
	}
}

void drinkPotion(item *theItem) {
	item *tempItem;
	creature *monst;
	boolean hadEffect = false;
	boolean hadEffect2 = false;
	
	switch (theItem->kind) {
		case POTION_HEALING:
			if (player.status.hallucinating > 2) {
				player.status.hallucinating /= 2;
			}
			if (player.status.confused > 1) {
				player.status.confused = 1;
			}
			if (player.status.nauseous > 1) {
				player.status.nauseous = 1;
			}
			if (player.status.slowed > 1) {
				player.status.slowed = 1;
			}
			if (player.status.poisoned) {
				player.status.poisoned = 0;
			}
			if (player.status.darkness > 0) {
				player.status.darkness /= 2;
				updateMinersLightRadius();
				updateVision();
			}
			if (player.currentHP < player.info.maxHP) {
				player.currentHP = min(player.info.maxHP, player.currentHP + player.info.maxHP / 2);
				// updateStatBar(HP_CURRENT_STAT);
				message("your wounds mend.", true, false);
			} else {
				player.currentHP += (player.info.maxHP / 5);
				// updateStatBar(HP_CURRENT_STAT);
				message("you feel energized!", true, false);
			}
			break;
		case POTION_EXTRA_HEALING:
			if (player.status.hallucinating > 1) {
				player.status.hallucinating = 1;
			}
			if (player.status.confused > 1) {
				player.status.confused = 1;
			}
			if (player.status.nauseous > 1) {
				player.status.nauseous = 1;
			}
			if (player.status.slowed > 1) {
				player.status.slowed = 1;
			}
			if (player.status.poisoned) {
				player.status.poisoned = 0;
			}
			if (player.status.darkness > 0) {
				player.status.darkness = 0;
				updateMinersLightRadius();
				updateVision();
			}
			if (player.currentHP < player.info.maxHP) {
				player.currentHP = player.info.maxHP;
				// updateStatBar(HP_CURRENT_STAT);
				message("your wounds heal completely.", true, false);
			} else {
				player.currentHP += (player.info.maxHP / 3);
				// updateStatBar(HP_CURRENT_STAT);
				message("you feel invigorated!", true, false);
			}
			break;
		case POTION_HALLUCINATION:
			player.status.hallucinating = player.maxStatus.hallucinating = 300;
			message("colors are everywhere! The walls are singing!", true, false);
			break;
		case POTION_INCINERATION:
			lightFlash(&darkOrange, 0, IN_FIELD_OF_VIEW, 4, 4, player.xLoc, player.yLoc);
			message("as you uncork the flask, it explodes in flame!", true, false);
			spawnDungeonFeature(player.xLoc, player.yLoc, &dungeonFeatureCatalog[DF_FLAMETHROWER], true);
			exposeCreatureToFire(&player);
			break;
		case POTION_DARKNESS:
			player.status.darkness = max(400, player.status.darkness);
			player.maxStatus.darkness = max(400, player.maxStatus.darkness);
			updateMinersLightRadius();
			updateVision();
			message("your vision flickers as a cloak of darkness settles around you!", true, false);
			break;
		case POTION_DESCENT:
			lightFlash(&darkBlue, 0, IN_FIELD_OF_VIEW, 3, 3, player.xLoc, player.yLoc);
			message("vapor pours out of the flask and causes the floor to disappear!", true, false);
			spawnDungeonFeature(player.xLoc, player.yLoc, &dungeonFeatureCatalog[DF_HOLE], true);
			break;
		case POTION_GAIN_LEVEL:
			addExperience(levelPoints[rogue.experienceLevel - 1] - rogue.experience);
			break;
		case POTION_GAIN_STRENGTH:
			rogue.strength++;
			updateEncumbrance();
			message("newfound strength surges through your body.", true, false);
			break;
		case POTION_POISON:
			spawnDungeonFeature(player.xLoc, player.yLoc, &dungeonFeatureCatalog[DF_POISON_GAS_CLOUD], true);
			message("poisonous gas billows out of the open flask!", true, false);
			break;
		case POTION_PARALYSIS:
			spawnDungeonFeature(player.xLoc, player.yLoc, &dungeonFeatureCatalog[DF_PARALYSIS_GAS_CLOUD], true);
			message("your muscles stiffen as a cloud of pink gas bursts from the open flask!", true, false);
			break;
		case POTION_TELEPATHY:
			player.status.telepathic = player.maxStatus.telepathic = 300;
			for (monst=monsters->nextCreature; monst != NULL; monst = monst->nextCreature) {
				refreshDungeonCell(monst->xLoc, monst->yLoc);
			}
			if (monsters->nextCreature == NULL) {
				message("you can somehow tell that you are alone on this level at the moment.", true, false);
			} else {
				message("you can somehow feel the presence of other creatures' minds!", true, false);
			}
			break;
		case POTION_LEVITATION:
			player.status.levitating = player.maxStatus.levitating = 30;
			player.bookkeepingFlags &= ~MONST_SEIZED; // break free of holding monsters
			message("you float into the air!", true, false);
			break;
		case POTION_CONFUSION:
			spawnDungeonFeature(player.xLoc, player.yLoc, &dungeonFeatureCatalog[DF_CONFUSION_GAS_CLOUD], true);
			message("a shimmering cloud of rainbow-colored gas billows out of the open flask!", true, false);
			break;
		case POTION_LICHEN:
			message("a handful of tiny spores burst out of the open flask!", true, false);
			spawnDungeonFeature(player.xLoc, player.yLoc, &dungeonFeatureCatalog[DF_LICHEN_PLANTED], true);
			break;
		case POTION_DETECT_MAGIC:
			hadEffect = false;
			hadEffect2 = false;
			for (tempItem = floorItems->nextItem; tempItem != NULL; tempItem = tempItem->nextItem) {
				tempItem->flags |= ITEM_MAGIC_DETECTED;
				if (itemMagicChar(tempItem)) {
					pmap[tempItem->xLoc][tempItem->yLoc].flags |= ITEM_DETECTED;
					hadEffect = true;
					refreshDungeonCell(tempItem->xLoc, tempItem->yLoc);
				}
			}
			for (monst = monsters->nextCreature; monst != NULL; monst = monst->nextCreature) {
				if (monst->carriedItem) {
					monst->carriedItem->flags |= ITEM_MAGIC_DETECTED;
					if (itemMagicChar(monst->carriedItem)) {
						hadEffect = true;
						refreshDungeonCell(monst->xLoc, monst->yLoc);
					}
				}
			}
			for (tempItem = packItems->nextItem; tempItem != NULL; tempItem = tempItem->nextItem) {
				tempItem->flags |= ITEM_MAGIC_DETECTED;
				if (itemMagicChar(tempItem)) {
					updateItemKnownMagicStatus(tempItem);
					if (tempItem->flags & ITEM_MAGIC_DETECTED) {
						hadEffect2 = true;
					}
				}
			}
			if (hadEffect || hadEffect2) {
				if (hadEffect && hadEffect2) {
					message("you can somehow feel the presence of magic on the level and in your pack.", true, false);
				} else if (hadEffect) {
					message("you can somehow feel the presence of magic on the level.", true, false);
				} else {
					message("you can somehow feel the presence of magic in your pack.", true, false);
				}
			} else {
				message("you can somehow feel the absence of magic on the level and in your pack.", true, false);
			}
			break;
		case POTION_HASTE_SELF:
			haste(&player, 25);
			break;
		case POTION_FIRE_IMMUNITY:
			player.status.immuneToFire = player.maxStatus.immuneToFire = 150;
			player.info.flags |= MONST_IMMUNE_TO_FIRE;
			if (player.status.burning) {
				extinguishFireOnCreature(&player);
			}
			message("you no longer fear fire.", true, false);
			break;
		default:
			message("you feel very strange, as though your body doesn't know how to react!", true, true);
	}
}

uchar itemMagicChar(item *theItem) {
	switch (theItem->category) {
		case WEAPON:
			if (theItem->flags & ITEM_CURSED) {
				return BAD_MAGIC_CHAR;
			} else if (theItem->enchant1 > 0) {
				return GOOD_MAGIC_CHAR;
			}
			return 0;
			break;
		case ARMOR:
			if (theItem->flags & ITEM_CURSED) {
				return BAD_MAGIC_CHAR;
			} else if (theItem->enchant1 > 0) {
				return GOOD_MAGIC_CHAR;
			}
			return 0;
			break;
		case SCROLL:
			switch (theItem->kind) {
				case SCROLL_AGGRAVATE_MONSTER:
				case SCROLL_SUMMON_MONSTER:
					return BAD_MAGIC_CHAR;
				default:
					return GOOD_MAGIC_CHAR;
			}
		case POTION:
			switch (theItem->kind) {
				case POTION_HALLUCINATION:
				case POTION_INCINERATION:
				case POTION_DESCENT:
				case POTION_POISON:
				case POTION_PARALYSIS:
				case POTION_CONFUSION:
				case POTION_LICHEN:
				case POTION_DARKNESS:
					return BAD_MAGIC_CHAR;
				default:
					return GOOD_MAGIC_CHAR;
			}
		case WAND:
			if (theItem->charges == 0) {
				return 0;
			}
			switch (theItem->kind) {
				case WAND_PLENTY:
				case WAND_INVISIBILITY:
				case WAND_BECKONING:
					return BAD_MAGIC_CHAR;
				default:
					return GOOD_MAGIC_CHAR;
			}
		case STAFF:
			switch (theItem->kind) {
				case STAFF_HEALING:
				case STAFF_HASTE:
					return BAD_MAGIC_CHAR;
				default:
					return GOOD_MAGIC_CHAR;
			}
		case RING:
			if (theItem->flags & ITEM_CURSED || theItem->enchant1 < 0) {
				return BAD_MAGIC_CHAR;
			} else if (theItem->enchant1 > 0) {
				return GOOD_MAGIC_CHAR;
			} else {
				return 0;
			}
		case AMULET:
			return AMULET_CHAR;
	}
	return 0;
}

void unequip() {
	item *theItem;
	char buf[COLS], buf2[COLS];
	unsigned char command[3];
	
	command[0] = UNEQUIP_KEY;
	theItem = promptForItemOfType(ALL_ITEMS, ITEM_EQUIPPED, 0, "Remove (unequip) what? (a-z or <esc> to cancel)");
	if (theItem == NULL) {
		return;
	}
	
	command[1] = theItem->inventoryLetter;
	command[2] = '\0';
	
	if (!(theItem->flags & (ITEM_EQUIPPED))) {
		itemName(theItem, buf2, false, false);
		sprintf(buf, "your %s was not equipped.", buf2);
		confirmMessages();
		message(buf, true, false);
		return;
	} else if (!unequipItem(theItem, false)) { // this is where the item gets unequipped
		itemName(theItem, buf2, false, false);
		sprintf(buf, "you can't; your %s appears to be cursed.", buf2);
		confirmMessages();
		message(buf, true, false);
		return;
	} else {
		if (theItem->category & RING) {
			updateRingBonuses();
		}
		itemName(theItem, buf2, true, true);
		if (strlen(buf2) > 52) {
			itemName(theItem, buf2, false, true);
		}
		confirmMessages();
		updateEncumbrance();
		sprintf(buf, "you are no longer %s %s.", (theItem->category & WEAPON ? "wielding" : "wearing"), buf2);
		message(buf, true, false);
	}
	recordKeystrokeSequence(command);
	playerTurnEnded();
}

void drop() {
	item *theItem;
	char buf[COLS], buf2[COLS];
	unsigned char command[3];
	
	command[0] = DROP_KEY;
	theItem = promptForItemOfType(ALL_ITEMS, 0, 0, "Drop what? (a-z, shift for more info; or <esc> to cancel)");
	if (theItem == NULL) {
		return;
	}
	command[1] = theItem->inventoryLetter;
	command[2] = '\0';
	
	if (theItem->flags & (ITEM_EQUIPPED) && !unequipItem(theItem, false)) { // this is where the item gets unequipped
		itemName(theItem, buf2, false, false);
		sprintf(buf, "you can't; your %s appears to be cursed.", buf2);
		confirmMessages();
		message(buf, true, false);
		return;
	}
	if ((theItem = dropItem(theItem))) { // this is where it gets dropped
		itemName(theItem, buf2, true, true);
		sprintf(buf, "You dropped %s.", buf2);
		message(buf, true, false);
		recordKeystrokeSequence(command);
		playerTurnEnded();
	} else {
		confirmMessages();
		message("There is already something there.", true, false);
	}
}

item *promptForItemOfType(unsigned short category,
						  unsigned long requiredFlags,
						  unsigned long forbiddenFlags,
						  char *prompt) {
	char keystroke;
	item *theItem;
	
	if (!numberOfMatchingPackItems(ALL_ITEMS, requiredFlags, forbiddenFlags, true)) {
		return NULL;
	}
	
	temporaryMessage(prompt, false);
	
	keystroke = displayInventory(category, requiredFlags, forbiddenFlags, false);
	
	if (!keystroke) {
		return NULL;
	}
	
	if (keystroke < 'a' || keystroke > 'z') {
		confirmMessages();
		if (keystroke != ESCAPE_KEY && keystroke != ACKNOWLEDGE_KEY) {
			message("Invalid entry.", true, false);
		}
		return NULL;
	}
	
	theItem = itemOfPackLetter(keystroke);
	if (theItem == NULL) {
		confirmMessages();
		message("No such item.", true, false);
		return NULL;
	}
	
	return theItem;
}

item *itemOfPackLetter(char letter) {
	item *theItem;
	for (theItem = packItems->nextItem; theItem != NULL; theItem = theItem->nextItem) {
		if (theItem->inventoryLetter == letter) {
			return theItem;
		}
	}
	return NULL;
}

item *itemAtLoc(short x, short y) {
	item *theItem;
	if (!(pmap[x][y].flags & HAS_ITEM)) {
		return NULL; // easy optimization
	}
	for (theItem = floorItems->nextItem; theItem != NULL && (theItem->xLoc != x || theItem->yLoc != y); theItem = theItem->nextItem);
	return theItem;
}

item *dropItem(item *theItem) {
	item *previousItem, *itemFromTopOfStack, *itemOnFloor;
	
	if (cellHasTerrainFlag(player.xLoc, player.yLoc, OBSTRUCTS_ITEMS)) {
		return NULL;
	}
	
	itemOnFloor = itemAtLoc(player.xLoc, player.yLoc);
	if (itemOnFloor && (itemOnFloor->flags & ITEM_NO_PICKUP)) {
		return NULL;
	}
	
	if (theItem->quantity > 1 && !(theItem->category & WEAPON)) { // peel off the top item and drop it
		itemFromTopOfStack = generateItem(-1, -1);
		*itemFromTopOfStack = *theItem; // clone the item
		theItem->quantity--;
		itemFromTopOfStack->quantity = 1;
		if (itemOnFloor) {
			itemOnFloor->inventoryLetter = theItem->inventoryLetter; // just in case all letters are taken
			pickUpItemAt(player.xLoc, player.yLoc);
		}
		placeItem(itemFromTopOfStack, player.xLoc, player.yLoc);
		return itemFromTopOfStack;
	} else { // drop the entire item
		for (previousItem = packItems; previousItem->nextItem != theItem; previousItem = previousItem->nextItem);
		previousItem->nextItem = theItem->nextItem;
		if (itemOnFloor) {
			itemOnFloor->inventoryLetter = theItem->inventoryLetter;
			pickUpItemAt(player.xLoc, player.yLoc);
		}
		placeItem(theItem, player.xLoc, player.yLoc);
		return theItem;
	}
}

void recalculateEquipmentBonuses() {
	float enchant;
	item *theItem;
	
	if (rogue.weapon) {
		theItem = rogue.weapon;
		enchant = netEnchant(theItem);
		player.info.damage = theItem->damage;
		player.info.damage.lowerBound *= pow(1.065, enchant);
		player.info.damage.upperBound *= pow(1.065, enchant);
		if (player.info.damage.lowerBound < 1) {
			player.info.damage.lowerBound = 1;
		}
		if (player.info.damage.upperBound < 1) {
			player.info.damage.upperBound = 1;
		}
	}
	
	if (rogue.armor) {
		theItem = rogue.armor;
		enchant = netEnchant(theItem);
		player.info.defense = theItem->armor + enchant * 10;
		if (player.info.defense < 0) {
			player.info.defense = 0;
		}
	}
}

boolean equipItem(item *theItem, boolean force) {
	item *previouslyEquippedItem = NULL;
	short HPBonus;
	
	if (theItem->category & RING && theItem->flags & ITEM_EQUIPPED) {
		updateRingBonuses();
		return true;
	}
	
	if (theItem->category & WEAPON) {
		previouslyEquippedItem = rogue.weapon;
	} else if (theItem->category & ARMOR) {
		previouslyEquippedItem = rogue.armor;
	}
	if (previouslyEquippedItem && !unequipItem(previouslyEquippedItem, force)) {
		return false; // already using a cursed item
	}
	if (theItem->category & WEAPON) {
		rogue.weapon = theItem;
		recalculateEquipmentBonuses();
	} else if (theItem->category & ARMOR) {
		rogue.armor = theItem;
		recalculateEquipmentBonuses();
		if (rogue.armor->flags & ITEM_RUNIC && rogue.armor->enchant2 == A_TOUGHNESS) {
			// any changes here must be mirrored PRECISELY in unequipItem().
			HPBonus = 5 * theItem->enchant1;
			player.currentHP += (HPBonus * player.currentHP / player.info.maxHP);
			player.info.maxHP += HPBonus;
		}
	} else if (theItem->category & RING) {
		if (rogue.ringLeft && rogue.ringRight) {
			return false;
		}
		if (rogue.ringLeft) {
			rogue.ringRight = theItem;
		} else {
			rogue.ringLeft = theItem;
		}
		updateRingBonuses();
		if (theItem->kind == RING_CLAIRVOYANCE) {
			updateClairvoyance();
			displayLevel();
			ringTable[RING_CLAIRVOYANCE].identified = true;
		} else if (theItem->kind == RING_LIGHT) {
			ringTable[RING_LIGHT].identified = true;
		}
	}
	theItem->flags |= ITEM_EQUIPPED;
	return true;
}

boolean unequipItem(item *theItem, boolean force) {
	short HPBonus;
	
	if (theItem == NULL) {
		return true;
	}
	if (theItem->flags & ITEM_CURSED && !force) {
		return false;
	}
	theItem->flags &= ~ITEM_EQUIPPED;
	if (theItem->category & WEAPON) {
		player.info.damage.lowerBound = 1;
		player.info.damage.upperBound = 2;
		player.info.damage.clumpFactor = 1;
		rogue.weapon = NULL;
	}
	if (theItem->category & ARMOR) {
		if (rogue.armor->flags & ITEM_RUNIC && rogue.armor->enchant2 == A_TOUGHNESS) {
			// any changes here MUST BE mirrored in equipItem().
			HPBonus = 5 * theItem->enchant1;
			player.currentHP -= (HPBonus * player.currentHP / player.info.maxHP);
			player.info.maxHP -= HPBonus;
			if (player.currentHP <= 0) {
				player.currentHP = 1; // just in case
			}
		}
		player.info.defense = 0;
		rogue.armor = NULL;
	}
	if (theItem->category & RING) {
		if (rogue.ringLeft == theItem) {
			rogue.ringLeft = NULL;
		} else if (rogue.ringRight == theItem) {
			rogue.ringRight = NULL;
		}
		updateRingBonuses();
		if (theItem->kind == RING_CLAIRVOYANCE) {
			updateClairvoyance();
			updateClairvoyance();
			displayLevel();
			//updateFieldOfViewDisplay();
			//updateClairvoyance();
		}
	}
	updateEncumbrance();
	return true;
}

void updateRingBonuses() {
	short i;
	item *rings[2] = {rogue.ringLeft, rogue.ringRight};
	
	rogue.clairvoyance = rogue.aggravating = rogue.stealthBonus = rogue.transference
	= rogue.awarenessBonus = rogue.regenerationBonus = rogue.reflectionBonus = 0;
	rogue.lightMultiplier = 1;
	
	for (i=0; i<= 1; i++) {
		if (rings[i]) {
			switch (rings[i]->kind) {
				case RING_CLAIRVOYANCE:
					rogue.clairvoyance += rings[i]->enchant1;
					break;
				case RING_STEALTH:
					rogue.stealthBonus += rings[i]->enchant1;
					break;
				case RING_REGENERATION:
					rogue.regenerationBonus += rings[i]->enchant1;
					break;
				case RING_TRANSFERENCE:
					rogue.transference += rings[i]->enchant1;
					break;
				case RING_LIGHT:
					rogue.lightMultiplier += rings[i]->enchant1;
					break;
				case RING_AWARENESS:
					rogue.awarenessBonus += 20 * rings[i]->enchant1;
					break;
				case RING_REFLECTION:
					rogue.reflectionBonus += rings[i]->enchant1;
					break;
			}
		}
	}
	
	if (rogue.lightMultiplier <= 0) {
		rogue.lightMultiplier--; // because it starts at positive 1 instead of 0
	}
	
	updateMinersLightRadius();
	updatePlayerRegenerationDelay();
	
	if (rogue.stealthBonus < 0) {
		rogue.stealthBonus *= 4;
		rogue.aggravating = true;
	}
	
	if (rogue.aggravating) {
		aggravateMonsters();
	}
}

void updatePlayerRegenerationDelay() {
	short maxHP;
	long turnsForFull;
	maxHP = player.info.maxHP;
	turnsForFull = (long) (1000 * TURNS_FOR_FULL_REGEN * pow(0.75, rogue.regenerationBonus));
	
	player.regenPerTurn = 0;
	while (maxHP > turnsForFull / 1000) {
		player.regenPerTurn++;
		maxHP -= turnsForFull / 1000;
	}
	
	player.info.turnsBetweenRegen = (turnsForFull / maxHP);
	// DEBUG printf("\nTurnsForFull: %i; regenPerTurn: %i; (thousandths of) turnsBetweenRegen: %i", turnsForFull, player.regenPerTurn, player.info.turnsBetweenRegen);
}

void deleteItem(item *theItem) {
	free(theItem);
}

void resetItemTableEntry(itemTable *theEntry) {
	theEntry->identified = false;
	theEntry->called = false;
	theEntry->callTitle[0] = '\0';
}

void shuffleFlavors() {
	short i, j, randIndex, randNumber;
	char buf[COLS];
	
	//	for (i=0; i<NUMBER_FOOD_KINDS; i++) {
	//		resetItemTableEntry(foodTable + i);
	//	}
	for (i=0; i<NUMBER_POTION_KINDS; i++) {
		resetItemTableEntry(potionTable + i);
	}
	//	for (i=0; i<NUMBER_WEAPON_KINDS; i++) {
	//		resetItemTableEntry(weaponTable + i);
	//	}
	//	for (i=0; i<NUMBER_ARMOR_KINDS; i++) {
	//		resetItemTableEntry(armorTable + i);
	//	}
	for (i=0; i<NUMBER_STAFF_KINDS; i++) {
		resetItemTableEntry(staffTable+ i);
	}
	for (i=0; i<NUMBER_WAND_KINDS; i++) {
		resetItemTableEntry(wandTable + i);
	}
	for (i=0; i<NUMBER_SCROLL_KINDS; i++) {
		resetItemTableEntry(scrollTable + i);
	}
	for (i=0; i<NUMBER_RING_KINDS; i++) {
		resetItemTableEntry(ringTable + i);
	}
	
	for (i=0; i<NUMBER_ITEM_COLORS; i++) {
		strcpy(itemColors[i], itemColorsRef[i]);
	}
	for (i=0; i<NUMBER_ITEM_COLORS; i++) {
		randIndex = rand_range(0, NUMBER_ITEM_COLORS - 1);
		strcpy(buf, itemColors[i]);
		strcpy(itemColors[i], itemColors[randIndex]);
		strcpy(itemColors[randIndex], buf);
	}
	
	for (i=0; i<NUMBER_ITEM_WOODS; i++) {
		strcpy(itemWoods[i], itemWoodsRef[i]);
	}
	for (i=0; i<NUMBER_ITEM_WOODS; i++) {
		randIndex = rand_range(0, NUMBER_ITEM_WOODS - 1);
		strcpy(buf, itemWoods[i]);
		strcpy(itemWoods[i], itemWoods[randIndex]);
		strcpy(itemWoods[randIndex], buf);
	}
	
	for (i=0; i<NUMBER_ITEM_GEMS; i++) {
		strcpy(itemGems[i], itemGemsRef[i]);
	}
	for (i=0; i<NUMBER_ITEM_GEMS; i++) {
		randIndex = rand_range(0, NUMBER_ITEM_GEMS - 1);
		strcpy(buf, itemGems[i]);
		strcpy(itemGems[i], itemGems[randIndex]);
		strcpy(itemGems[randIndex], buf);
	}
	
	for (i=0; i<NUMBER_ITEM_METALS; i++) {
		strcpy(itemMetals[i], itemMetalsRef[i]);
	}
	for (i=0; i<NUMBER_ITEM_METALS; i++) {
		randIndex = rand_range(0, NUMBER_ITEM_METALS - 1);
		strcpy(buf, itemMetals[i]);
		strcpy(itemMetals[i], itemMetals[randIndex]);
		strcpy(itemMetals[randIndex], buf);
	}
	
	for (i=0; i<NUMBER_SCROLL_KINDS; i++) {
		itemTitles[i][0] = '\0';
		randNumber = rand_range(3, 4);
		for (j=0; j<randNumber; j++) {
			randIndex = rand_range(0, NUMBER_TITLE_PHONEMES - 1);
			strcpy(buf, itemTitles[i]);
			sprintf(itemTitles[i], "%s%s%s", buf, ((rand_percent(50) && j>0) ? " " : ""), titlePhonemes[randIndex]);
		}
	}
}
