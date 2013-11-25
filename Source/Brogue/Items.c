/*
 *  Items.c
 *  Brogue
 *
 *  Created by Brian Walker on 1/17/09.
 *  Copyright 2009. All rights reserved.
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


#include "rogue.h"
#include "IncludeGlobals.h"
#include <math.h>

// Allocates space, generates a specified item (or random category/kind if -1)
// and returns a pointer to that item. Note that the item
// is not given a location here and is not inserted into the item chain!
item *generateItem(short theCategory, short theKind) {
	item *theItem;
	theItem = (item *) malloc(sizeof(item));
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
	makeItemInto(theItem, theCategory, theKind);
	return theItem;
}

unsigned short pickItemCategory() {
	short i, sum, randIndex;
	short probabilities[11] =						{50,	40,		40,		4,		4,		11,		8,		5,		4,		0,			0};
	unsigned short correspondingCategories[11] =	{GOLD,	SCROLL,	POTION,	STAFF,	WAND,	WEAPON,	ARMOR,	FOOD,	RING,	AMULET,		GEM};
	
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
	short i, blessing;
	
	if (itemCategory < 0) {
		itemCategory = pickItemCategory();
	}
	
	theItem->category = itemCategory;
	
	switch (itemCategory) {
		case FOOD:
			if (itemKind < 0) {
				itemKind = chooseKind(foodTable, NUMBER_FOOD_KINDS);
			}
			theItem->displayChar = FOOD_CHAR;
			theItem->flags |= ITEM_IDENTIFIED;
			break;
		case WEAPON:
			if (itemKind < 0) {
				itemKind = chooseKind(weaponTable, NUMBER_WEAPON_KINDS);
			}
			theItem->damage = weaponTable[itemKind].range;
			theItem->strengthRequired = weaponTable[itemKind].strengthRequired;
			theItem->displayChar = WEAPON_CHAR;
			if (rand_percent(32)) {
				blessing = rand_range(1, 3);
				for(i = 0; i < blessing; i++) {
					if (rand_percent(50)) {
						theItem->enchant1 += 1;
					} else {
						theItem->enchant2 += 1;
					}
				}
				if (rand_percent(50)) {
					// cursed
					theItem->enchant1 *= -1;
					theItem->enchant2 *= -1;
					theItem->flags |= ITEM_CURSED;
				}
			}
			if (itemKind == DART || itemKind == SHURIKEN || itemKind == JAVELIN) {
				theItem->quantity = rand_range(5, 20);
				theItem->quiverNumber = rand_range(1, 60000);
				theItem->flags &= ~ITEM_CURSED; // throwing weapons can't be cursed
			}
			theItem->charges = 20;
			break;
		case ARMOR:
			if (itemKind < 0) {
				itemKind = chooseKind(armorTable, NUMBER_ARMOR_KINDS);
			}
			theItem->armor = randClump(armorTable[itemKind].range);
			theItem->strengthRequired = armorTable[itemKind].strengthRequired;
			theItem->displayChar = ARMOR_CHAR;
			if (rand_percent(32)) {
				theItem->enchant1 += rand_range(1, 3);
				if (rand_percent(50)) {
					// cursed
					theItem->enchant1 *= -1;
					theItem->flags |= ITEM_CURSED;
				}
			}
			break;
		case SCROLL:
			if (itemKind < 0) {
				itemKind = chooseKind(scrollTable, NUMBER_SCROLL_KINDS);
			}
			theItem->displayChar = SCROLL_CHAR;
			theItem->flags |= ITEM_FLAMMABLE;
			break;
		case POTION:
			if (itemKind < 0) {
				itemKind = chooseKind(potionTable, NUMBER_POTION_KINDS);
			}
			theItem->displayChar = POTION_CHAR;
			break;
		case STAFF:
			if (itemKind < 0) {
				itemKind = chooseKind(staffTable, NUMBER_STAFF_KINDS);
			}
			theItem->displayChar = STAFF_CHAR;
			theItem->charges = theItem->enchant1 = randClump(staffTable[itemKind].range);
			theItem->enchant2 = (itemKind == STAFF_BLINKING || itemKind == STAFF_OBSTRUCTION ? 1000 : 500); // start with no recharging mojo
			break;
		case WAND:
			if (itemKind < 0) {
				itemKind = chooseKind(wandTable, NUMBER_WAND_KINDS);
			}
			theItem->displayChar = WAND_CHAR;
			theItem->charges = randClump(wandTable[itemKind].range);
			break;
		case RING:
			if (itemKind < 0) {
				itemKind = chooseKind(ringTable, NUMBER_RING_KINDS);
			}
			theItem->displayChar = RING_CHAR;
			theItem->enchant1 = randClump(ringTable[itemKind].range);
			if (rand_percent(16)) {
				// cursed
				theItem->enchant1 *= -1;
				theItem->flags |= ITEM_CURSED;
			}
			break;
		case GOLD:
			theItem->displayChar = GOLD_CHAR;
			theItem->quantity = rand_range(50 + rogue.depthLevel * 10, 100 + rogue.depthLevel * 15);
			break;
		case AMULET:
			theItem->displayChar = AMULET_CHAR;
			itemKind = 0;
			theItem->flags |= ITEM_IDENTIFIED;
			break;
		case GEM:
			theItem->displayChar = GEM_CHAR;
			itemKind = 0;
			theItem->flags |= ITEM_IDENTIFIED;
			break;
		default:
			message("something has gone terribly wrong!", true, true);
			break;
	}
	if (!(theItem->flags & ITEM_IDENTIFIED)) {
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
	short theCategory;
	
	if (rogue.depthLevel > AMULET_LEVEL) {
		numberOfItems = 3;
	} else {
		numberOfItems = 6;
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
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			if (cellHasTerrainFlag(i, j, OBSTRUCTS_ITEMS | TRAP_DESCENT) || pmap[i][j].layers[LIQUID] != NOTHING) {
				itemSpawnHeatMap[i][j] = 0;
			}
			if (itemSpawnHeatMap[i][j] == 50000) {
				itemSpawnHeatMap[i][j] = 0;
				pmap[i][j].layers[DUNGEON] = PERM_WALL;
			}
			totalHeat += itemSpawnHeatMap[i][j];
		}
	}
	
	for (i=0; i<numberOfItems; i++) {
		// guarantee a certain food minimum
		if ((rogue.foodSpawned + 1) * 3 <= rogue.depthLevel) {
			theCategory = FOOD;
			if (rogue.depthLevel > AMULET_LEVEL) {	
				numberOfItems++;
			}
		} else if (rogue.depthLevel > AMULET_LEVEL) {
			theCategory = GEM;
		} else {
			theCategory = -1;
		}
		theItem = generateItem(theCategory, -1);
		if (theItem->category & FOOD) {
			rogue.foodSpawned++;
			randomMatchingLocation(spawnLoc, FLOOR, NOTHING, -1);
			totalHeat -= itemSpawnHeatMap[spawnLoc[0]][spawnLoc[1]];
			itemSpawnHeatMap[spawnLoc[0]][spawnLoc[1]] = 0;
		} else {
			getItemSpawnLoc(itemSpawnHeatMap, spawnLoc, &totalHeat);
		}
		
		// regulate the frequency of enchantment scrolls:
		if (theItem->category & SCROLL && theItem->kind == SCROLL_ENCHANT_ITEM) {
			scrollTable[SCROLL_ENCHANT_ITEM].frequency -= 30; // this means it can go negative!
		} else if (!(theItem->category & GEM)){
			scrollTable[SCROLL_ENCHANT_ITEM].frequency += 2;
		}
		
		placeItem(theItem, spawnLoc[0], spawnLoc[1]); // random valid location
	}
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
	for (theItem = floorItems->nextItem; (theItem->xLoc != x || theItem->yLoc != y) && theItem->nextItem != NULL; theItem = theItem->nextItem);
	
	if (numberOfItemsInPack() < MAX_PACK_ITEMS || theItem->category & GOLD || itemWillStackWithPack(theItem)) {
		
		if (theItem->flags & ITEM_NO_PICKUP) {
			// auto-identify the scroll here
			scrollTable[theItem->kind].identified = true;
			
			nameOfItem(theItem, buf2, true, false); // include suffix but not article
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
		
		nameOfItem(theItem, buf2, true, true); // include suffix, article
		
		sprintf(buf, "you now have %s (%c)", buf2, theItem->inventoryLetter);
		message(buf, true, false);
	} else {
		nameOfItem(theItem, buf2, false, true); // include article
		sprintf(buf, "Your pack is too full to pick up %s.", buf2);
		message(buf, true, false);
	}
}

item *addItemToPack(item *theItem) {
	item *previousItem, *tempItem;
	
	// can the item stack with another in the inventory?
	if (theItem->category & (FOOD|POTION|SCROLL|GEM)) {
		for (tempItem = packItems->nextItem; tempItem != NULL; tempItem = tempItem->nextItem) {
			if (theItem->category == tempItem->category && theItem->kind == tempItem->kind) {
				// we found a match! Increment the quantity of the old item...
				tempItem->quantity++;
				
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
				
				// and delete the new item.
				deleteItem(theItem);
				
				// Pass back the incremented (old) item. No need to add it to the pack since it's already there.
				return tempItem;
			}
		}
	}
	
	// append to pack chain
	theItem->inventoryLetter = nextAvailableInventoryCharacter();
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

void call() {
	item *theItem;
	char itemText[30], buf[COLS];
	
	theItem = promptForItemOfType((SCROLL|RING|POTION|STAFF|WAND), 0, (ITEM_IDENTIFIED),
								  "Call what? (a-z or <esc> to cancel)");
	
	if (theItem == NULL) {
		return;
	}
	
	clearMessage();
	
	if (theItem->flags & ITEM_IDENTIFIED || theItem->category & (WEAPON|ARMOR|FOOD|GOLD|AMULET|GEM)) {
		message("you already know what that is.", true, false);
		return;
	}
	
	if (getInputTextString(itemText, "call it: ", 29)) {
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
		clearMessage();
		nameOfItem(theItem, buf, false, true);
		message(buf, true, false);
	} else {
		clearMessage();
	}
}

void nameOfItem(item *theItem, char *root, boolean includeDetails, boolean includeArticle) {
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
				if (theItem->flags & ITEM_IDENTIFIED) {
					sprintf(buf, "%s%i,%s%i %s", (theItem->enchant1 < 0 ? "" : "+"), theItem->enchant1,
							(theItem->enchant2 < 0 ? "" : "+"), theItem->enchant2, root);
					strcpy(root, buf);
				}
				
				if (theItem->flags & ITEM_VORPALIZED) {
					if (theItem->flags & ITEM_VORPAL_IDENTIFIED) {
						sprintf(root, "%s of %s slaying", root, monsterText[theItem->vorpalEnemy].name);
					} else {
						strcat(root, " (unknown runic)");
					}
				}
				sprintf(root, "%s <%i>", root, theItem->strengthRequired);
			}
			break;
		case ARMOR:
			sprintf(root, "%s", armorTable[theItem->kind].name);
			if (includeDetails) {
				if (theItem->flags & ITEM_IDENTIFIED) {
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
			}
			break;
		case SCROLL:
			if (scrollTable[theItem->kind].identified) {
				sprintf(root, "scroll%s of %s", pluralization, scrollTable[theItem->kind].name);
			} else if (scrollTable[theItem->kind].called) {
				sprintf(root, "scroll%s called %s", pluralization, scrollTable[theItem->kind].callTitle);
			} else {
				sprintf(root, "scroll%s entitled %s", pluralization, scrollTable[theItem->kind].flavor);
			}
			break;
		case POTION:
			if (potionTable[theItem->kind].identified) {
				sprintf(root, "potion%s of %s", pluralization, potionTable[theItem->kind].name);
			} else if (potionTable[theItem->kind].called) {
				sprintf(root, "potion%s called %s", pluralization, potionTable[theItem->kind].callTitle);
			} else {
				sprintf(root, "%s potion%s", potionTable[theItem->kind].flavor, pluralization);
			}
			break;
		case WAND:
			if (wandTable[theItem->kind].identified) {
				sprintf(root, "wand%s of %s", pluralization, wandTable[theItem->kind].name);
			} else if (wandTable[theItem->kind].called) {
				sprintf(root, "wand%s called %s", pluralization, wandTable[theItem->kind].callTitle);
			} else {
				sprintf(root, "%s wand%s", wandTable[theItem->kind].flavor, pluralization);
			}
			if (includeDetails && theItem->flags & ITEM_IDENTIFIED) {
				sprintf(root, "%s [%i]", root, theItem->charges);
			}
			break;
		case STAFF:
			if (staffTable[theItem->kind].identified) {
				sprintf(root, "staff%s of %s", pluralization, staffTable[theItem->kind].name);
			} else if (staffTable[theItem->kind].called) {
				sprintf(root, "staff%s called %s", pluralization, staffTable[theItem->kind].callTitle);
			} else {
				sprintf(root, "%s staff%s", staffTable[theItem->kind].flavor, pluralization);
			}
			if (includeDetails && theItem->flags & ITEM_IDENTIFIED) {
				sprintf(root, "%s [%i/%i]", root, theItem->charges, theItem->enchant1);
			}
			break;
		case RING:
			if (ringTable[theItem->kind].identified) {
				sprintf(root, "ring%s of %s", pluralization, ringTable[theItem->kind].name);
			} else if (ringTable[theItem->kind].called) {
				sprintf(root, "ring%s called %s", pluralization, ringTable[theItem->kind].callTitle);
			} else {
				sprintf(root, "%s ring%s", ringTable[theItem->kind].flavor, pluralization);
			}
			if (includeDetails && theItem->flags & ITEM_IDENTIFIED) {
				sprintf(root, "%s [%i]", root, theItem->enchant1);
			}
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
	
	return;
}

boolean isVowel(char theChar) {
	return (theChar == 'a' || theChar == 'e' || theChar == 'i' || theChar == 'o' || theChar == 'u' ||
			theChar == 'A' || theChar == 'E' || theChar == 'I' || theChar == 'O' || theChar == 'U');
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
	char theKey;
	rogueEvent theEvent;
	color backgroundColor, foregroundColor;
	uchar bufChar;
	
	if (packItems->nextItem == NULL) {
		clearMessage();
		message("Your pack is empty!", true, false);
		return 0;
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
		
		nameOfItem(theItem, buf, true, true); // name of item with suffix, article
		upperCase(buf);
		sprintf(descriptionList[itemNumber], " %c%s x %s%s", theItem->inventoryLetter,
				(theItem->flags & ITEM_PROTECTED ? "}" : ")"), buf,
				(theItem->flags & ITEM_EQUIPPED ? (theItem->category & WEAPON ? " (in hand)" : " (being worn)") : ""));
		maxLength = max(maxLength, (strlen(descriptionList[itemNumber])));
		displayedItemLettersList[itemNumber] = theItem->inventoryLetter;
		itemList[itemNumber] = theItem;
		itemNumber++;
	}
	if (!itemNumber) {
		clearMessage();
		message("Nothing of that type!", true, false);
		return 0;
	}
	if (waitForAcknowledge) {
		itemNameColor[itemNumber] = white;
		sprintf(descriptionList[itemNumber++], " -- press space to continue -- ");
		maxLength = max(maxLength, (strlen(descriptionList[itemNumber - 1])));
	}
	
	for (i=0; i < itemNumber; i++) {
		for (j=0; descriptionList[i][j] != '\0' && j <= DCOLS; j++) {
			getCellAppearance(DCOLS - maxLength + j, i, &bufChar, &foregroundColor, &backgroundColor);
			applyColorAverage(&backgroundColor, &black, 85);
			if (descriptionList[i][j] == ' ') {
				colorBlendCell(DCOLS - maxLength + j, i, &black, 85);
			} else if (j == 4 && (!waitForAcknowledge || i < itemNumber - 1)) { // item character
				plotCharWithColor(itemList[i]->displayChar,
								  DCOLS - maxLength + j + STAT_BAR_WIDTH,
								  i + 1, itemColor[i], backgroundColor);
			} else {
				plotCharWithColor(descriptionList[i][j], DCOLS - maxLength + j + STAT_BAR_WIDTH,
								  i + 1, itemNameColor[i], backgroundColor);
			}
		}
		//printString(descriptionList[i], DCOLS - maxLength, i+1, &white, &black);
		for (j=strlen(descriptionList[i]); j<maxLength; j++) {
			colorBlendCell(DCOLS - maxLength + j, i, &black, 85);
		}
	}
	// updateDisplay();
	if (waitForAcknowledge) {
		while (theKey = nextKeyPress() != ACKNOWLEDGE_KEY);
	} else {
		do {
			nextKeyOrMouseEvent(&theEvent);
		} while (theEvent.eventType != KEYSTROKE && !( theEvent.eventType == MOUSE_UP && theEvent.param1 >= DCOLS - maxLength
													  && theEvent.param2 >= 1 && theEvent.param2 <= itemNumber));
		if (theEvent.eventType == KEYSTROKE) {
			theKey = theEvent.param1;
		} else if (theEvent.eventType == MOUSE_UP) {
			theKey = displayedItemLettersList[theEvent.param2 - 1];
		}
	}
	for (j = 0; j < itemNumber; j++) { // j scans down through the number of items
		for (i = DCOLS - maxLength; i < DCOLS; i++) { // i scans across through the maxlength of the inventory display
			refreshDungeonCell(i, j);
		}
	}
	// updateDisplay();
	return theKey;
}

short numberOfMatchingPackItems(unsigned short categoryMask,
								unsigned long requiredFlags, unsigned long forbiddenFlags,
								boolean displayErrors) {
	item *theItem;
	short matchingItemCount = 0;
	
	if (packItems->nextItem == NULL) {
		if (displayErrors) {
			clearMessage();
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
			clearMessage();
			message("You have nothing suitable.", true, false);
		}
		return 0;
	}
	
	return matchingItemCount;
}

void updateEncumbrance() {
	short moveSpeed, attackSpeed, strengthDeficiency;
	moveSpeed = PLAYER_BASE_MOVEMENT_SPEED;
	attackSpeed = PLAYER_BASE_ATTACK_SPEED;
	
	if (rogue.weapon && rogue.weapon->strengthRequired > rogue.currentStrength) {
		strengthDeficiency = rogue.weapon->strengthRequired - rogue.currentStrength;
		moveSpeed += 25 * strengthDeficiency;
		attackSpeed += 25 * strengthDeficiency;
	}
	
	if (rogue.armor && rogue.armor->strengthRequired > rogue.currentStrength) {
		strengthDeficiency = rogue.armor->strengthRequired - rogue.currentStrength;
		moveSpeed += 25 * strengthDeficiency;
		attackSpeed += 25 * strengthDeficiency;
	}
	
	if (player.status.hasted) {
		moveSpeed /= 2;
		attackSpeed /= 2;
	} else if (player.status.slowed) {
		moveSpeed *= 2;
		attackSpeed *= 2;
	}
	
	//player.info.movementSpeed = moveSpeed;
	//player.info.attackSpeed = attackSpeed;
}

void strengthCheck(item *theItem) {
	char buf1[COLS], buf2[COLS];
	short strengthDeficiency;
	
	updateEncumbrance();
	if (theItem) {
		if (theItem->category & WEAPON && theItem->strengthRequired > rogue.currentStrength) {
			strengthDeficiency = theItem->strengthRequired - rogue.currentStrength;
			strcpy(buf1, "");
			nameOfItem(theItem, buf1, false, false);
			sprintf(buf2, "You can barely lift the %s; %i more strength would be ideal.", buf1, strengthDeficiency);
			message(buf2, true, false);
		}
		
		if (theItem->category & ARMOR && theItem->strengthRequired > rogue.currentStrength) {
			strengthDeficiency = theItem->strengthRequired - rogue.currentStrength;
			strcpy(buf1, "");
			nameOfItem(theItem, buf1, false, false);
			sprintf(buf2, "You stagger under the weight of the %s; %i more strength would be ideal.",
					buf1, strengthDeficiency);
			message(buf2, true, false);
		}
	}
}

void equip() {
	char buf1[COLS], buf2[COLS];
	item *theItem;
	
	theItem = promptForItemOfType((WEAPON|ARMOR|RING), 0, ITEM_EQUIPPED, "Equip what? (a-z or <esc> to cancel)");
	
	if (theItem == NULL) {
		return;
	}
	if (theItem->category & (WEAPON|ARMOR|RING)) {
		
		if (theItem->category & RING) {
			if (theItem->flags & ITEM_EQUIPPED) {
				clearMessage();
				message("you are already wearing that ring.", true, false);
				return;
			} else if (rogue.ringLeft && rogue.ringRight) {
				clearMessage();
				message("You are already wearing two rings; remove one first.", true, false);
				return;
			}
		}
		
		if (!equipItem(theItem, false)) { // this is where the item gets equipped
			// equip failed because current item is cursed
			if (theItem->category & WEAPON) {
				nameOfItem(rogue.weapon, buf1, false, false);
			} else if (theItem->category & ARMOR) {
				nameOfItem(rogue.armor, buf1, false, false);
			} else {
				sprintf(buf1, "one");
			}
			sprintf(buf2, "You can't; the %s you are using appears to be cursed.", buf1);
			clearMessage();
			message(buf2, true, false);
			return;
		}	
		nameOfItem(theItem, buf2, true, true);
		sprintf(buf1, "Now %s %s.", (theItem->category & WEAPON ? "wielding" : "wearing"), buf2);
		clearMessage();
		message(buf1, true, false);
		
		strengthCheck(theItem);
		
		if (theItem->flags & ITEM_CURSED) {
			nameOfItem(theItem, buf2, false, false);
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
		
		playerTurnEnded();
		return;
	} else {
		clearMessage();
		message("You can't equip that.", true, false);
		return;
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

// returns the number of items on the list
short getLineCoordinates(short listOfCoordinates[DCOLS][2], short originLoc[2], short targetLoc[2]) {
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
		
	} while ((!(pmap[currentLoc[0]][currentLoc[1]].flags & HAS_MONSTER) || !canSeeMonster(monst))
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
	
	for (layer = 0; layer < NUMBER_TERRAIN_LAYERS; layer++) {
		if (tileCatalog[pmap[x][y].layers[layer]].flags & (OBSTRUCTS_PASSABILITY | OBSTRUCTS_VISION)) {
			pmap[x][y].layers[layer] = (layer == DUNGEON ? FLOOR : NOTHING);
			didSomething = true;
		}
	}
	if (didSomething) {
		spawnDungeonFeature(x, y, &dungeonFeatureCatalog[DF_RUBBLE], true);
	}
	return didSomething;
}

// returns whether the bolt effect should autoID any staff or wand it came from, if it came from a staff or wand
boolean zap(short originLoc[2], short targetLoc[2], enum boltType bolt, short boltLevel, boolean hideDetails) {
	short listOfCoordinates[DCOLS][2];
	short i, j, k, x, y, numCells, blinkDistance;
	float healthFraction;
	short previousExperience;
	creature *monst = NULL, *blinkingMonst;
	char buf[COLS], monstName[COLS];
	boolean autoID = false;
	boolean displayChanged = false;
	boolean fastForward = false;
	color *boltColor;
	dungeonFeature feat;
	enum directions dir;
	
	updateDisplay(); // so that bolts don't shoot at the place where the monster/player is about to be
	
	numCells = getLineCoordinates(listOfCoordinates, originLoc, targetLoc);
	
	blinkingMonst = monsterAtLoc(originLoc[0], originLoc[1]);
	
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
		updateCoordinate(originLoc[0] + STAT_BAR_WIDTH, originLoc[1] + 1);
		blinkDistance = boltLevel * 2 + 1;
	}
	if (bolt == BOLT_TUNNELING) {
		tunnelize(originLoc[0], originLoc[1]);
	}
	
	for (i=0; i<numCells; i++) {
		
		x = listOfCoordinates[i][0];
		y = listOfCoordinates[i][1];
		
		monst = monsterAtLoc(x, y);
		
		for (k = 0; k <= min(i, boltLevel + 2); k++) {
			if (i % 2 != 0) {
				displayChanged = false;
			}
			if (pmap[listOfCoordinates[i-k][0]][listOfCoordinates[i-k][1]].flags & (IN_FIELD_OF_VIEW | CLAIRVOYANT_VISIBLE)) {
				displayChanged = true;
				hiliteCell(listOfCoordinates[i-k][0], listOfCoordinates[i-k][1], boltColor, max(0, 100 - k * 100 / (boltLevel)));
				updateCoordinate(listOfCoordinates[i-k][0] + STAT_BAR_WIDTH, listOfCoordinates[i-k][1] + 1);
			}
		}
		
		if (!fastForward && i % 2 == 0 && displayChanged) {
			fastForward = pauseForMilliseconds(1);
		}
		
		if (bolt == BOLT_FIRE) {
			if (exposeTileToFire(x, y, true)) {
				updateVision();
				updateDisplay();
				autoID = true;
			}
		}
		
		if (bolt == BOLT_BLINKING) {
			boltLevel = (blinkDistance - i) / 2 + 1;
			if (i >= blinkDistance) {
				break;
			}
		}
		
		// some bolts halt the square before they hit something
		if ((bolt == BOLT_BLINKING || bolt == BOLT_OBSTRUCTION)
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
		
		if (pmap[x][y].flags & (HAS_PLAYER | HAS_MONSTER) && (bolt == BOLT_LIGHTNING) && (!monst || !(monst->bookkeepingFlags & MONST_SUBMERGED))) {
			monsterName(monstName, monst, true);
			
			autoID = true;
			
			if (monst->status.magicalFear) {
				monst->status.magicalFear = 1;
			}
			monst->status.entranced = 0;
			
			if (inflictDamage(blinkingMonst, monst, randClumpedRange(2 * (2 + boltLevel) / 3, 4 + 2 * boltLevel, 1 + boltLevel / 3))) {
				// killed monster
				if (player.currentHP <= 0) {
					return false;
				}
				sprintf(buf, "the lightning bolt kills %s", monstName);
				combatMessage(buf);
				free(monst);
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
			}
		}
		
		if (cellHasTerrainFlag(x, y, (OBSTRUCTS_PASSABILITY | OBSTRUCTS_VISION))
			|| ((pmap[x][y].flags & HAS_PLAYER || (pmap[x][y].flags & (HAS_MONSTER) && !(monst->bookkeepingFlags & MONST_SUBMERGED)))
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
				updateDisplay();
				autoID = true;
				if (!--boltLevel) {
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
		pmap[x][y].flags |= (blinkingMonst == &player ? HAS_PLAYER : HAS_MONSTER);
		blinkingMonst->xLoc = x;
		blinkingMonst->yLoc = y;
		updateVision();
		updateDisplay();
		autoID = true;
	}
	
	if (boltLevel > 0) {
		for (j=i; j < i + boltLevel + 2; j++) { // note that j can imply a bolt position off the tmap
			
			for (k = j-i; k <= min(j, boltLevel + 2); k++) {
				if (j % 2 != 0) {
					displayChanged = false;
				}
				if (pmap[listOfCoordinates[j-k][0]][listOfCoordinates[j-k][1]].flags & (IN_FIELD_OF_VIEW | CLAIRVOYANT_VISIBLE)) {
					displayChanged = true;
					hiliteCell(listOfCoordinates[j-k][0], listOfCoordinates[j-k][1], boltColor, max(0, 100 - k * 100 / (boltLevel)));
					updateCoordinate(listOfCoordinates[j-k][0] + STAT_BAR_WIDTH, listOfCoordinates[j-k][1] + 1);
				}
			}
			if (!fastForward && j % 2 == 0 && displayChanged) {
				fastForward = pauseForMilliseconds(1);
			}
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
				teleport(monst);
			}
			break;
		case BOLT_SLOW:
			if (monst && !(monst->info.flags & MONST_INANIMATE)) {
				monst->status.slowed = monst->maxStatus.slowed = boltLevel;
				monst->status.hasted = 0;
				monst->movementSpeed = monst->info.movementSpeed * 2;
				monst->attackSpeed = monst->info.attackSpeed * 2;
				autoID = true;
				if (monst == &player) {
					message("you feel yourself slow down.", true, false);
				}
			}
			break;
		case BOLT_HASTE:
			if (monst && !(monst->info.flags & MONST_INANIMATE)) {
				monst->status.slowed = 0;
				monst->status.hasted = monst->maxStatus.hasted = boltLevel;
				monst->movementSpeed = monst->info.movementSpeed / 2;
				monst->attackSpeed = monst->info.attackSpeed / 2;
				autoID = true;
				if (monst == &player) {
					message("you feel yourself speed up.", true, false);
				}
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
				
				refreshDungeonCell(monst->xLoc, monst->yLoc);
				if (!(monst->info.flags & MONST_INVISIBLE)) {
					autoID = true;
				}
			}
			break;
		case BOLT_INVISIBILITY:
			if (monst && monst != &player && !(monst->info.flags & MONST_INANIMATE)) {
				monst->info.flags |= MONST_INVISIBLE;
				refreshDungeonCell(monst->xLoc, monst->yLoc);
			}
			break;
		case BOLT_CANCELLATION:
			if (monst) { // works on inanimates
				monst->info.abilityFlags = 0; // canceled monsters lose all special abilities
				monst->status.immuneToFire = 0;
				monst->status.slowed = 0;
				monst->status.hasted = 0;
				monst->status.entranced = 0;
				monst->status.telepathic = min(monst->status.telepathic, 1);
				monst->status.magicalFear = min(monst->status.magicalFear, 1);
				monst->status.levitating = min(monst->status.levitating, 1);
				monst->info.flags &= ~MONST_IMMUNE_TO_FIRE;
				monst->movementSpeed = monst->info.movementSpeed;
				monst->attackSpeed = monst->info.attackSpeed;
				if (monst != &player && monst->info.flags & CANCELLABLE_TRAITS) {
					if (monst->info.flags & MONST_FIERY && monst->status.burning) {
						extinguishFireOnCreature(monst);
					}
					monst->info.flags &= ~CANCELLABLE_TRAITS;
					refreshDungeonCell(monst->xLoc, monst->yLoc);
					refreshSideBar(NULL);
					applyTileEffectToCreature(monst); // in case it should immediately die or fall into a chasm
				}
			}
			break;
		case BOLT_LIGHTNING:
			// already handled above
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
				} else if (inflictDamage(blinkingMonst, monst, randClumpedRange(3 * (2 + boltLevel) / 4, 4 + 5 * boltLevel / 2, 1 + boltLevel / 3))) {
					// killed creature
					
					if (player.currentHP <= 0) {
						return false;
					}
					
					sprintf(buf, "the firebolt kills %s", monstName);
					combatMessage(buf);
					free(monst);
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
				}
			}
			exposeTileToFire(x, y, true); // burninate
			break;
		case BOLT_BLINKING:
			if (blinkingMonst == &player) {
				// handled above for visual effect (i.e. before contrail fades)
				// increase scent turn number so monsters don't sniff around at the old cell like idiots
				rogue.scentTurnNumber += 300;
				// get any items at the destination location
				if (!player.status.levitating && pmap[player.xLoc][player.yLoc].flags & HAS_ITEM) {
					pickUpItemAt(player.xLoc, player.yLoc);
				}
			}
			break;
		case BOLT_ENTRANCEMENT:
			if (monst && !(monst->info.flags & MONST_INANIMATE)) {
				monst->status.entranced = monst->maxStatus.entranced = 20 * (40 * (boltLevel - 3) / 7 + 20) / monst->info.maxHP;
				wakeUp(monst);
				if (canSeeMonster(monst)) {
					autoID = true;
					sprintf(buf, "%s is entranced!", monstName);
					message(buf, true, false);
				}
			}
			break;
		case BOLT_HEALING:
			if (monst && !(monst->info.flags & MONST_INANIMATE)) {
				monst->currentHP = min(monst->info.maxHP, monst->currentHP + boltLevel * monst->info.maxHP / 10);
				if (canSeeMonster(monst)) {
					autoID = true;
					sprintf(buf, "%s looks healthier!", monstName);
					message(buf, true, false);
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
	}
	
	return autoID;
}

creature *nextTargetAfter(short targetX, short targetY, boolean targetAllies) {
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
			&& openPathBetween(player.xLoc, player.yLoc, monst->xLoc, monst->yLoc)) {
			
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
					   && openPathBetween(player.xLoc, player.yLoc, monst->xLoc, monst->yLoc)) {
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
				&& openPathBetween(player.xLoc, player.yLoc, monst->xLoc, monst->yLoc)) {
				// potentially this one
				shortestDistance = distanceBetween(player.xLoc, player.yLoc, monst->xLoc, monst->yLoc);
				returnMonst = monst;
			}
		}
	}
	
	if (returnMonst) {
	plotCharWithColor(returnMonst->info.displayChar, returnMonst->xLoc + STAT_BAR_WIDTH,
					  returnMonst->yLoc + 1, *(returnMonst->info.foreColor), white);
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
				   && (pmap[x][y].flags & (VISIBLE | CLAIRVOYANT_VISIBLE) || player.status.telepathic)) {
			monst = monsterAtLoc(x, y);
			if (!(monst->bookkeepingFlags & MONST_SUBMERGED)
				&& (!(monst->info.flags & MONST_INVISIBLE) || player.status.telepathic)) {
				break;
			}
		}
		
	}
}

boolean chooseTarget(short returnLoc[2], short maxDistance, boolean stopAtTarget, boolean autoTarget, boolean targetAllies, boolean passThroughCreatures) {
	uchar keystroke;
	short originLoc[2], targetLoc[2], oldTargetLoc[2], coordinates[DCOLS][2], numCells;
	creature *monst;
	rogueEvent theEvent;
	boolean acceptableInput, targetConfirmed, focusedOnMonster = false;
	
	originLoc[0] = player.xLoc;
	originLoc[1] = player.yLoc;
	
	targetLoc[0] = player.xLoc;
	targetLoc[1] = player.yLoc;
	
	if (autoTarget) {
		monst = nextTargetAfter(targetLoc[0], targetLoc[1], targetAllies);
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
		targetConfirmed = false;
		do {
			oldTargetLoc[0] = targetLoc[0];
			oldTargetLoc[1] = targetLoc[1];
			acceptableInput = false;
			nextKeyOrMouseEvent(&theEvent);
			if (theEvent.eventType == MOUSE_UP
				&& coordinatesAreInMap(theEvent.param1 - STAT_BAR_WIDTH, theEvent.param2 - 1)) {
				
				if (theEvent.controlKey
					|| (targetLoc[0] == theEvent.param1 - STAT_BAR_WIDTH
						&& targetLoc[1] == theEvent.param2 - 1)) {
					targetConfirmed = true;
				}
				targetLoc[0] = theEvent.param1 - STAT_BAR_WIDTH;
				targetLoc[1] = theEvent.param2 - 1;
				acceptableInput = true;
				break;
			} else if (theEvent.eventType == KEYSTROKE) {
				keystroke = theEvent.param1;
				switch(keystroke) {
					case LEFT_ARROW:
					case LEFT_KEY:
						if (targetLoc[0] > 0) {
							targetLoc[0]--;
							acceptableInput = true;
						}
						break;
					case RIGHT_ARROW:
					case RIGHT_KEY:
						if (targetLoc[0] < DCOLS - 1) {
							targetLoc[0]++;
							acceptableInput = true;
						}
						break;
					case UP_ARROW:
					case UP_KEY:
						if (targetLoc[1] > 0) {
							targetLoc[1]--;
							acceptableInput = true;
						}
						break;
					case DOWN_ARROW:
					case DOWN_KEY:
						if (targetLoc[1] < DROWS - 1) {
							targetLoc[1]++;
							acceptableInput = true;
						}
						break;
					case UPLEFT_KEY:
						if (targetLoc[0] > 0 && targetLoc[1] > 0) {
							targetLoc[0]--;
							targetLoc[1]--;
							acceptableInput = true;
						}
						break;
					case UPRIGHT_KEY:
						if (targetLoc[0] < DCOLS - 1 && targetLoc[1] > 0) {
							targetLoc[0]++;
							targetLoc[1]--;
							acceptableInput = true;
						}
						break;
					case DOWNLEFT_KEY:
						if (targetLoc[0] > 0 && targetLoc[1] < DROWS - 1) {
							targetLoc[0]--;
							targetLoc[1]++;
							acceptableInput = true;
						}
						break;
					case DOWNRIGHT_KEY:
						if (targetLoc[0] < DCOLS - 1 && targetLoc[1] < DROWS - 1) {
							targetLoc[0]++;
							targetLoc[1]++;
							acceptableInput = true;
						}
						break;
					case TAB_KEY:
						monst = nextTargetAfter(targetLoc[0], targetLoc[1], targetAllies);
						if (monst) {
							targetLoc[0] = monst->xLoc;
							targetLoc[1] = monst->yLoc;
							acceptableInput = true;
						}
						break;
					case RETURN_KEY:
						targetConfirmed = true;
						break;
					case ESCAPE_KEY:
					case ACKNOWLEDGE_KEY:
						refreshDungeonCell(oldTargetLoc[0], oldTargetLoc[1]);
						hiliteTrajectory(coordinates, numCells, true, passThroughCreatures);
						updateDisplay();
						clearMessage();
						return false;
					default:
						break;
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
		} while (!acceptableInput && !targetConfirmed);
		
		refreshDungeonCell(oldTargetLoc[0], oldTargetLoc[1]);
		hiliteTrajectory(coordinates, numCells, true, passThroughCreatures);
		updateDisplay();
		
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
		clearMessage();
		return false;
	}
	
	returnLoc[0] = targetLoc[0];
	returnLoc[1] = targetLoc[1];
	return true;
}

// returns whether the item disappeared
boolean hitMonsterWithProjectileWeapon(creature *thrower, creature *monst, item *theItem) {
	char buf[DCOLS], itemName[DCOLS], targetName[DCOLS];
	boolean thrownWeaponHit, reportProceedings;
	item *equippedWeapon;
	short damage;
	
	if (!(theItem->category & WEAPON)) {
		return false;
	}
	
	if (pmap[monst->xLoc][monst->yLoc].flags & VISIBLE
		|| pmap[thrower->xLoc][thrower->yLoc].flags & VISIBLE) {
		reportProceedings = true;
		nameOfItem(theItem, itemName, false, false);
		if (monst) {
			monsterName(targetName, monst, true);
		}
	} else {
		reportProceedings = false;
	}
	
	monst->status.entranced = 0;
	
	if (monst != &player
		&& monst->creatureMode != MODE_PERM_FLEEING
		&& (monst->creatureState != MONSTER_FLEEING || monst->status.magicalFear)) {
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
		damage = (monst->info.flags & MONST_IMMUNE_TO_WEAPONS ? 0 : randClump(theItem->damage) + theItem->enchant2);
		if (inflictDamage(thrower, monst, damage)) { // monster killed
			sprintf(buf, "the %s killed %s.", itemName, targetName);
			free(monst);
		} else {
			sprintf(buf, "the %s hit %s.", itemName, targetName);
		}
		message(buf, true, false);
		return true;
	} else {
		sprintf(buf, "the %s missed %s", itemName, targetName);
		message(buf, true, false);
		return false;
	}
}

void throwItem(item *theItem, creature *thrower, short targetLoc[2], short maxDistance) {
	short listOfCoordinates[DCOLS][2], originLoc[2];
	short i, x, y, numCells;
	creature *monst = NULL;
	char buf[COLS], buf2[COLS], buf3[COLS];
	uchar displayChar;
	color foreColor, backColor, multColor;
	short dropLoc[2];
	boolean hitSomethingSolid = false, fastForward = false;
	
	originLoc[0] = thrower->xLoc;
	originLoc[1] = thrower->yLoc;
	
	numCells = getLineCoordinates(listOfCoordinates, originLoc, targetLoc);
	
	thrower->ticksUntilTurn = thrower->attackSpeed;
	
	if (thrower != &player && pmap[originLoc[0]][originLoc[1]].flags & IN_FIELD_OF_VIEW) {
		monsterName(buf2, thrower, true);
		nameOfItem(theItem, buf3, false, true);
		sprintf(buf, "%s hurls %s.", buf2, buf3);
		message(buf, true, false);
	}
	
	for (i=0; i<numCells && i <= maxDistance; i++) {
		
		x = listOfCoordinates[i][0];
		y = listOfCoordinates[i][1];
		
		if (pmap[x][y].flags & (HAS_MONSTER | HAS_PLAYER)) {
			monst = monsterAtLoc(x, y);
			
			if (theItem->category & WEAPON && hitMonsterWithProjectileWeapon(thrower, monst, theItem)) {
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
		
		if (pmap[x][y].flags & (VISIBLE | CLAIRVOYANT_VISIBLE)) { // show the graphic
			getCellAppearance(x, y, &displayChar, &foreColor, &backColor);
			foreColor = *(theItem->foreColor);
			colorMultiplierFromDungeonLight(x, y, &multColor);
			applyColorMultiplier(&foreColor, &multColor);
			plotCharWithColor(theItem->displayChar, x + STAT_BAR_WIDTH, y + 1, foreColor, backColor);
			updateCoordinate(x + STAT_BAR_WIDTH, y + 1);
			
			if (!fastForward) {
				fastForward = pauseForMilliseconds(1);
			}
			
			refreshDungeonCell(x, y);
			updateCoordinate(x + STAT_BAR_WIDTH, y + 1);
		}
		
		if (x == targetLoc[0] && y == targetLoc[1]) { // reached its target
			break;
		}
	}	
	
	if (theItem->category & POTION && (hitSomethingSolid || !cellHasTerrainFlag(x, y, TRAP_DESCENT))) {
		if (theItem->kind == POTION_CONFUSION || theItem->kind == POTION_POISON
			|| theItem->kind == POTION_PARALYSIS || theItem->kind == POTION_INCINERATION) {
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
					strcpy(buf, "the flask shatters and its contents burst violently into flame!");
					message(buf, true, false);
					spawnDungeonFeature(x, y, &dungeonFeatureCatalog[DF_FLAMETHROWER], true);
					break;
			}
			
			identify(theItem);
			
			refreshDungeonCell(x, y);
			
			if (pmap[x][y].flags & (HAS_MONSTER | HAS_PLAYER)) {
				monst = monsterAtLoc(x, y);
				applyTileEffectToCreature(monst);
			}
		} else {
			sprintf(buf, "the flask shatters and %s liquid splashes harmlessly on the ground.",
					potionTable[theItem->kind].flavor);
			message(buf, true, false);
		}
		return; // potions disappear when they break
	}
	getQualifyingLocNear(dropLoc, x, y, DCOLS, (OBSTRUCTS_ITEMS), (HAS_ITEM), false);
	placeItem(theItem, dropLoc[0], dropLoc[1]);
	refreshDungeonCell(dropLoc[0], dropLoc[1]);
}

void throw() {
	item *theItem, *previousItem, *thrownItem;
	char buf[COLS], buf2[COLS];
	short maxDistance, zapTarget[2], originLoc[2];
	
	theItem = promptForItemOfType((ALL_ITEMS), 0, 0, "Throw what? (a-z or <esc> to cancel)");
	
	if (theItem == NULL) {
		return;
	}
	clearMessage();
	
	if (theItem->flags & ITEM_EQUIPPED) {
		nameOfItem(theItem, buf2, false, false);
		sprintf(buf, "Are you sure you want to throw your %s? (Y/N)", buf2);
		if (!confirm(buf)) {
			return;
		}
		if (!unequipItem(theItem, false)) {
			sprintf(buf, "You cannot unequip your %s; it appears to be cursed.", buf2);
			message(buf, true, false);
			return;
		}
	}
	
	message("Direction? (<hjklyubn>, mouse, or <tab>; <return> to confirm)", true, false);
	maxDistance = (rogue.currentStrength + 2 * max(rogue.currentStrength - 12, 0));
	if (chooseTarget(zapTarget, maxDistance, true, true, false, false)) {
		clearMessage();
		
		thrownItem = generateItem(-1, -1);
		*thrownItem = *theItem; // clone the item
		thrownItem->quantity = 1;
		
		nameOfItem(thrownItem, buf2, false, false);
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
	playerTurnEnded();
}

void apply(item *theItem) {
	item *previousItem;
	char buf[COLS], buf2[COLS];
	short zapTarget[2], originLoc[2], maxDistance;
	boolean autoTarget, targetAllies, autoID, passThroughCreatures;
	
	if (!theItem) {
		theItem = promptForItemOfType((SCROLL|FOOD|POTION|STAFF|WAND), 0, 0, "Apply what? (a-z or <esc> to cancel)");
	}
	
	if (theItem == NULL) {
		return;
	}
	clearMessage();
	switch (theItem->category) {
		case FOOD:
			player.status.nutrition = min(foodTable[theItem->kind].strengthRequired + player.status.nutrition, STOMACH_SIZE);
			if (theItem->kind == RATION) {
				message("That food tasted delicious!", true, false);
			} else {
				message("My, what a yummy mango!", true, false);
			}
			// updateStatBar(HUNGER_STAT);
			break;
		case POTION:
			potionTable[theItem->kind].identified = true;
			drinkPotion(theItem);
			break;
		case SCROLL:
			scrollTable[theItem->kind].identified = true;
			readScroll(theItem);
			break;
		case STAFF:
		case WAND:
			if (theItem->charges <= 0 && theItem->flags & ITEM_IDENTIFIED) {
				nameOfItem(theItem, buf2, false, false);
				sprintf(buf, "Your %s has no charges.", buf2);
				message(buf, true, false);
				return;
			}
			message("Direction? (<hjklyubn>, mouse, or <tab>; <return> to confirm)", true, false);
			if (theItem->category & STAFF && theItem->kind == STAFF_BLINKING && theItem->flags & ITEM_IDENTIFIED) {
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
				(theItem->kind == STAFF_HEALING))
				|| (theItem->category & WAND && staffTable[theItem->kind].identified &&
					(theItem->kind == WAND_INVISIBILITY || theItem->kind == WAND_HASTE))) {
				targetAllies = true;
			}
			passThroughCreatures = false;
			if (theItem->category & STAFF && staffTable[theItem->kind].identified &&
				theItem->kind == STAFF_LIGHTNING) {
				passThroughCreatures = true;
			}
			if (chooseTarget(zapTarget, maxDistance, false, autoTarget, targetAllies, passThroughCreatures)) {
				clearMessage();
				
				originLoc[0] = player.xLoc;
				originLoc[1] = player.yLoc;
				
				if (theItem->charges > 0) {
					autoID = zap(originLoc, zapTarget,
						(theItem->kind + (theItem->category == STAFF ? NUMBER_WAND_KINDS : 0)),		// bolt type
						(theItem->category == STAFF ? theItem->enchant1 : 10),						// bolt level
						!(theItem->category & WAND && wandTable[theItem->kind].identified
						 || theItem->category & STAFF && staffTable[theItem->kind].identified));	// hide bolt details
					if (autoID) {
						if (theItem->category & STAFF) {
							staffTable[theItem->kind].identified = true;
						} else {
							wandTable[theItem->kind].identified = true;
						}
					}
				} else {
					nameOfItem(theItem, buf2, false, false);
					sprintf(buf, "Your %s fizzles; it must be out of charges.", buf2);
					message(buf, true, false);
					playerTurnEnded();
					return;
				}
			} else {
				// updateDisplay();
				return;
			}
			break;
		default:
			nameOfItem(theItem, buf2, false, true);
			sprintf(buf, "you can't apply %s.", buf2);
			message(buf, true, false);
			return;
	}
	
	if (theItem->charges > 0) {
		theItem->charges--;
	} else if (theItem->quantity > 1) {
		theItem->quantity--;
	} else {
		for (previousItem = packItems; previousItem->nextItem != theItem; previousItem = previousItem->nextItem);
		previousItem->nextItem = theItem->nextItem;
		free(theItem);
	}
	playerTurnEnded();
}

void identify(item *theItem) {
	itemTable *theTable;
	short tableCount = 0, i, lastItem = -1;
	
	theItem->flags |= ITEM_IDENTIFIED;
	theItem->flags &= ~ITEM_CAN_BE_IDENTIFIED;
	if (theItem->flags & ITEM_VORPALIZED) {
		theItem->flags |= (ITEM_VORPAL_IDENTIFIED | ITEM_VORPAL_HINTED);
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
	
	// TODO: this part doesn't work for some reason.
	if (tableCount) {
		theTable[theItem->kind].identified = true;
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
	short i, index, possCount = 0, deepestLevel = 0, deepestHorde, chosenHorde;
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
		return hordeCatalog[chosenHorde].leaderType;
	} else {
		return hordeCatalog[chosenHorde].memberType[index];
	}	
}

void readScroll(item *theItem) {
	short i, j, x, y, numberOfMonsters = 0;
	item *tempItem;
	creature *monst;
	boolean hadEffect = false;
	char buf[COLS], buf2[COLS];
	
	switch (theItem->kind) {
		case SCROLL_IDENTIFY:
			message("this is a scroll of identify.", true, false);
			if (numberOfMatchingPackItems(ALL_ITEMS, ITEM_CAN_BE_IDENTIFIED, 0, false) == 0) {
				message("everything in your pack is already identified.", true, false);
				break;
			}
			do {
				theItem = promptForItemOfType((ALL_ITEMS), ITEM_CAN_BE_IDENTIFIED, 0, "Identify what? (a-z)");
			} while (theItem == NULL);
			clearMessage();
			identify(theItem);
			nameOfItem(theItem, buf, true, true);
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
			message("this is a scroll of enchantment.", true, false);
			if (!numberOfMatchingPackItems(WEAPON | ARMOR | RING | STAFF | WAND, 0, 0, false)) {
				clearMessage();
				message("you have nothing that can be enchanted.", true, false);
				break;
			}
			do {
				theItem = promptForItemOfType((WEAPON | ARMOR | RING | STAFF | WAND), 0, 0, "Enchant what? (a-z)");
				clearMessage();
				if (theItem == NULL || !(theItem->category & (WEAPON | ARMOR | RING | STAFF | WAND))) {
					message("Can't enchant that.", true, false);
				}
			} while (theItem == NULL || !(theItem->category & (WEAPON | ARMOR | RING | STAFF | WAND)));
			clearMessage();
			switch (theItem->category) {
				case WEAPON:
					theItem->strengthRequired = max(0, theItem->strengthRequired - 1);
					if (rand_range(0, 1)) {
						theItem->enchant1++;
					} else {
						theItem->enchant2++;
					}
					break;
				case ARMOR:
					theItem->strengthRequired = max(0, theItem->strengthRequired - 1);
					theItem->enchant1++;
					break;
				case RING:
					theItem->enchant1++;
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
			nameOfItem(theItem, buf, false, false);
			sprintf(buf2, "your %s shines with an inner light.", buf);
			message(buf2, true, false);
			if (theItem->flags & ITEM_CURSED) {
				sprintf(buf2, "a malevolent force leaves your %s.", buf);
				message(buf2, true, false);				
				theItem->flags &= ~ITEM_CURSED;
			}
			break;
		case SCROLL_PROTECT_ARMOR:
			if (tempItem = rogue.armor) {
				tempItem->flags |= ITEM_PROTECTED;
				message("a protective golden light covers your armor.", true, false);
				if (tempItem->flags & ITEM_CURSED) {
					nameOfItem(tempItem, buf, false, false);
					sprintf(buf2, "a malevolent force leaves your %s.", buf);
					message(buf2, true, false);				
					tempItem->flags &= ~ITEM_CURSED;
				}
			} else {
				message("a protective golden light surrounds you, but it quickly disperses.", true, false);
			}
			break;
		case SCROLL_PROTECT_WEAPON:
			if (tempItem = rogue.weapon) {
				tempItem->flags |= ITEM_PROTECTED;
				message("a protective golden light covers your weapon.", true, false);
				if (tempItem->flags & ITEM_CURSED) {
					nameOfItem(tempItem, buf, false, false);
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
					if (!(pmap[i][j].flags & DISCOVERED) && pmap[i][j].layers[DUNGEON] != GRANITE) {
						pmap[i][j].flags |= MAGIC_MAPPED;
						refreshDungeonCell(i, j);
					}
					if (cellHasTerrainFlag(i, j, IS_SECRET)) {
						discover(i, j);
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
		case SCROLL_VORPALIZE_WEAPON:
			if (rogue.weapon && rogue.weapon->category & WEAPON) {
				nameOfItem(rogue.weapon, buf2, false, false);
				if (rogue.weapon->flags & ITEM_VORPALIZED) {
					sprintf(buf, "your %s flex%s under intense strain, but nothing happens.",
							buf2, (rogue.weapon->quantity > 1 ? "" : "es"));
					message(buf, true, false);
				} else {
					sprintf(buf, "your %s gleam%s with arcane runes!", buf2, (rogue.weapon->quantity > 1 ? "" : "s"));
					message(buf, true, false);
					rogue.weapon->flags |= (ITEM_VORPALIZED | ITEM_CAN_BE_IDENTIFIED);
					if (theItem->flags & ITEM_CURSED) {
						sprintf(buf, "a malevolent force leaves your %s.", buf2);
						message(buf, true, false);				
						theItem->flags &= ~ITEM_CURSED;
					}
					rogue.weapon->enchant1++;
					rogue.weapon->enchant2++;
					rogue.weapon->vorpalEnemy = chooseVorpalEnemy();
					equipItem(rogue.weapon, false);
				}
			} else {
				message("white-hot energy surrounds your hands, but it gradually disperses.", true, false);
			}
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
					&& monst->creatureState != MONSTER_FLEEING && monst->creatureState != MONSTER_COWERING
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
	
	switch (theItem->kind) {
		case POTION_HEALING:
			if (player.status.blind > 3) {
				player.status.blind /= 3;
			}
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
			if (player.status.blind > 1) {
				player.status.blind = 1;
			}
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
			message("as you uncork the flask, it explodes in flame!", true, false);
			spawnDungeonFeature(player.xLoc, player.yLoc, &dungeonFeatureCatalog[DF_FLAMETHROWER], true);
			exposeCreatureToFire(&player);
			break;
//		case POTION_BLINDNESS:
//			player.status.blind = player.maxStatus.blind = 150;
//			goBlind();
//			message("a cloak of darkness falls around you.", true, false);
//			break;
		case POTION_GAIN_LEVEL:
			addExperience(levelPoints[rogue.experienceLevel - 1] - rogue.experience);
			break;
		case POTION_GAIN_STRENGTH:
			rogue.currentStrength++;
			rogue.maxStrength = max(rogue.currentStrength, rogue.maxStrength);
			// updateStatBar(STR_CURRENT_STAT | STR_MAX_STAT);
			message("newfound strength surges through your body.", true, false);
			break;
		case POTION_RESTORE_STRENGTH:
			rogue.currentStrength = rogue.maxStrength;
			// updateStatBar(STR_CURRENT_STAT);
			message("you feel warm all over as strength floods back to your muscles.", true, false);
			break;
		case POTION_WEAKNESS:
			rogue.currentStrength -= rand_range(1, 2);
			// updateStatBar(STR_CURRENT_STAT);
			message("you suddenly feel weaker.", true, false);
			strengthCheck(rogue.weapon);
			strengthCheck(rogue.armor);
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
		case POTION_DETECT_MAGIC:
			hadEffect = false;
			for (tempItem = floorItems->nextItem; tempItem != NULL; tempItem = tempItem->nextItem) {
				if (itemMagicChar(tempItem)) {
					pmap[tempItem->xLoc][tempItem->yLoc].flags |= ITEM_DETECTED;
					hadEffect = true;
					refreshDungeonCell(tempItem->xLoc, tempItem->yLoc);
				}
			}
			if (hadEffect) {
				message("you can somehow feel the presence of magic on the level.", true, false);
			} else {
				message("you can somehow feel the absence of magic on the level.", true, false);
			}
			break;
		case POTION_SLOW_SELF:
			player.status.hasted = 0;
			player.status.slowed = player.maxStatus.slowed = 25;
			player.movementSpeed = player.info.movementSpeed * 2;
			player.attackSpeed = player.info.attackSpeed * 2;
			message("you feel yourself moving slower.", true, false);
			break;
		case POTION_HASTE_SELF:
			player.status.slowed = 0;
			player.status.hasted = player.maxStatus.hasted = 25;
			player.movementSpeed = player.info.movementSpeed / 2;
			player.attackSpeed = player.info.attackSpeed / 2;
			message("you feel yourself moving faster.", true, false);
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
			} else if (theItem->enchant1 > 0 && theItem->enchant2 > 0) {
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
				case POTION_WEAKNESS:
				case POTION_POISON:
				case POTION_SLOW_SELF:
				case POTION_PARALYSIS:
				case POTION_CONFUSION:
					return BAD_MAGIC_CHAR;
				default:
					return GOOD_MAGIC_CHAR;
			}
		case WAND:
			if (theItem->charges == 0) {
				return 0;
			}
			switch (theItem->kind) {
				case WAND_HASTE:
				case WAND_INVISIBILITY:
					return BAD_MAGIC_CHAR;
				default:
					return GOOD_MAGIC_CHAR;
			}
		case STAFF:
			switch (theItem->kind) {
				case STAFF_HEALING:
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
	
	theItem = promptForItemOfType(ALL_ITEMS, ITEM_EQUIPPED, 0, "Remove (unequip) what? (a-z or <esc> to cancel)");
	
	if (theItem == NULL) {
		return;
	}
	if (!(theItem->flags & (ITEM_EQUIPPED))) {
		nameOfItem(theItem, buf2, false, false);
		sprintf(buf, "your %s was not equipped.", buf2);
		clearMessage();
		message(buf, true, false);
		return;
	} else if (!unequipItem(theItem, false)) { // this is where the item gets unequipped
		nameOfItem(theItem, buf2, false, false);
		sprintf(buf, "you can't; your %s appears to be cursed.", buf2);
		clearMessage();
		message(buf, true, false);
		return;
	} else {
		if (theItem->category & RING) {
			updateRingBonuses();
		}
		nameOfItem(theItem, buf2, true, true);
		if (strlen(buf2) > 52) {
			nameOfItem(theItem, buf2, false, true);
		}
		clearMessage();
		updateEncumbrance();
		sprintf(buf, "you are no longer %s %s.", (theItem->category & WEAPON ? "wielding" : "wearing"), buf2);
		message(buf, true, false);
	}
	playerTurnEnded();
}

void drop() {
	item *theItem;
	char buf[COLS], buf2[COLS];
	
	theItem = promptForItemOfType(ALL_ITEMS, 0, 0, "Drop what? (a-z or <esc> to cancel)");
	
	if (theItem == NULL) {
		return;
	}
	if (theItem->flags & (ITEM_EQUIPPED) && !unequipItem(theItem, false)) { // this is where the item gets unequipped
		nameOfItem(theItem, buf2, false, false);
		sprintf(buf, "you can't; your %s appears to be cursed.", buf2);
		clearMessage();
		message(buf, true, false);
		return;
	}
	if (theItem = dropItem(theItem)) { // this is where it gets dropped
		clearMessage();
		nameOfItem(theItem, buf2, true, true);
		sprintf(buf, "You dropped %s.", buf2);
		message(buf, true, false);
		playerTurnEnded();
	} else {
		clearMessage();
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
	
	message(prompt, true, false);
	
	keystroke = displayInventory(category, requiredFlags, forbiddenFlags, false);
	
	if (!keystroke) {
		return NULL;
	}
	
	if (keystroke < 'a' || keystroke > 'z') {
		clearMessage();
		if (keystroke != ESCAPE_KEY && keystroke != ACKNOWLEDGE_KEY) {
			message("Invalid entry.", true, false);
		}
		return NULL;
	}
	
	theItem = itemOfPackLetter(keystroke);
	if (theItem == NULL) {
		clearMessage();
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

item *dropItem(item *theItem) {
	item *previousItem, *itemFromTopOfStack;
	if (cellHasTerrainFlag(player.xLoc, player.yLoc, OBSTRUCTS_ITEMS)
		|| pmap[player.xLoc][player.yLoc].flags & (HAS_ITEM)) {
		return NULL;
	}
	if (theItem->quantity > 1 && !(theItem->category & WEAPON)) { // peel off the top item and drop it
		itemFromTopOfStack = generateItem(-1, -1);
		*itemFromTopOfStack = *theItem; // clone the item
		theItem->quantity--;
		itemFromTopOfStack->quantity = 1;
		placeItem(itemFromTopOfStack, player.xLoc, player.yLoc);
		return itemFromTopOfStack;
	} else { // drop the entire item
		for (previousItem = packItems; previousItem->nextItem != theItem; previousItem = previousItem->nextItem);
		previousItem->nextItem = theItem->nextItem;
		placeItem(theItem, player.xLoc, player.yLoc);
		return theItem;
	}
}

boolean equipItem(item *theItem, boolean force) {
	item *previouslyEquippedItem = NULL;
	
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
		player.info.damage = theItem->damage;
		if ((player.info.damage.lowerBound + player.info.damage.upperBound) / 2 > 7.69) { // (1 / 0.13)
			player.info.damage.lowerBound += player.info.damage.lowerBound * theItem->enchant2 * 0.13;
			player.info.damage.upperBound += player.info.damage.upperBound * theItem->enchant2 * 0.13;
		} else {
			player.info.damage.lowerBound += theItem->enchant2;
			player.info.damage.upperBound += theItem->enchant2;
		}
		if (player.info.damage.lowerBound < 1) {
			player.info.damage.lowerBound = 1;
		}
		if (player.info.damage.upperBound < 1) {
			player.info.damage.upperBound = 1;
		}
		rogue.weapon = theItem;
	} else if (theItem->category & ARMOR) {
		player.info.defense = theItem->armor + theItem->enchant1 * 10;
		if (player.info.defense < 0) {
			player.info.defense = 0;
		}
		rogue.armor = theItem;
		// updateStatBar(ARMOR_STAT);
		identify(theItem);
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
//			updateFieldOfViewDisplay();
//			updateClairvoyance();
		}
	}
	theItem->flags |= ITEM_EQUIPPED;
	return true;
}

boolean unequipItem(item *theItem, boolean force) {
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
	return true;
}
	
void updateRingBonuses() {
	short i;
	item *rings[2] = {rogue.ringLeft, rogue.ringRight};
	short lightRadius;
	
	rogue.clairvoyance = rogue.aggravating = rogue.stealthBonus = rogue.transference
						= rogue.awarenessBonus = rogue.regenerationBonus = 0;
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
				case RING_PERCEPTION:
					rogue.lightMultiplier += rings[i]->enchant1;
					break;
				case RING_AWARENESS:
					rogue.awarenessBonus += 20 * rings[i]->enchant1;
					break;
			}
		}
	}

	lightRadius = 2 + (DCOLS - 1) * pow(0.85, rogue.depthLevel);
	if (rogue.lightMultiplier < 0) {
		lightRadius /= ((-1 * rogue.lightMultiplier) + 1);
	} else {
		lightRadius *= (rogue.lightMultiplier);
		lightRadius = max(lightRadius, (rogue.lightMultiplier - 1) * 1.5);
	}
	rogue.minersLight->lightRadius.lowerBound = rogue.minersLight->lightRadius.upperBound = max(2, lightRadius);
	rogue.minersLight->radialFadeToPercent = 35 + max(0, min(65, rogue.lightMultiplier * 5));
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
	turnsForFull = 1000 * TURNS_FOR_FULL_REGEN * pow(0.75, rogue.regenerationBonus);
	
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
	char *stringPtr;
	
	for (i=0; i<NUMBER_FOOD_KINDS; i++) {
		resetItemTableEntry(foodTable + i);
	}
	for (i=0; i<NUMBER_POTION_KINDS; i++) {
		resetItemTableEntry(potionTable + i);
	}
	for (i=0; i<NUMBER_WEAPON_KINDS; i++) {
		resetItemTableEntry(weaponTable + i);
	}
	for (i=0; i<NUMBER_ARMOR_KINDS; i++) {
		resetItemTableEntry(armorTable + i);
	}
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
		randIndex = rand_range(0, NUMBER_ITEM_COLORS - 1);
		strcpy(buf, itemColors[i]);
		strcpy(itemColors[i], itemColors[randIndex]);
		strcpy(itemColors[randIndex], buf);
	}
	for (i=0; i<NUMBER_ITEM_WOODS; i++) {
		randIndex = rand_range(0, NUMBER_ITEM_WOODS - 1);
		strcpy(buf, itemWoods[i]);
		strcpy(itemWoods[i], itemWoods[randIndex]);
		strcpy(itemWoods[randIndex], buf);
	}
	for (i=0; i<NUMBER_ITEM_GEMS; i++) {
		randIndex = rand_range(0, NUMBER_ITEM_GEMS - 1);
		strcpy(buf, itemGems[i]);
		strcpy(itemGems[i], itemGems[randIndex]);
		strcpy(itemGems[randIndex], buf);
	}
	for (i=0; i<NUMBER_SCROLL_KINDS; i++) {
		stringPtr = scrollTable[i].flavor;
		strcpy(itemTitles[i], "\"");
		randNumber = rand_range(3, 4);
		for (j=0; j<randNumber; j++) {
			randIndex = rand_range(0, NUMBER_TITLE_PHONEMES - 1);
			strcpy(buf, itemTitles[i]);
			sprintf(itemTitles[i], "%s%s%s", buf, ((rand_percent(50) && j>0) ? " " : ""), titlePhonemes[randIndex]);
		}
		strcat(itemTitles[i], "\"");
	}
}