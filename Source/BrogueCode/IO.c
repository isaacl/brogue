/*
 *  IO.c
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
#include <time.h>

// accuracy depends on how many clock cycles occur per second
#define MILLISECONDS	(clock() * 1000 / CLOCKS_PER_SEC)

#define MILLISECONDS_FOR_CAUTION	100

void considerCautiousMode() {
	/*
	signed long oldMilliseconds = rogue.milliseconds;
	rogue.milliseconds = MILLISECONDS;
	clock_t i = clock();
	printf("\n%li", i);
	if (rogue.milliseconds - oldMilliseconds < MILLISECONDS_FOR_CAUTION) {
		rogue.cautiousMode = true;
	}*/
}

// flags the entire window as needing to be redrawn at next flush.
// very low level -- does not interface with the guts of the game.
void refreshScreen() {
	short i, j;
	
	for( i=0; i<COLS; i++ ) {
		for( j=0; j<ROWS; j++ ) {
			displayBuffer[i][j].needsUpdate = true;
		}
	}
	commitDraws();
}

void displayLevel() {
	
	short i, j;
	
	for( i=0; i<DCOLS; i++ ) {
		for( j=0; j<DROWS; j++ ) {
			refreshDungeonCell(i, j);
		}
	}
}

// converts colors into components for memory; note that it cuts random factors by 80% in line with the memory color
void storeColorComponents(char components[3], color *theColor) {
	short rand = rand_range(0, theColor->rand);
	components[0] = max(0, min(100, theColor->red + rand_range(0, theColor->redRand) / 5 + rand / 5));
	components[1] = max(0, min(100, theColor->green + rand_range(0, theColor->greenRand) / 5 + rand / 5));
	components[2] = max(0, min(100, theColor->blue + rand_range(0, theColor->blueRand) / 5 + rand / 5));
}

void bakeTerrainColors(color *foreColor, color *backColor, short x, short y) {
	const short *vals = &(terrainRandomValues[x][y][0]);
	const short foreRand = foreColor->rand * vals[6] / 1000;
	const short backRand = backColor->rand * vals[7] / 1000;
	
	foreColor->red += foreColor->redRand * vals[0] / 1000 + foreRand;
	foreColor->green += foreColor->greenRand * vals[1] / 1000 + foreRand;
	foreColor->blue += foreColor->blueRand * vals[2] / 1000 + foreRand;
	foreColor->redRand = foreColor->greenRand = foreColor->blueRand = foreColor->rand = 0;
	
	backColor->red += backColor->redRand * vals[3] / 1000 + backRand;
	backColor->green += backColor->greenRand * vals[4] / 1000 + backRand;
	backColor->blue += backColor->blueRand * vals[5] / 1000 + backRand;
	backColor->redRand = backColor->greenRand = backColor->blueRand = backColor->rand = 0;
	
	if (foreColor->colorDances || backColor->colorDances) {
		pmap[x][y].flags |= TERRAIN_COLORS_DANCING;
	} else {
		pmap[x][y].flags &= ~TERRAIN_COLORS_DANCING;
	}
}

void bakeColor(color *theColor) {
	short rand;
	
	rand = rand_range(0, theColor->rand);
	theColor->red += rand_range(0, theColor->redRand) + rand;
	theColor->green += rand_range(0, theColor->greenRand) + rand;
	theColor->blue += rand_range(0, theColor->blueRand) + rand;
	theColor->redRand = theColor->greenRand = theColor->blueRand = theColor->rand = 0;
}

/*(tileCatalog[pmap[i][j].layers[DUNGEON]].foreColor->colorDances
 || tileCatalog[pmap[i][j].layers[DUNGEON]].backColor->colorDances
 || (tileCatalog[pmap[i][j].layers[LIQUID]].backColor) && tileCatalog[pmap[i][j].layers[LIQUID]].backColor->colorDances
 || (tileCatalog[pmap[i][j].layers[LIQUID]].foreColor) && tileCatalog[pmap[i][j].layers[LIQUID]].foreColor->colorDances
 || (tileCatalog[pmap[i][j].layers[SURFACE]].backColor) && tileCatalog[pmap[i][j].layers[SURFACE]].backColor->colorDances
 || (tileCatalog[pmap[i][j].layers[SURFACE]].foreColor) && tileCatalog[pmap[i][j].layers[SURFACE]].foreColor->colorDances
 || (tileCatalog[pmap[i][j].layers[GAS]].backColor) && tileCatalog[pmap[i][j].layers[GAS]].backColor->colorDances
 || (tileCatalog[pmap[i][j].layers[GAS]].foreColor) && tileCatalog[pmap[i][j].layers[GAS]].foreColor->colorDances
 || (player.status.hallucinating && pmap[i][j].flags & (VISIBLE)))*/

void shuffleTerrainColors(short percentOfCells, boolean refreshCells) {
	short i, j, k;
	
	for (i=0; i<DCOLS; i++) {
		for( j=0; j<DROWS; j++ ) {
			if (pmap[i][j].flags & (VISIBLE | CLAIRVOYANT_VISIBLE)
				&& (!rogue.automationActive || !(rogue.turnNumber % 10))
				&& (pmap[i][j].flags & TERRAIN_COLORS_DANCING
					|| (player.status.hallucinating && pmap[i][j].flags & VISIBLE))
				&& (percentOfCells >= 100 || rand_range(1, 100) <= percentOfCells)) {
					
					for (k=0; k<8; k++) {
						//terrainRandomValues[i][j][k] = rand_range(0, 1000);
						terrainRandomValues[i][j][k] += rand_range(-600, 600);
						terrainRandomValues[i][j][k] = max(0, min(1000, terrainRandomValues[i][j][k]));
					}
					
					if (refreshCells) {
						refreshDungeonCell(i, j);
					}
				}
		}
	}
}

#define MIN_COLOR_DIFF			500

// weighted sum of the squares of the component differences
#define COLOR_DIFF(f, b)		 (((f).red - (b).red) * ((f).red - (b).red) * 0.2126 \
								+ ((f).green - (b).green) * ((f).green - (b).green) * 0.7152 \
								+ ((f).blue - (b).blue) * ((f).blue - (b).blue) * 0.0722)

// if forecolor is too similar to back, darken or lighten it and return true.
// Assumes colors have already been baked (no random components).
boolean separateColors(color *fore, color *back) {
	color f, b, *modifier;
	short failsafe;
	boolean madeChange;
	
	f = *fore;
	b = *back;
	f.red		= max(0, min(100, f.red));
	f.green		= max(0, min(100, f.green));
	f.blue		= max(0, min(100, f.blue));
	b.red		= max(0, min(100, b.red));
	b.green		= max(0, min(100, b.green));
	b.blue		= max(0, min(100, b.blue));
	
	if (f.red + f.blue + f.green > 50 * 3) {
		modifier = &black;
	} else {
		modifier = &white;
	}
	
	madeChange = false;
	failsafe = 10;
	
	while(COLOR_DIFF(f, b) < MIN_COLOR_DIFF && --failsafe) {
		applyColorAverage(&f, modifier, 20);
		madeChange = true;
	}
	
	if (madeChange) {
		*fore = f;
		return true;
	} else {
		return false;
	}
}

// okay, this is kind of a beast...
void getCellAppearance(short x, short y, uchar *returnChar, color *returnForeColor, color *returnBackColor) {	
	short bestBCPriority, bestFCPriority, bestCharPriority;
	uchar cellChar = 0;
	color cellForeColor, cellBackColor, lightMultiplierColor, gasAugmentColor;
	boolean haveForeColor = false, haveBackColor = false;
	boolean monsterWithDetectedItem = false, needDistinctness = false;
	short gasAugmentWeight = 0;
	creature *monst = NULL;
	item *theItem = NULL;
	uchar itemChars[] = {POTION_CHAR, SCROLL_CHAR, FOOD_CHAR, WAND_CHAR,
						STAFF_CHAR, GOLD_CHAR, ARMOR_CHAR, WEAPON_CHAR, RING_CHAR};
	enum dungeonLayers layer;
	
	if (pmap[x][y].flags & HAS_MONSTER) {
		monst = monsterAtLoc(x, y);
		monsterWithDetectedItem = (monst->carriedItem && (monst->carriedItem->flags & ITEM_MAGIC_DETECTED)
								   && itemMagicChar(monst->carriedItem) && !canSeeMonster(monst));
	}
	
	if (monsterWithDetectedItem) {
		theItem = monst->carriedItem;
	} else {
		theItem = itemAtLoc(x, y);
	}
	
	if (!(pmap[x][y].flags & (VISIBLE | CLAIRVOYANT_VISIBLE | ITEM_DETECTED | HAS_PLAYER))
		&& (!player.status.telepathic || !monst)
		&& !monsterWithDetectedItem
		&& (pmap[x][y].flags & (DISCOVERED | MAGIC_MAPPED))
		&& (pmap[x][y].flags & STABLE_MEMORY)) {
		
		// restore memory
		cellChar = pmap[x][y].rememberedAppearance.character;
		cellForeColor.redRand = cellForeColor.greenRand = cellForeColor.blueRand = cellForeColor.rand = 0;
		cellBackColor.redRand = cellBackColor.greenRand = cellBackColor.blueRand = cellBackColor.rand = 0;
		cellForeColor.red = pmap[x][y].rememberedAppearance.foreColorComponents[0];
		cellForeColor.green = pmap[x][y].rememberedAppearance.foreColorComponents[1];
		cellForeColor.blue = pmap[x][y].rememberedAppearance.foreColorComponents[2];
		cellBackColor.red = pmap[x][y].rememberedAppearance.backColorComponents[0];
		cellBackColor.green = pmap[x][y].rememberedAppearance.backColorComponents[1];
		cellBackColor.blue = pmap[x][y].rememberedAppearance.backColorComponents[2];
	} else {
		
		bestFCPriority = bestBCPriority = bestCharPriority = 10000;
		
		for (layer = 0; layer < NUMBER_TERRAIN_LAYERS; layer++) {
			// gas shows up as a color augment, not directly
			if (pmap[x][y].layers[layer] && layer != GAS) {
				
				if ((!haveForeColor || tileCatalog[pmap[x][y].layers[layer]].drawPriority < bestFCPriority) && (tileCatalog[pmap[x][y].layers[layer]].foreColor)) {
					cellForeColor = *(tileCatalog[pmap[x][y].layers[layer]].foreColor);
					haveForeColor = true;
					bestFCPriority = tileCatalog[pmap[x][y].layers[layer]].drawPriority;
				}
				if ((!haveBackColor || tileCatalog[pmap[x][y].layers[layer]].drawPriority < bestBCPriority) && (tileCatalog[pmap[x][y].layers[layer]].backColor)) {
					cellBackColor = *(tileCatalog[pmap[x][y].layers[layer]].backColor);
					haveBackColor = true;
					bestBCPriority = tileCatalog[pmap[x][y].layers[layer]].drawPriority;
				}
				if ((!cellChar || tileCatalog[pmap[x][y].layers[layer]].drawPriority < bestCharPriority) && (tileCatalog[pmap[x][y].layers[layer]].displayChar)) {
					cellChar = tileCatalog[pmap[x][y].layers[layer]].displayChar;
					bestCharPriority = tileCatalog[pmap[x][y].layers[layer]].drawPriority;
				}
			}
		}
		
		colorMultiplierFromDungeonLight(x, y, &lightMultiplierColor);
		
		if (pmap[x][y].layers[GAS] && tileCatalog[pmap[x][y].layers[GAS]].backColor
			/*&& (pmap[x][y].flags & (VISIBLE | CLAIRVOYANT_VISIBLE))*/) {
			gasAugmentColor = *(tileCatalog[pmap[x][y].layers[GAS]].backColor);
			gasAugmentWeight = min(85, 35 + pmap[x][y].volume);
		}
		
		if (pmap[x][y].flags & HAS_PLAYER) {
			cellChar = player.info.displayChar;
			cellForeColor = *(player.info.foreColor);
			needDistinctness = true;
		} else if (((pmap[x][y].flags & HAS_ITEM) && (pmap[x][y].flags & ITEM_DETECTED)
					&& itemMagicChar(theItem)
					&& (!(pmap[x][y].flags & (VISIBLE | CLAIRVOYANT_VISIBLE)) ||
						cellHasTerrainFlag(x, y, OBSTRUCTS_ITEMS)))
				   || monsterWithDetectedItem){
			cellChar = itemMagicChar(theItem);
			cellForeColor = white;
			cellBackColor = black;
		} else if (pmap[x][y].flags & HAS_MONSTER && (pmap[x][y].flags & (VISIBLE | CLAIRVOYANT_VISIBLE)
													  || (monst->info.flags & MONST_IMMOBILE && pmap[x][y].flags & DISCOVERED))
				   && !(monst->info.flags & MONST_INVISIBLE)
				   && (!(monst->bookkeepingFlags & MONST_SUBMERGED) || (rogue.inWater && distanceBetween(x, y, player.xLoc, player.yLoc) < 10))) {
			needDistinctness = true;
			if (player.status.hallucinating > 0 && !(monst->info.flags & MONST_INANIMATE)) {
				cellChar = rand_range('a', 'z') + (rand_range(0, 1) ? 'A' - 'a' : 0);
				cellForeColor = *(monsterCatalog[rand_range(1, NUMBER_MONSTER_KINDS - 1)].foreColor);
			} else {
				cellChar = monst->info.displayChar;
				cellForeColor = *(monst->info.foreColor);
				//DEBUG if (monst->bookkeepingFlags & MONST_LEADER) applyColorAverage(&cellBackColor, &purple, 50);
			}
		} else if (player.status.telepathic > 0 && pmap[x][y].flags & HAS_MONSTER
				   && !(monst->info.flags & MONST_INANIMATE)
				   && (!(pmap[x][y].flags & (VISIBLE | CLAIRVOYANT_VISIBLE))
					   || (monst->info.flags & MONST_INVISIBLE || monst->bookkeepingFlags & MONST_SUBMERGED))) {
			if (player.status.hallucinating) {
				cellChar = (rand_range(0, 1) ? 'X' : 'x');
			} else {
				cellChar = (monst->info.displayChar >= 'a' && monst->info.displayChar <= 'z' ? 'x' : 'X');
			}
			cellForeColor = white;
			lightMultiplierColor = white;
			if (!(pmap[x][y].flags & DISCOVERED)) {
				cellBackColor = black;
				gasAugmentColor = black;
			}
		} else if (pmap[x][y].flags & HAS_ITEM && !cellHasTerrainFlag(x, y, OBSTRUCTS_ITEMS)
				   && (pmap[x][y].flags & (VISIBLE | CLAIRVOYANT_VISIBLE) || (pmap[x][y].flags & (DISCOVERED) && !cellHasTerrainFlag(x, y, MOVES_ITEMS))) ) {
			needDistinctness = true;
			if (player.status.hallucinating) {
				cellChar = itemChars[rand_range(0, 8)];
				cellForeColor = yellow;
			} else {
				//for (theItem = floorItems; (theItem->xLoc != x || theItem->yLoc != y); theItem = theItem->nextItem);
				theItem = itemAtLoc(x, y);
				cellChar = theItem->displayChar;
				cellForeColor = *(theItem->foreColor);
			}
		} else if (pmap[x][y].flags & (DISCOVERED | MAGIC_MAPPED | CLAIRVOYANT_VISIBLE)) {
			// just don't want these to be plotted as black
		} else {
			*returnChar = ' ';
			*returnForeColor = black;
			*returnBackColor = undiscoveredColor;
			return;
		}
		
		if (gasAugmentWeight) {
			applyColorAverage(&cellForeColor, &gasAugmentColor, gasAugmentWeight);
			// phantoms create sillhouettes in gas clouds
			if (pmap[x][y].flags & HAS_MONSTER && monst->info.flags & MONST_INVISIBLE && !player.status.telepathic) {
				if (player.status.hallucinating > 0) {
					cellChar = monsterCatalog[rand_range(1, NUMBER_MONSTER_KINDS - 1)].displayChar;
				} else {
					cellChar = monst->info.displayChar;
				}
				cellForeColor = cellBackColor;
			}
			applyColorAverage(&cellBackColor, &gasAugmentColor, gasAugmentWeight);
		}
		
		if (!(pmap[x][y].flags & (VISIBLE | CLAIRVOYANT_VISIBLE | ITEM_DETECTED | HAS_PLAYER))
			&& (!player.status.telepathic || !monst) && !monsterWithDetectedItem) {
			// store memory
			storeColorComponents(pmap[x][y].rememberedAppearance.foreColorComponents, &cellForeColor);
			storeColorComponents(pmap[x][y].rememberedAppearance.backColorComponents, &cellBackColor);
			pmap[x][y].rememberedAppearance.character = cellChar;
			pmap[x][y].flags |= STABLE_MEMORY;
			pmap[x][y].rememberedTerrain = pmap[x][y].layers[highestPriorityLayer(x, y, false)];
			if (pmap[x][y].flags & HAS_ITEM) {
				for (theItem = floorItems; (theItem->xLoc != x || theItem->yLoc != y); theItem = theItem->nextItem);
				pmap[x][y].rememberedItemCategory = theItem->category;
			} else {
				pmap[x][y].rememberedItemCategory = 0;
			}
			
			// then restore to eliminate visual artifacts
			cellForeColor.redRand = cellForeColor.greenRand = cellForeColor.blueRand = cellForeColor.rand = 0;
			cellBackColor.redRand = cellBackColor.greenRand = cellBackColor.blueRand = cellBackColor.rand = 0;
			cellForeColor = colorFromComponents(pmap[x][y].rememberedAppearance.foreColorComponents);
			cellBackColor = colorFromComponents(pmap[x][y].rememberedAppearance.backColorComponents);
		}
	}
	
	if ((pmap[x][y].flags & (ITEM_DETECTED) || monsterWithDetectedItem
		 || (player.status.telepathic > 0 && pmap[x][y].flags & (HAS_MONSTER) && !(monst->info.flags & MONST_IMMOBILE)))
		&& !(pmap[x][y].flags & (VISIBLE | CLAIRVOYANT_VISIBLE))) {
		// do nothing		
	} else if (!(pmap[x][y].flags & VISIBLE) && pmap[x][y].flags & CLAIRVOYANT_VISIBLE) {
		applyColorMultiplier(&cellForeColor, &clairvoyanceColor);
		applyColorMultiplier(&cellBackColor, &clairvoyanceColor);
	} else if (!(pmap[x][y].flags & DISCOVERED) && pmap[x][y].flags & MAGIC_MAPPED) {
		needDistinctness = false;
		applyColorMultiplier(&cellForeColor, &magicMapColor);
		applyColorMultiplier(&cellBackColor, &magicMapColor);
	} else if (!(pmap[x][y].flags & VISIBLE)) {
		needDistinctness = false;
		if (rogue.inWater) {
			applyColorAverage(&cellForeColor, &black, 80);
			applyColorAverage(&cellBackColor, &black, 80);
		} else {
			applyColorMultiplier(&cellForeColor, &memoryColor);
			applyColorMultiplier(&cellBackColor, &memoryColor);
			applyColorAverage(&cellForeColor, &memoryOverlay, 15);
			applyColorAverage(&cellBackColor, &memoryOverlay, 15);
		}
	} else {
		applyColorMultiplier(&cellForeColor, &lightMultiplierColor);
		applyColorMultiplier(&cellBackColor, &lightMultiplierColor);
		
		if (player.status.hallucinating) {
			randomizeColor(&cellForeColor, 40 * player.status.hallucinating / 300 + 20);
			randomizeColor(&cellBackColor, 40 * player.status.hallucinating / 300 + 20);
		}
		if (rogue.inWater) {
			applyColorMultiplier(&cellForeColor, &deepWaterLightColor);
			applyColorMultiplier(&cellBackColor, &deepWaterLightColor);
		}
	}
	//DEBUG cellBackColor.red = max(0,((tmap[x][y].scent - rogue.scentTurnNumber) * 2) + 100);
	/*DEBUG if (pmap[x][y].flags & CAUGHT_FIRE_THIS_TURN) {
		cellBackColor.red = 100;
	}*/
	
	bakeTerrainColors(&cellForeColor, &cellBackColor, x, y);
	
	if (needDistinctness) {
		separateColors(&cellForeColor, &cellBackColor);
	}
	
	*returnChar = cellChar;
	*returnForeColor = cellForeColor;
	*returnBackColor = cellBackColor;
}

void refreshDungeonCell(short x, short y) {
	uchar cellChar;
	color foreColor, backColor;
	getCellAppearance(x, y, &cellChar, &foreColor, &backColor);
	plotCharWithColor(cellChar, x + STAT_BAR_WIDTH, y + MESSAGE_LINES, foreColor, backColor);
}

void applyColorMultiplier(color *baseColor, color *multiplierColor) {
	baseColor->red = baseColor->red * multiplierColor->red / 100;
	baseColor->redRand = baseColor->redRand * multiplierColor->redRand / 100;
	baseColor->green = baseColor->green * multiplierColor->green / 100;
	baseColor->greenRand = baseColor->greenRand * multiplierColor->greenRand / 100;
	baseColor->blue = baseColor->blue * multiplierColor->blue / 100;
	baseColor->blueRand = baseColor->blueRand * multiplierColor->blueRand / 100;
	baseColor->rand = baseColor->rand * multiplierColor->rand / 100;
	//baseColor->colorDances *= multiplierColor->colorDances;
	return;
}

void applyColorAverage(color *baseColor, color *newColor, short averageWeight) {
	short weightComplement = 100 - averageWeight;
	baseColor->red = (baseColor->red * weightComplement + newColor->red * averageWeight) / 100;
	baseColor->redRand = (baseColor->redRand * weightComplement + newColor->redRand * averageWeight) / 100;
	baseColor->green = (baseColor->green * weightComplement + newColor->green * averageWeight) / 100;
	baseColor->greenRand = (baseColor->greenRand * weightComplement + newColor->greenRand * averageWeight) / 100;
	baseColor->blue = (baseColor->blue * weightComplement + newColor->blue * averageWeight) / 100;
	baseColor->blueRand = (baseColor->blueRand * weightComplement + newColor->blueRand * averageWeight) / 100;
	baseColor->rand = (baseColor->rand * weightComplement + newColor->rand * averageWeight) / 100;
	return;
}

void applyColorAugment(color *baseColor, color *augmentingColor, short augmentWeight) {
	baseColor->red += (augmentingColor->red * augmentWeight) / 100;
	baseColor->redRand += (augmentingColor->redRand * augmentWeight) / 100;
	baseColor->green += (augmentingColor->green * augmentWeight) / 100;
	baseColor->greenRand += (augmentingColor->greenRand * augmentWeight) / 100;
	baseColor->blue += (augmentingColor->blue * augmentWeight) / 100;
	baseColor->blueRand += (augmentingColor->blueRand * augmentWeight) / 100;
	baseColor->rand += (augmentingColor->rand * augmentWeight) / 100;
	return;
}

void desaturate(color *baseColor, short weight) {
	short avg;
	avg = (baseColor->red + baseColor->green + baseColor->blue) / 3 + 1;
	baseColor->red = baseColor->red * (100 - weight) / 100 + (avg * weight / 100);
	baseColor->green = baseColor->green * (100 - weight) / 100 + (avg * weight / 100);
	baseColor->blue = baseColor->blue * (100 - weight) / 100 + (avg * weight / 100);
	
	avg = (baseColor->redRand + baseColor->greenRand + baseColor->blueRand) / 3 + 1;
	baseColor->redRand = baseColor->redRand * (100 - weight) / 100;
	baseColor->greenRand = baseColor->greenRand * (100 - weight) / 100;
	baseColor->blueRand = baseColor->blueRand * (100 - weight) / 100;
	
	baseColor->rand += 3* avg * weight / 100;
}

short randomizeByPercent(short input, short percent) {
	return (rand_range(input * (100 - percent) / 100, input * (100 + percent) / 100));
}

void randomizeColor(color *baseColor, short randomizePercent) {
	baseColor->red = randomizeByPercent(baseColor->red, randomizePercent);
	baseColor->green = randomizeByPercent(baseColor->green, randomizePercent);
	baseColor->blue = randomizeByPercent(baseColor->blue, randomizePercent);
}

// takes dungeon coordinates
void colorBlendCell(short x, short y, color *hiliteColor, short hiliteStrength) {
	uchar displayChar;
	color foreColor, backColor;
	
	getCellAppearance(x, y, &displayChar, &foreColor, &backColor);
	applyColorAverage(&foreColor, hiliteColor, hiliteStrength);
	applyColorAverage(&backColor, hiliteColor, hiliteStrength);
	plotCharWithColor(displayChar, x + STAT_BAR_WIDTH, y + MESSAGE_LINES, foreColor, backColor);
}

// takes dungeon coordinates
void hiliteCell(short x, short y, color *hiliteColor, short hiliteStrength) {
	uchar displayChar;
	color foreColor, backColor;
	
	getCellAppearance(x, y, &displayChar, &foreColor, &backColor);
	applyColorAugment(&foreColor, hiliteColor, hiliteStrength);
	applyColorAugment(&backColor, hiliteColor, hiliteStrength);
	plotCharWithColor(displayChar, x + STAT_BAR_WIDTH, y + MESSAGE_LINES, foreColor, backColor);
}

void colorMultiplierFromDungeonLight(short x, short y, color *editColor) {
	editColor->red = max(0, editColor->redRand = tmap[x][y].light[0]);
	editColor->green = max(0, editColor->greenRand = tmap[x][y].light[1]);
	editColor->blue = max(0, editColor->blueRand = tmap[x][y].light[2]);
	editColor->rand = max(0, tmap[x][y].light[0] + tmap[x][y].light[1] + tmap[x][y].light[2]) / 3;
	editColor->colorDances = false;
}

void plotCharWithColor(uchar inputChar, short xLoc, short yLoc, color cellForeColor, color cellBackColor) {
	
	short foreRed = cellForeColor.red,
	foreGreen = cellForeColor.green,
	foreBlue = cellForeColor.blue,
	
	backRed = cellBackColor.red,
	backGreen = cellBackColor.green,
	backBlue = cellBackColor.blue,
	
	foreRand, backRand;
	
	if (rogue.gameHasEnded) {
		return;
	}
	
	foreRand = rand_range(0, cellForeColor.rand);
	backRand = rand_range(0, cellBackColor.rand);
	foreRed += rand_range(0, cellForeColor.redRand) + foreRand;
	foreGreen += rand_range(0, cellForeColor.greenRand) + foreRand;
	foreBlue += rand_range(0, cellForeColor.blueRand) + foreRand;
	backRed += rand_range(0, cellBackColor.redRand) + backRand;
	backGreen += rand_range(0, cellBackColor.greenRand) + backRand;
	backBlue += rand_range(0, cellBackColor.blueRand) + backRand;
	
	foreRed =		min(100, max(0, foreRed));
	foreGreen =		min(100, max(0, foreGreen));
	foreBlue =		min(100, max(0, foreBlue));
	backRed =		min(100, max(0, backRed));
	backGreen =		min(100, max(0, backGreen));
	backBlue =		min(100, max(0, backBlue));
	
	if (inputChar		!= displayBuffer[xLoc][yLoc].character
		|| foreRed		!= displayBuffer[xLoc][yLoc].foreColorComponents[0]
		|| foreGreen	!= displayBuffer[xLoc][yLoc].foreColorComponents[1]
		|| foreBlue		!= displayBuffer[xLoc][yLoc].foreColorComponents[2]
		|| backRed		!= displayBuffer[xLoc][yLoc].backColorComponents[0]
		|| backGreen	!= displayBuffer[xLoc][yLoc].backColorComponents[1]
		|| backBlue		!= displayBuffer[xLoc][yLoc].backColorComponents[2]) {
		
		displayBuffer[xLoc][yLoc].needsUpdate = true;
		
		displayBuffer[xLoc][yLoc].character = inputChar;
		displayBuffer[xLoc][yLoc].foreColorComponents[0] = foreRed;
		displayBuffer[xLoc][yLoc].foreColorComponents[1] = foreGreen;
		displayBuffer[xLoc][yLoc].foreColorComponents[2] = foreBlue;
		displayBuffer[xLoc][yLoc].backColorComponents[0] = backRed;
		displayBuffer[xLoc][yLoc].backColorComponents[1] = backGreen;
		displayBuffer[xLoc][yLoc].backColorComponents[2] = backBlue;
	}
}

void plotCharToBuffer(uchar inputChar, short x, short y, color *foreColor, color *backColor, cellDisplayBuffer dbuf[COLS][ROWS]) {
	dbuf[x][y].foreColorComponents[0] = foreColor->red + rand_range(0, foreColor->redRand) + rand_range(0, foreColor->rand);
	dbuf[x][y].foreColorComponents[1] = foreColor->green + rand_range(0, foreColor->greenRand) + rand_range(0, foreColor->rand);
	dbuf[x][y].foreColorComponents[2] = foreColor->blue + rand_range(0, foreColor->blueRand) + rand_range(0, foreColor->rand);
	dbuf[x][y].backColorComponents[0] = backColor->red + rand_range(0, backColor->redRand) + rand_range(0, backColor->rand);
	dbuf[x][y].backColorComponents[1] = backColor->green + rand_range(0, backColor->greenRand) + rand_range(0, backColor->rand);
	dbuf[x][y].backColorComponents[2] = backColor->blue + rand_range(0, backColor->blueRand) + rand_range(0, backColor->rand);
	dbuf[x][y].character = inputChar;
	dbuf[x][y].opacity = 100;
}

// Set to false and draws don't take effect, they simply queue up. Set to true and all of the
// queued up draws take effect.
void commitDraws() {
	short i, j;
	
	for (i=0; i<COLS; i++) {
		for (j=0; j<ROWS; j++) {
			if (displayBuffer[i][j].needsUpdate) {
				plotChar(displayBuffer[i][j].character, i, j,
						 displayBuffer[i][j].foreColorComponents[0],
						 displayBuffer[i][j].foreColorComponents[1],
						 displayBuffer[i][j].foreColorComponents[2],
						 displayBuffer[i][j].backColorComponents[0],
						 displayBuffer[i][j].backColorComponents[1],
						 displayBuffer[i][j].backColorComponents[2]);
				displayBuffer[i][j].needsUpdate = false;
			}
		}
	}
}

void copyDisplayBuffer(cellDisplayBuffer toBuf[COLS][ROWS], cellDisplayBuffer fromBuf[COLS][ROWS]) {
	short i, j;
	
	for (i=0; i<COLS; i++) {
		for (j=0; j<ROWS; j++) {
			toBuf[i][j] = fromBuf[i][j];
		}
	}
}

void clearDisplayBuffer(cellDisplayBuffer dbuf[COLS][ROWS]) {
	short i, j, k;
	
	for (i=0; i<COLS; i++) {
		for (j=0; j<ROWS; j++) {
			dbuf[i][j].character = ' ';
			for (k=0; k<3; k++) {
				dbuf[i][j].foreColorComponents[k] = 0;
				dbuf[i][j].backColorComponents[k] = 0;
			}
			dbuf[i][j].opacity = 0;
		}
	}
}

color colorFromComponents(char rgb[3]) {
	color theColor = black;
	theColor.red	= rgb[0];
	theColor.green	= rgb[1];
	theColor.blue	= rgb[2];
	return theColor;
}

// draws overBuf over the current display with per-cell pseudotransparency as specified in overBuf.
// If previousBuf is not null, it gets filled with the preexisting display for reversion purposes.
void overlayDisplayBuffer(cellDisplayBuffer overBuf[COLS][ROWS], cellDisplayBuffer previousBuf[COLS][ROWS]) {
	short i, j;
	color foreColor, backColor, tempColor;
	uchar character;
	
	if (previousBuf) {
		copyDisplayBuffer(previousBuf, displayBuffer);
	}
	
	for (i=0; i<COLS; i++) {
		for (j=0; j<ROWS; j++) {
			
			backColor = colorFromComponents(overBuf[i][j].backColorComponents);
			
			// character and fore color:
			if (overBuf[i][j].character == ' ') {
				character = displayBuffer[i][j].character;
				foreColor = colorFromComponents(displayBuffer[i][j].foreColorComponents);
				applyColorAverage(&foreColor, &backColor, overBuf[i][j].opacity);
			} else {
				character = overBuf[i][j].character;
				foreColor = colorFromComponents(overBuf[i][j].foreColorComponents);
			}
			
			// back color:
			tempColor = colorFromComponents(displayBuffer[i][j].backColorComponents);
			applyColorAverage(&backColor, &tempColor, 100 - overBuf[i][j].opacity);
			
			plotCharWithColor(character, i, j, foreColor, backColor);
		}
	}
}

// takes a list of locations, a color and a list of strengths and flashes the foregrounds of those locations.
// Strengths are percentages measuring how hard the color flashes at its peak.
void flashForeground(short *x, short *y, color **flashColor, short *flashStrength, short count, short frames) {
	short i, j, percent;
	uchar *displayChar;
	color *bColor, *fColor, newColor;
	
	if (count <= 0) {
		return;
	}
	
	displayChar = (uchar *) malloc(count * sizeof(uchar));
	fColor = (color *) malloc(count * sizeof(color));
	bColor = (color *) malloc(count * sizeof(color));
	
	for (i=0; i<count; i++) {
		getCellAppearance(x[i], y[i], &displayChar[i], &fColor[i], &bColor[i]);
		bakeColor(&fColor[i]);
		bakeColor(&bColor[i]);
	}
	
	for (j=frames; j>= 0; j--) {
		for (i=0; i<count; i++) {
			percent = flashStrength[i] * j / frames;
			newColor = fColor[i];
			applyColorAverage(&newColor, flashColor[i], percent);
			plotCharWithColor(displayChar[i], x[i] + STAT_BAR_WIDTH, y[i] + MESSAGE_LINES, newColor, bColor[i]);
		}
		if (j) {
			if (pauseBrogue(1)) {
				j = 1;
			}
		}
	}
	
	free(displayChar);
	free(fColor);
	free(bColor);
}

void flash(color *theColor, short frames, short x, short y) {
	short i;
	boolean interrupted = false;
	
	for (i=0; i<frames && !interrupted; i++) {
		colorBlendCell(x, y, theColor, 100 - 100 * i / frames);
		interrupted = pauseBrogue(50);
	}
	
	refreshDungeonCell(x, y);
}

// special effect expanding flash of light at dungeon coordinates (x, y) restricted to tiles with matching flags
void lightFlash(color *theColor, unsigned long reqTerrainFlags,
				unsigned long reqTileFlags, short frames, short maxRadius, short x, short y) {
	short i, j, k, intensity, currentRadius, fadeOut;
	short localRadius[DCOLS][DROWS];
	boolean tileQualifies[DCOLS][DROWS], aTileQualified, fastForward;
	
	aTileQualified = false;
	fastForward = false;
	
	for (i = max(x - maxRadius, 0); i < min(x + maxRadius, DCOLS); i++) {
		for (j = max(y - maxRadius, 0); j < min(y + maxRadius, DROWS-1); j++) {
			if ((!reqTerrainFlags || cellHasTerrainFlag(reqTerrainFlags, i, j))
				&& (!reqTileFlags || (pmap[i][j].flags & reqTileFlags))
				&& (i-x) * (i-x) + (j-y) * (j-y) <= maxRadius * maxRadius) {
				tileQualifies[i][j] = true;
				localRadius[i][j] = sqrt((i-x) * (i-x) + (j-y) * (j-y));
				aTileQualified = true;
			} else {
				tileQualifies[i][j] = false;
			}
		}
	}
	
	if (!aTileQualified) {
		return;
	}
	
	for (k = 1; k <= frames; k++) {
		currentRadius = max(1, maxRadius * k / frames);
		fadeOut = min(100, (frames - k) * 100 * 5 / frames);
		for (i = max(x - maxRadius, 0); i < min(x + maxRadius, DCOLS); i++) {
			for (j = max(y - maxRadius, 0); j < min(y + maxRadius, DROWS-1); j++) {
				if (tileQualifies[i][j] && localRadius[i][j] <= currentRadius) {
					
					intensity = 100 - 100 * (currentRadius - localRadius[i][j] - 2) / currentRadius;
					intensity = fadeOut * intensity / 100;
					
					hiliteCell(i, j, theColor, intensity);
				}
			}
		}
		if (!fastForward && pauseBrogue(50)) {
			k = frames - 1;
			fastForward = true;
		}
	}
}

#define bCurve(x)	(((x) * (x) + 11) / (10 * ((x) * (x) + 1)) - 0.1)

// x and y are global coordinates, not within the playing square
void funkyFade(cellDisplayBuffer displayBuf[COLS][ROWS], color *colorStart,
			   color *colorEnd, short stepCount, short x, short y, boolean invert) {
	short i, j, n, weight;
	double x2, y2, weightGrid[COLS][ROWS][3], percentComplete;
	color tempColor, colorMid, foreColor, backColor;
	uchar tempChar;
	short **distanceMap;
	boolean fastForward;
	
#ifdef BROGUE_LIBTCOD
	stepCount *= 15; // libtcod displays much faster
#endif
	
	fastForward = false;
	distanceMap = allocDynamicGrid();
	calculateDistances(distanceMap, player.xLoc, player.yLoc, OBSTRUCTS_PASSABILITY, 0);
	
	for (i=0; i<COLS; i++) {
		x2 = (double) ((i - x) * 5.0 / COLS);
		for (j=0; j<ROWS; j++) {
			y2 = (double) ((j - y) * 2.5 / ROWS);
			
			weightGrid[i][j][0] = bCurve(x2*x2+y2*y2) * (.7 + .3 * cos(5*x2*x2) * cos(5*y2*y2));
			weightGrid[i][j][1] = bCurve(x2*x2+y2*y2) * (.7 + .3 * sin(5*x2*x2) * cos(5*y2*y2));
			weightGrid[i][j][2] = bCurve(x2*x2+y2*y2);
			
		}	
	}
	
	for (n=(invert ? stepCount - 1 : 0); (invert ? n >= 0 : n <= stepCount); n += (invert ? -1 : 1)) {
		for (i=0; i<COLS; i++) {
			for (j=0; j<ROWS; j++) {
				
				percentComplete = (double) (n) * 100 / stepCount;
				
				colorMid = *colorStart;
				if (colorEnd) {
					applyColorAverage(&colorMid, colorEnd, n * 100 / stepCount);
				}
				
				// the fade color floods the reachable dungeon tiles faster
				if (!invert && coordinatesAreInMap(i - STAT_BAR_WIDTH, j - MESSAGE_LINES)
					&& distanceMap[i - STAT_BAR_WIDTH][j - MESSAGE_LINES] >= 0 && distanceMap[i - STAT_BAR_WIDTH][j - MESSAGE_LINES] < 30000) {
					percentComplete *= 1.0 + (100.0 - min(100, distanceMap[i - STAT_BAR_WIDTH][j - MESSAGE_LINES])) / 100.;
				}
				
				weight = (short) percentComplete + weightGrid[i][j][2] * percentComplete * 10;
				weight = min(100, weight);
				tempColor = black;
				
				tempColor.red = ((short) percentComplete + weightGrid[i][j][0] * percentComplete * 10) * colorMid.red / 100;
				tempColor.red = min(colorMid.red, tempColor.red);
				
				tempColor.green = ((short) percentComplete + weightGrid[i][j][1] * percentComplete * 10) * colorMid.green / 100;
				tempColor.green = min(colorMid.green, tempColor.green);
				
				tempColor.blue = ((short) percentComplete + weightGrid[i][j][2] * percentComplete * 10) * colorMid.blue / 100;
				tempColor.blue = min(colorMid.blue, tempColor.blue);
				
				backColor = black;
				
				backColor.red = displayBuf[i][j].backColorComponents[0];
				backColor.green = displayBuf[i][j].backColorComponents[1];
				backColor.blue = displayBuf[i][j].backColorComponents[2];
				
				foreColor = (invert ? white : black);
				
				if (j < MESSAGE_LINES && i >= STAT_BAR_WIDTH && i < STAT_BAR_WIDTH + strlen(displayedMessage[MESSAGE_LINES - j - 1])) {
					tempChar = displayedMessage[MESSAGE_LINES - j - 1][i - STAT_BAR_WIDTH];
				} else {
					tempChar = displayBuf[i][j].character;
					
					foreColor.red = displayBuf[i][j].foreColorComponents[0];
					foreColor.green = displayBuf[i][j].foreColorComponents[1];
					foreColor.blue = displayBuf[i][j].foreColorComponents[2];
					
					applyColorAverage(&foreColor, &tempColor, weight);
				}
				applyColorAverage(&backColor, &tempColor, weight);
				plotCharWithColor(tempChar, i, j, foreColor, backColor);
			}
		}
		if (!fastForward && pauseBrogue(1)) {
			fastForward = true;
			n = (invert ? 1 : stepCount - 2);
		}
	}
	
	freeDynamicGrid(distanceMap);
}

void displayLoops() {
	short i, j;
	color foreColor, backColor;
	uchar dchar;
	
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			if (pmap[i][j].flags & IN_LOOP) {
				getCellAppearance(i, j, &dchar, &foreColor, &backColor);
				applyColorAugment(&backColor, &yellow, 50);
				plotCharWithColor(dchar, i + STAT_BAR_WIDTH, j + MESSAGE_LINES, foreColor, backColor);
				//colorBlendCell(i, j, &tempColor, 100);//hiliteCell(i, j, &tempColor, 100);
			}
		}
	}
}

boolean pauseBrogue(short milliseconds) {
	boolean interrupted;
	
	commitDraws();
	interrupted = pauseForMilliseconds(milliseconds);
	pausingTimerStartsNow();
	return interrupted;
}

void nextBrogueEvent(rogueEvent *returnEvent, boolean colorsDance) {
	
	commitDraws();
	
	if (rogue.creaturesWillFlashThisTurn) {
		displayMonsterFlashes(true);
	}
	
	nextKeyOrMouseEvent(returnEvent, colorsDance);
	pausingTimerStartsNow();
}

void executeMouseClick(rogueEvent *theEvent) {
	short x, y;
	boolean autoConfirm;
	x = theEvent->param1;
	y = theEvent->param2;
	autoConfirm = theEvent->controlKey;
	if (coordinatesAreInMap(x - STAT_BAR_WIDTH, y - MESSAGE_LINES)) {
		travel(x - STAT_BAR_WIDTH, y - MESSAGE_LINES, autoConfirm);
	}
}

void executeKeystroke(unsigned short keystroke, boolean controlKey, boolean shiftKey) {
	short direction = -1;
	
	confirmMessages();
	stripShiftFromMovementKeystroke(&keystroke);
	
	switch (keystroke) {
		case UP_KEY:
		case UP_ARROW:
		case NUMPAD_8:
			direction = UP;
			break;
		case DOWN_KEY:
		case DOWN_ARROW:
		case NUMPAD_2:
			direction = DOWN;
			break;
		case LEFT_KEY:
		case LEFT_ARROW:
		case NUMPAD_4:
			direction = LEFT;
			break;
		case RIGHT_KEY:
		case RIGHT_ARROW:
		case NUMPAD_6:
			direction = RIGHT;
			break;
		case UPLEFT_KEY:
		case NUMPAD_7:
			direction = UPLEFT;
			break;
		case UPRIGHT_KEY:
		case NUMPAD_9:
			direction = UPRIGHT;
			break;
		case DOWNLEFT_KEY:
		case NUMPAD_1:
			direction = DOWNLEFT;
			break;
		case DOWNRIGHT_KEY:
		case NUMPAD_3:
			direction = DOWNRIGHT;
			break;
		case DESCEND_KEY:
			considerCautiousMode();
			useStairs(1);
			refreshSideBar(NULL);
			break;
		case ASCEND_KEY:
			considerCautiousMode();
			useStairs(-1);
			refreshSideBar(NULL);
			break;
		case REST_KEY:
		case PERIOD_KEY:
		case NUMPAD_5:
			considerCautiousMode();
			rogue.justRested = true;
			playerTurnEnded();
			refreshSideBar(NULL);
			break;
		case AUTO_REST_KEY:
			rogue.justRested = true;
			autoRest();
			break;
		case SEARCH_KEY:
			considerCautiousMode();
			search(rogue.awarenessBonus < 0 ? 20 : 50);
			playerTurnEnded();
			refreshSideBar(NULL);
			break;
		case INVENTORY_KEY:
			displayInventory(ALL_ITEMS, 0, 0, true);
			break;
		case EQUIP_KEY:
			equip();
			refreshSideBar(NULL);
			break;
		case UNEQUIP_KEY:
			unequip();
			refreshSideBar(NULL);
			break;
		case DROP_KEY:
			drop();
			refreshSideBar(NULL);
			break;
		case APPLY_KEY:
			apply(NULL);
			refreshSideBar(NULL);
			break;
		case THROW_KEY:
			throwCommand();
			refreshSideBar(NULL);
			break;
		case FIGHT_KEY:
			autoFight(false);
			refreshSideBar(NULL);
			break;
		case FIGHT_TO_DEATH_KEY:
			autoFight(true);
			refreshSideBar(NULL);
			break;
		case CALL_KEY:
			call();
			break;
		case REPEAT_TRAVEL_KEY:
			considerCautiousMode();
			if (rogue.lastTravelLoc[0] != 0 && rogue.lastTravelLoc[1] != 0) {
				travel(rogue.lastTravelLoc[0], rogue.lastTravelLoc[1], (controlKey || shiftKey));
			} else {
				examineMode();
			}
			break;
		case EXAMINE_KEY:
			examineMode();
			break;
		case EXPLORE_KEY:
			considerCautiousMode();
			explore(controlKey ? 1 : 30);
			break;
		case AUTOPLAY_KEY:
			autoPlayLevel(controlKey);
			break;
		case HELP_KEY:
			printHelpScreen();
			break;
		case DISCOVERIES_KEY:
			printDiscoveriesScreen();
			break;
		case SEED_KEY:
			/*DEBUG {
				cellDisplayBuffer dbuf[COLS][ROWS];
				copyDisplayBuffer(dbuf, displayBuffer);
				funkyFade(dbuf, &white, 0, 100, player.xLoc + STAT_BAR_WIDTH, player.yLoc + MESSAGE_LINES, false);
			}*/
			// DEBUG victory();
			// DEBUG showWaypoints();
			// DEBUG displayLoops();
			// DEBUG spawnDungeonFeature(player.xLoc, player.yLoc, &dungeonFeatureCatalog[DF_METHANE_GAS_ARMAGEDDON], true);
			// DEBUG spawnDungeonFeature(player.xLoc, player.yLoc, &dungeonFeatureCatalog[DF_LICHEN_GROW], true);
			// DEBUG printStringWithWrapping("Then there is attraction, or the state of being in love (what is sometimes known as \
romantic or obsessive love). This is a refinement of mere lust that allows people to home in on a particular mate. This state is \
characterised by feelings of exhilaration, and intrusive, obsessive thoughts about the object of one's affection. Some researchers \
suggest this mental state might share neurochemical characteristics with the manic phase of manic depression. Dr Fisher's work, \
however, suggests that the actual behavioural patterns of those in love -- such as attempting to evoke reciprocal responses in \
one's loved one -- resemble obsessive compulsive disorder (OCD).\n...\nWonderful though it is, romantic love is unstable -- not \
a good basis for child-rearing. But the final stage of love, long-term attachment, allows parents to cooperate in raising \
children. This state, says Dr Fisher, is characterised by feelings of calm, security, social comfort and emotional union.", 25, 5, 65, &black, &white, 0);
			printSeed();
			break;
		case EASY_MODE_KEY:
			enableEasyMode();
			break;
		default:
			break;
	}
	if (direction >= 0) { // if it was a movement command
		considerCautiousMode();
		if (controlKey || shiftKey) {
			playerRuns(direction);
		} else {
			playerMoves(direction);
		}
		refreshSideBar(NULL);
		if (D_SAFETY_VISION) {
			displaySafetyMap();
		}
	}
	
	rogue.cautiousMode = false;
}

boolean getInputTextString(char *inputText, char *prompt, short maxLength) {
	short charNum, charStartNum, i;
	char keystroke;
	confirmMessages();
	message(prompt, true, false);
	
	for (i=0; i<maxLength; i++) {
		inputText[i] = ' ';
	}
	
	charNum = 0;
	charStartNum = strlen(prompt) + STAT_BAR_WIDTH;
	maxLength = min(maxLength, charStartNum + COLS);
	
	do {
		plotCharWithColor(inputText[charNum], charStartNum + charNum, MESSAGE_LINES - 1, white, white);
		keystroke = nextKeyPress();
		if (keystroke >= ' ' && keystroke <= '~') {
			inputText[charNum] = keystroke;
			plotCharWithColor(keystroke, charStartNum + charNum, MESSAGE_LINES - 1, white, black);
			if (charNum < maxLength) {
				charNum++;
			}
		} else if (keystroke == DELETE_KEY && charNum > 0) {
			plotCharWithColor(' ', charStartNum + charNum, MESSAGE_LINES - 1, black, black);
			charNum--;
			inputText[charNum] = ' ';
		}
	} while (keystroke != RETURN_KEY && keystroke != ESCAPE_KEY);
	
	inputText[charNum] = '\0';
	
	if (keystroke == ESCAPE_KEY) {
		return false;
	}
	return true;
}

#define PRESS_SPACE_TO_CONTINUE_TEXT			" -- Press space to continue -- "
#define PRESS_SPACE_TO_CONTINUE_TEXT_LENGTH		strlen(PRESS_SPACE_TO_CONTINUE_TEXT)

void waitForAcknowledgment() {
	boolean fastForward;
	unsigned short int key = 0;
	short i, j, x;
	color backColors[PRESS_SPACE_TO_CONTINUE_TEXT_LENGTH], backColor, foreColor;
	cellDisplayBuffer dbufs[PRESS_SPACE_TO_CONTINUE_TEXT_LENGTH];
	uchar dchar;
	
	if (rogue.autoPlayingLevel) {
		return;
	}
	
	while (key != ACKNOWLEDGE_KEY && key != ESCAPE_KEY) {
		key = nextKeyPress();
		if (key != ACKNOWLEDGE_KEY && key != ESCAPE_KEY) {
			fastForward = false;
			x = (COLS - PRESS_SPACE_TO_CONTINUE_TEXT_LENGTH) / 2;
			for (i=0; i < 100 && fastForward == false; i++) {
				for (j=0; j<PRESS_SPACE_TO_CONTINUE_TEXT_LENGTH; j++) {
					if (i==0) {
						backColors[j] = colorFromComponents(displayBuffer[j + x][ROWS / 2].backColorComponents);
						dbufs[j] = displayBuffer[j + x][ROWS / 2];
					}
					backColor = backColors[j];
					applyColorAverage(&backColor, &black, 100 - i);
					if (i < 50) {
						dchar = PRESS_SPACE_TO_CONTINUE_TEXT[j];
						foreColor = teal;
						applyColorAverage(&foreColor, &backColor, i * 2);
					} else {
						dchar = dbufs[j].character;
						foreColor = colorFromComponents(dbufs[j].foreColorComponents);
						applyColorAverage(&foreColor, &backColor, (100 - i) * 2);
					}
					plotCharWithColor(dchar, j+x, ROWS/2, foreColor, backColor);
				}
				fastForward = pauseBrogue(5);
			}
			for (j=0; j<PRESS_SPACE_TO_CONTINUE_TEXT_LENGTH; j++) {
				plotCharWithColor(dbufs[j].character, j+x, ROWS/2, colorFromComponents(dbufs[j].foreColorComponents), backColors[j]);
			}
		}
	}
}

boolean confirm(char *prompt) {
	char keystroke;
	
	if (rogue.autoPlayingLevel) {
		return true; // oh yes he did
	}
	
	message(prompt, true, false);
	keystroke = nextKeyPress();
	confirmMessages();
	if (keystroke == 'Y' || keystroke == 'y') {
		return true;
	}
	return false;
}

void displayMonsterFlashes(boolean flashingEnabled) {
	creature *monst;
	short x[100], y[100], strength[100], count = 0;
	color *flashColor[100];
	
	rogue.creaturesWillFlashThisTurn = false;
	
	if (rogue.autoPlayingLevel || rogue.blockCombatText) {
		return;
	}
	
	CYCLE_MONSTERS_AND_PLAYERS(monst) {
		if (monst->bookkeepingFlags & MONST_WILL_FLASH) {
			monst->bookkeepingFlags &= ~MONST_WILL_FLASH;
			if (flashingEnabled && canSeeMonster(monst)) {
				x[count] = monst->xLoc;
				y[count] = monst->yLoc;
				strength[count] = monst->flashStrength;
				flashColor[count] = &(monst->flashColor);
				count++;
			}
		}
	}
	flashForeground(x, y, flashColor, strength, count, 250);
}

void temporaryMessage(char *msg1, boolean requireAcknowledgment) {
	char message[COLS];
	short i, j;
	
	strcpy(message, msg1);
	
	upperCase(message);
	
	refreshSideBar(NULL);
	
	for (i=0; i<MESSAGE_LINES; i++) {
		for (j=0; j<DCOLS; j++) {
			plotCharWithColor(' ', j + STAT_BAR_WIDTH, i, black, black);
		}
	}
	printString(message, STAT_BAR_WIDTH, MESSAGE_LINES-1, &white, &black, 0);
	if (requireAcknowledgment) {
		waitForAcknowledgment();
		updateMessageDisplay();
	}
}

void message(char *msg1, boolean primaryMessage, boolean requireAcknowledgment) {
	char message[COLS];
	short i;
	
	for (i=0; i < DCOLS && msg1[i] != '\0'; i++) {
		message[i] = msg1[i];
	}
	message[i] = '\0';
	
	upperCase(message);
	
	if (primaryMessage) {
		rogue.disturbed = true;
		refreshSideBar(NULL);
		displayCombatText();
		
		// need to confirm the oldest message?
		if (!messageConfirmed[MESSAGE_LINES - 1]) {
			displayMoreSign();
			for (i=0; i<MESSAGE_LINES; i++) {
				messageConfirmed[i] = true;
			}
		}
		
		for (i = MESSAGE_LINES - 1; i >= 1; i--) {
			messageConfirmed[i] = messageConfirmed[i-1];
			strcpy(displayedMessage[i], displayedMessage[i-1]);
		}
		messageConfirmed[0] = false;
		strcpy(displayedMessage[0], message);
		
		// display the message:
		updateMessageDisplay();
		
		if (requireAcknowledgment || rogue.cautiousMode) {
			displayMoreSign();
			confirmMessages();
			rogue.cautiousMode = false;
		}
	} else {
		//return;
		// flavor text
		printString(message, STAT_BAR_WIDTH, ROWS - 1, &teal, &black, 0);
		for (i = strlen(message); i < DCOLS; i++) {
			plotCharWithColor(' ', i + STAT_BAR_WIDTH, ROWS - 1, black, black);
		}
	}
}

void displayMoreSign() {
	short i;
	
	if (rogue.autoPlayingLevel) {
		return;
	}
	
	if (strlen(displayedMessage[0]) < DCOLS - 8 || messageConfirmed[0]) {
		printString("--MORE--", COLS - 8, MESSAGE_LINES-1, &black, &white, 0);
		waitForAcknowledgment();
		printString("        ", COLS - 8, MESSAGE_LINES-1, &black, &black, 0);
	} else {
		printString("--MORE--", COLS - 8, MESSAGE_LINES, &black, &white, 0);
		waitForAcknowledgment();
		for (i=1; i<=8; i++) {
			refreshDungeonCell(DCOLS - i, 0);
		}
	}
}

void updateMessageDisplay() {
	short i, j;
	color messageColor;
	
	for (i=0; i<MESSAGE_LINES; i++) {
		if (messageConfirmed[i]) {
			messageColor = gray;
			applyColorAverage(&messageColor, &black, 75 * i / MESSAGE_LINES);
		} else {
			messageColor = white;
		}
		
		for (j = 0; displayedMessage[i][j]; j++) {
			plotCharWithColor(displayedMessage[i][j], j + STAT_BAR_WIDTH, MESSAGE_LINES - i - 1,
							  messageColor,
							  black);
		}
		for (; j < DCOLS; j++) {
			plotCharWithColor(' ', j + STAT_BAR_WIDTH, MESSAGE_LINES - i - 1, black, black);
		}
	}
}

void deleteMessages() {
	short i;
	for (i=0; i<MESSAGE_LINES; i++) {
		displayedMessage[i][0] = '\0';
	}
	confirmMessages();
}

void confirmMessages() {
	short i;
	for (i=0; i<MESSAGE_LINES; i++) {
		messageConfirmed[i] = true;
	}
	updateMessageDisplay();
}

void stripShiftFromMovementKeystroke(unsigned short *keystroke) {
	unsigned short newKey = *keystroke - ('A' - 'a');
	if (newKey == LEFT_KEY
		|| newKey == RIGHT_KEY
		|| newKey == DOWN_KEY
		|| newKey == UP_KEY
		|| newKey == UPLEFT_KEY
		|| newKey == UPRIGHT_KEY
		|| newKey == DOWNLEFT_KEY
		|| newKey == DOWNRIGHT_KEY) {
		*keystroke -= 'A' - 'a';
	}
}

void upperCase(char *theChar) {
	if (*theChar >= 'a' && *theChar <= 'z') {
		(*theChar) += ('A' - 'a');
	}
}

void refreshSideBar(creature *focusMonst) {
	short printY, shortestDistance, i, x, y;
	creature *monst, *closestMonst;
	//item *theItem, *closestItem;
	char buf[COLS];
	
	if (rogue. gameHasEnded) {
		return;
	}
	
	printY = 0;
	
	x = player.xLoc;
	y = player.yLoc;
	
	printY = printMonsterInfo(&player, printY, (focusMonst ? true : false));
	
	for (monst = monsters->nextCreature; monst != NULL; monst = monst->nextCreature) {
		monst->bookkeepingFlags &= ~MONST_ADDED_TO_STATS_BAR;
	}
	
	if (focusMonst) {
		printY = printMonsterInfo(focusMonst, printY, false);
		focusMonst->bookkeepingFlags |= MONST_ADDED_TO_STATS_BAR;
	}
	
	do {
		shortestDistance = 10000;
		for (monst = monsters->nextCreature; monst != NULL; monst = monst->nextCreature) {
			if (canSeeMonster(monst) && !(monst->bookkeepingFlags & MONST_ADDED_TO_STATS_BAR)
				&& (x - monst->xLoc) * (x - monst->xLoc) + (y - monst->yLoc) * (y - monst->yLoc) < shortestDistance) {
				shortestDistance = (x - monst->xLoc) * (x - monst->xLoc) + (y - monst->yLoc) * (y - monst->yLoc);
				closestMonst = monst;
			}
		}
		if (shortestDistance < 10000) {
			printY = printMonsterInfo(closestMonst, printY, (focusMonst ? true : false));
			closestMonst->bookkeepingFlags |= MONST_ADDED_TO_STATS_BAR;
		}
	} while (shortestDistance < 10000 && printY < ROWS);
	
	for (i=printY; i< ROWS - 1; i++) {
		printString("                    ", 0, i, &white, &black, 0);
	}
	sprintf(buf, "  -- Depth: %i --%s   ", rogue.depthLevel, (rogue.depthLevel < 10 ? " " : ""));
	printString(buf, 0, ROWS - 1, &white, &black, 0);
}

void printString(char *theString, short x, short y, color *foreColor, color*backColor, cellDisplayBuffer dbuf[COLS][ROWS]) {
	short i;
	for (i=0; theString[i] != '\0' && x+i <= COLS; i++) {
		if (dbuf) {
			plotCharToBuffer(theString[i], x+i, y, foreColor, backColor, dbuf);
		} else {
			plotCharWithColor(theString[i], x+i, y, *foreColor, *backColor);
		}
	}
}

// returns the y-coordinate of the last line
short printStringWithWrapping(char *theString, short x, short y, short width, color *foreColor,
							 color*backColor, cellDisplayBuffer dbuf[COLS][ROWS]) {
	char printString[COLS * ROWS];
	short i, scanChar, lineWidth, textLength,
	px, py;
	
	strcpy(printString, theString);
	textLength = strlen(printString);
	
	// first go through and replace spaces with newlines as needed
	for (scanChar = 0, lineWidth= 0;
		 scanChar < textLength;
		 scanChar++) {
		
		if (lineWidth > width) {
			for (i=scanChar; i >= scanChar - lineWidth; i--) {
				if (printString[i] == ' ') {
					printString[i] = '\n';
					lineWidth = 0;
					scanChar = i + 1;
					break;
				}
			}
		}
		
		if (scanChar > 0 && printString[scanChar - 1] == '\n') {
			lineWidth = 0;
		} else {
			lineWidth++;
		}
	}
	
	// now display the string
	px = x;
	py = y;
	
	for (i=0; printString[i] != '\0'; i++) {
		if (printString[i] == '\n') {
			px = x;
			if (py++ >= ROWS) {
				py--;
				break;
			}
		} else if (dbuf) {
			plotCharToBuffer(printString[i], px++, py, foreColor, backColor, dbuf);
		} else {
			plotCharWithColor(printString[i], px++, py, *foreColor, *backColor);
		}
	}
	return py;
}

char nextKeyPress() {
	rogueEvent theEvent;
	do {
		nextBrogueEvent(&theEvent, false);
	} while (theEvent.eventType != KEYSTROKE);
	return theEvent.param1;
}

void printHelpScreen() {
	short i, j;
	char helpText[25][80] = {
		"Commands:",
		"hjklyubn, arrow keys, or numpad:              move or attack",
		"<control> + hjklyubn, arrow keys, or numpad:  run",
		"            a: apply or activate an item (eat, read, zap)",
		"            e: equip an item (armor, weapon or ring)",
		"            r: remove an item (armor, weapon or ring)",
		"            d: drop an item",
		"            t: throw an item",
		"            c: call an item something (i.e. name it)",
		"            z: rest (do nothing for one turn)",
		"            Z: sleep (rest until better or interrupted)",
		"            s: search for secret doors and traps",
		"            >: descend a flight of stairs",
		"            <: ascend a flight of stairs",
		"            f: fight monster (shift-f: fight to the death)",
		"            D: display discovered items",
		"            x: examine your surroundings",
		"            X: auto-explore the level (control-X: fast forward)",
		"            A: autopilot (control-A: fast forward)",
		"     <return>: travel to previous travel destination",
		"      <space>: clear message",
		"        <esc>: cancel a command",
		"<mouse click>: travel to location (control-click: auto-confirm)",
		" ",
		"        -- press space to continue --"
	};
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<ROWS; j++) {
			plotCharWithColor(' ', i + STAT_BAR_WIDTH, j, black, black);
		}
	}
	for (i=0; i<25; i++) {
		printString(helpText[i], 1 + STAT_BAR_WIDTH, i + MESSAGE_LINES, &white, &black, 0);
	}
	waitForAcknowledgment();
	displayLevel();
	updateFlavorText();
	updateMessageDisplay();
}

void printDiscoveries(itemTable *theTable, short count, unsigned short itemCharacter, short x, short y) {
	color *theColor;
	char buf[COLS];
	short i;
	
	for (i = 0; i < count; i++) {
		if (theTable[i].identified) {
			theColor = &white;
			plotCharWithColor(itemCharacter, x, y + i, yellow, black);
		} else {
			theColor = &darkGray;
		}
		strcpy(buf, theTable[i].name);
		upperCase(buf);
		printString(buf, x + 2, y+i, theColor, &black, 0);
	}
}

void printDiscoveriesScreen() {
	short i, j, y;
	
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS + 1; j++) {
			plotCharWithColor(' ', i + STAT_BAR_WIDTH, j + MESSAGE_LINES, black, black);
		}
	}
	
	printString("-- SCROLLS --", STAT_BAR_WIDTH + 3, y = MESSAGE_LINES + 1, &teal, &black, 0);
	printDiscoveries(scrollTable, NUMBER_SCROLL_KINDS, SCROLL_CHAR, STAT_BAR_WIDTH + 3, ++y);

	printString("-- WANDS --", STAT_BAR_WIDTH + 3, y += NUMBER_SCROLL_KINDS + 1, &teal, &black, 0);
	printDiscoveries(wandTable, NUMBER_WAND_KINDS, WAND_CHAR, STAT_BAR_WIDTH + 3, ++y);
	
	printString("-- POTIONS --", STAT_BAR_WIDTH + 27, y = MESSAGE_LINES + 1, &teal, &black, 0);
	printDiscoveries(potionTable, NUMBER_POTION_KINDS, POTION_CHAR, STAT_BAR_WIDTH + 27, ++y);
	
	printString("-- STAFFS --", STAT_BAR_WIDTH + 50, y = MESSAGE_LINES + 1, &teal, &black, 0);
	printDiscoveries(staffTable, NUMBER_STAFF_KINDS, STAFF_CHAR, STAT_BAR_WIDTH + 50, ++y);
	
	printString("-- RINGS --", STAT_BAR_WIDTH + 50, y += NUMBER_STAFF_KINDS + 1, &teal, &black, 0);
	printDiscoveries(ringTable, NUMBER_RING_KINDS, RING_CHAR, STAT_BAR_WIDTH + 50, ++y);
	
	displayMoreSign();
	confirmMessages();
	displayLevel();
}

void printHighScores(boolean hiliteMostRecent) {
	short i, j, hiliteLineNum, maxLength = 0, leftOffset;
	rogueHighScoresEntry list[25];
	char buf[DCOLS];
	color scoreColor;
	
	hiliteLineNum = getHighScoresList(list);
	
	if (!hiliteMostRecent) {
		hiliteLineNum = -1;
	}
	
	for (i=0; i<COLS; i++) {
		for (j=0; j<ROWS; j++) {
			plotCharWithColor(' ', i, j, black, black);
		}
	}
	
	for (i = 0; i < 25 && list[i].score > 0; i++) {
		if (strlen(list[i].description) > maxLength) {
			maxLength = strlen(list[i].description);
		}
	}
	
	leftOffset = min(COLS - maxLength - 21 - 1, COLS/5);
	
	printString("-- HIGH SCORES --", (COLS - 17 + 1) / 2, 1, &yellow, &black, 0);
	
	for (i = 0; i < 25 && list[i].score > 0; i++) {
		scoreColor = white;
		applyColorAverage(&scoreColor, &black, (i * 50 / 24));
		
		// number
		sprintf(buf, "%s%i)", (i + 1 < 10 ? " " : ""), i + 1);
		printString(buf, leftOffset, i + 3, (i == hiliteLineNum ? &teal : &scoreColor), &black, 0);
		
		// score
		sprintf(buf, "%li", list[i].score);
		printString(buf, leftOffset + 5, i + 3, (i == hiliteLineNum ? &teal : &scoreColor), &black, 0);
		
		// date
		printString(list[i].date, leftOffset + 12, i + 3, (i == hiliteLineNum ? &teal : &scoreColor), &black, 0);
		
		// description
		printString(list[i].description, leftOffset + 21, i + 3, (i == hiliteLineNum ? &teal : &scoreColor), &black, 0);
	}
	
	printString("Choose 'New Game' from the File menu (or press command-N) to play again.", (COLS - 72 + 1) / 2, ROWS - 1, &gray, &black, 0);
}

void showWaypoints() {
	short i, j, k, n, start[2], end[2];
	short coords[DCOLS][2];
	
	for (i = 0; i < numberOfWaypoints; i++) {
		start[0] = waypoints[i].x;
		start[1] = waypoints[i].y;
		for (j = 0; j < waypoints[i].connectionCount; j++) {
			end[0] = waypoints[i].connection[j][0];
			end[1] = waypoints[i].connection[j][1];
			n = getLineCoordinates(coords, start, end);
			for (k = 0; k < n && (coords[k][0] != end[0] || coords[k][1] != end[1]); k++) {
				hiliteCell(coords[k][0], coords[k][1], &yellow, 50);
			}
		}
	}
	
	for (i = 0; i < numberOfWaypoints; i++) {
		hiliteCell(waypoints[i].x, waypoints[i].y, &white, 50);
	}
}

void displaySafetyMap() {
	short i, j, score, topRange, bottomRange;
	color tempColor, foreColor, backColor;
	uchar dchar;
	
	topRange = -30000;
	bottomRange = 30000;
	tempColor = black;
	
	if (!rogue.updatedSafetyMapThisTurn) {
		updateSafetyMap();
	}
	
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			if (cellHasTerrainFlag(i, j, WAYPOINT_BLOCKER) || (safetyMap[i][j] == safetyMap[0][0]) || (i == player.xLoc && j == player.yLoc)) {
				continue;
			}
			if (safetyMap[i][j] > topRange) {
				topRange = safetyMap[i][j];
				//if (topRange == 0) {
					//printf("\ntop is zero at %i,%i", i, j);
				//}
			}
			if (safetyMap[i][j] < bottomRange) {
				bottomRange = safetyMap[i][j];
			}
		}
	}
	
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			if (cellHasTerrainFlag(i, j, OBSTRUCTS_PASSABILITY | LAVA_INSTA_DEATH)
				|| (safetyMap[i][j] == safetyMap[0][0])
				|| (i == player.xLoc && j == player.yLoc)) {
				continue;
			}
			score = 300 - (safetyMap[i][j] - bottomRange) * 300 / max(1, (topRange - bottomRange));
			tempColor.blue = max(min(score, 100), 0);
			score -= 100;
			tempColor.red = max(min(score, 100), 0);
			score -= 100;
			tempColor.green = max(min(score, 100), 0);
			getCellAppearance(i, j, &dchar, &foreColor, &backColor);
			plotCharWithColor(dchar, i + STAT_BAR_WIDTH, j + MESSAGE_LINES, foreColor, tempColor);
			//colorBlendCell(i, j, &tempColor, 100);//hiliteCell(i, j, &tempColor, 100);
		}
	}
	//printf("\ntop: %i; bottom: %i", topRange, bottomRange);
}

void printSeed() {
	char buf[COLS];
	sprintf(buf, "Level sequence ID #%li", rogue.seed);
	message(buf, true, false);	
}

void printProgressBar(short x, short y, char barLabel[COLS], long amtFilled, long amtMax, color *fillColor, boolean dim) {
	char barText[] = "                    "; // string length is 20
	short i, labelOffset;
	color currentFillColor, progressBarColor, darkenedBarColor;
	
	if (y >= ROWS - 1) {
		return;
	}
	
	progressBarColor = *fillColor;
	if (y % 2) {
		applyColorAverage(&progressBarColor, &black, 25);
	}
	
	if (dim) {
		applyColorAverage(&progressBarColor, &black, 50);
		applyColorAverage(&darkenedBarColor, &black, 50);
	}
	
	darkenedBarColor = progressBarColor;
	
	applyColorAverage(&darkenedBarColor, &black, 75);
	
	labelOffset = (20 - strlen(barLabel)) / 2;
	for (i=0; i<strlen(barLabel); i++) {
		barText[i + labelOffset] = barLabel[i];
	}
	
	amtFilled = max(0, min(amtFilled, amtMax));
	
	if (amtMax < 10000000) {
		amtFilled *= 100;
		amtMax *= 100;
	}
	
	for (i=0; i<20; i++) {
		currentFillColor = (i <= (20 * amtFilled / amtMax) ? progressBarColor : darkenedBarColor);
		if (i == 20 * amtFilled / amtMax) {
			applyColorAverage(&currentFillColor, &black, 75 - 75 * (amtFilled % (amtMax / 20)) / (amtMax / 20));
		}
		plotCharWithColor(barText[i], x + i, y, (dim ? gray : white), currentFillColor);
	}
}

// returns the y-coordinate after the last line printed
short printMonsterInfo(creature *monst, short y, boolean dim) {
	char buf[COLS], monstName[COLS];
	uchar monstChar;
	color monstForeColor, monstBackColor;
	long amtFilled, amtMax;
	
	char hallucinationStrings[10][COLS] = {
		"     (Dancing)      ",
		"     (Singing)      ",
		"  (Pontificating)   ",
		"     (Skipping)     ",
		"     (Spinning)     ",
		"      (Crying)      ",
		"     (Laughing)     ",
		"     (Humming)      ",
		"    (Whistling)     ",
		"    (Quivering)     ",
	};
	
	if (y >= ROWS - 1) {
		return ROWS - 1;
	}
	
	printString("                    ", 0, y, &white, &black, 0);
	getCellAppearance(monst->xLoc, monst->yLoc, &monstChar, &monstForeColor, &monstBackColor);
	if (dim) {
		applyColorAverage(&monstForeColor, &black, 50);
		applyColorAverage(&monstBackColor, &black, 50);
	}
	plotCharWithColor(monstChar, 0, y, monstForeColor, monstBackColor);
	monsterName(monstName, monst, false);
	upperCase(monstName);
	
	if (monst == &player) {
		sprintf(buf, ": %s, Level %i", monstName, rogue.experienceLevel);
	} else {
		sprintf(buf, ": %s", monstName);
	}
	printString(buf, 1, y++, (dim ? &gray : &white), &black, 0);
	
	// hit points
	printProgressBar(0, y++, "HP", monst->currentHP, monst->info.maxHP, &blueBar, dim);
	
	if (monst == &player) {
		// strength
		sprintf(buf, "Str: %i/%i", rogue.currentStrength, rogue.maxStrength);
		printProgressBar(0, y++, buf, rogue.currentStrength, rogue.maxStrength, &blueBar, dim);
		
		// nutrition
		if (player.status.nutrition > HUNGER_THRESHOLD) {
			printProgressBar(0, y++, "Nutrition", player.status.nutrition, STOMACH_SIZE, &blueBar, dim);
		} else if (player.status.nutrition > WEAK_THRESHOLD) {
			printProgressBar(0, y++, "Nutrition (Hungry)", player.status.nutrition, STOMACH_SIZE, &blueBar, dim);
		} else if (player.status.nutrition > FAINT_THRESHOLD) {
			printProgressBar(0, y++, "Nutrition (Weak)", player.status.nutrition, STOMACH_SIZE, &blueBar, dim);
		} else {
			printProgressBar(0, y++, "Nutrition (Faint)", player.status.nutrition, STOMACH_SIZE, &blueBar, dim);
		}
		
		// experience
		sprintf(buf, "%i%s      Exp.      %s%i", rogue.experienceLevel, (rogue.experienceLevel < 10 ? " " : ""),
				(rogue.experienceLevel < 9 ? " " : ""), rogue.experienceLevel + 1);
		amtFilled = rogue.experience - (rogue.experienceLevel == 1 ? 0 : levelPoints[rogue.experienceLevel - 2]);
		amtMax = (rogue.experienceLevel == 1 ? levelPoints[0]
				  : levelPoints[rogue.experienceLevel - 1] - levelPoints[rogue.experienceLevel - 2]);
		printProgressBar(0, y++, buf, amtFilled, amtMax, &blueBar, dim);
	}
	
	if (!player.status.hallucinating || monst == &player) {
		if (monst->status.burning) {
			printProgressBar(0, y++, "Burning", monst->status.burning, monst->maxStatus.burning, &redBar, dim);
		}
		if (monst->status.paralyzed) {
			printProgressBar(0, y++, "Paralyzed", monst->status.paralyzed, monst->maxStatus.paralyzed, &redBar, dim);
		}
		if (monst->status.stuck) {
			printProgressBar(0, y++, "Entangled", monst->status.stuck, monst->maxStatus.stuck, &redBar, dim);
		}
		if (monst->status.confused) {
			printProgressBar(0, y++, "Confused", monst->status.confused, monst->maxStatus.confused, &redBar, dim);
		}
		if (monst->status.hasted) {
			printProgressBar(0, y++, "Hasted", monst->status.hasted, monst->maxStatus.hasted, &redBar, dim);
		}
		if (monst->status.slowed) {
			printProgressBar(0, y++, "Slowed", monst->status.slowed, monst->maxStatus.slowed, &redBar, dim);
		}
		if (monst->status.immuneToFire) {
			printProgressBar(0, y++, "Immune to Fire", monst->status.immuneToFire, monst->maxStatus.immuneToFire, &redBar, dim);
		}
		if (monst->status.levitating) {
			printProgressBar(0, y++, (monst == &player ? "Levitating" : "Flying"), monst->status.levitating, monst->maxStatus.levitating, &redBar, dim);
		}
		if (monst->status.blind) {
			printProgressBar(0, y++, "Blind", monst->status.blind, monst->maxStatus.blind, &redBar, dim);
		}
		if (monst->status.hallucinating) {
			printProgressBar(0, y++, "Hallucinating", monst->status.hallucinating, monst->maxStatus.hallucinating, &redBar, dim);
		}
		if (monst->status.darkness) {
			printProgressBar(0, y++, "Darkened", monst->status.darkness, monst->maxStatus.darkness, &redBar, dim);
		}
		if (monst->status.telepathic) {
			printProgressBar(0, y++, "Telepathic", monst->status.telepathic, monst->maxStatus.telepathic, &redBar, dim);
		}
		if (monst->status.magicalFear) {
			printProgressBar(0, y++, "Frightened", monst->status.magicalFear, monst->maxStatus.magicalFear, &redBar, dim);
		}
		if (monst->status.nauseous) {
			printProgressBar(0, y++, "Nauseous", monst->status.nauseous, monst->maxStatus.nauseous, &redBar, dim);
		}
		if (monst->status.poisoned) {
			printProgressBar(0, y++, "Poisoned", monst->status.poisoned, monst->maxStatus.poisoned, &redBar, dim);
		}
		if (monst->status.entranced) {
			printProgressBar(0, y++, "Entranced", monst->status.entranced, monst->maxStatus.entranced, &redBar, dim);
		}
		if (monst->status.discordant) {
			printProgressBar(0, y++, "Discordant", monst->status.discordant, monst->maxStatus.discordant, &redBar, dim);
		}
		if (monst != &player && monst->info.flags & MONST_REFLECT_4) {
			printProgressBar(0, y++, "Reflects Magic", 1, 1, &redBar, dim);
		}
	}
	
	if (monst != &player && !(monst->info.flags & MONST_INANIMATE)) {
		if (player.status.hallucinating) {
			printString(hallucinationStrings[rand_range(0, 9)], 0, y++, (dim ? &gray : &white), &black, 0);
		} else if (monst->bookkeepingFlags & MONST_CAPTIVE && y < ROWS - 1) {
			printString("     (Captive)      ", 0, y++, (dim ? &gray : &white), &black, 0);
		} else if (monst->creatureState == MONSTER_SLEEPING && y < ROWS - 1) {
			printString("     (Sleeping)     ", 0, y++, (dim ? &gray : &white), &black, 0);
		} else if (monst->creatureState == MONSTER_FLEEING && y < ROWS - 1) {
			printString("     (Fleeing)      ", 0, y++, (dim ? &gray : &white), &black, 0);
		} else if ((monst->creatureState == MONSTER_TRACKING_SCENT) && y < ROWS - 1) {
			printString("     (Hunting)      ", 0, y++, (dim ? &gray : &white), &black, 0);
		} else if ((monst->creatureState == MONSTER_WANDERING) && y < ROWS - 1) {
			if ((monst->bookkeepingFlags & MONST_FOLLOWER) && monst->leader && (monst->leader->info.flags & MONST_IMMOBILE)) {
				// follower of an immobile leader -- i.e. a totem
				printString("    (Worshiping)    ", 0, y++, (dim ? &gray : &white), &black, 0);
			} else if ((monst->bookkeepingFlags & MONST_FOLLOWER) && monst->leader && (monst->leader->bookkeepingFlags & MONST_CAPTIVE)) {
				// actually a captor/torturer
				printString("     (Guarding)     ", 0, y++, (dim ? &gray : &white), &black, 0);
			} else {
				printString("    (Wandering)     ", 0, y++, (dim ? &gray : &white), &black, 0);
			}
		} else if ((monst->creatureState == MONSTER_ALLY) && y < ROWS - 1) {
			printString("       (Ally)       ", 0, y++, (dim ? &gray : &white), &black, 0);
		}
	} else if (monst == &player) {
		if (y < ROWS - 1) {
			if (!rogue.armor || rogue.armor->flags & ITEM_IDENTIFIED) {
				sprintf(buf, "Arm: %i Gold: %li             ", player.info.defense / 10, rogue.gold);
			} else {
				sprintf(buf, "Arm: ? Gold: %li              ", rogue.gold);
			}
			buf[20] = '\0';
			printString(buf, 0, y++, (dim ? &gray : &white), &black, 0);
		}
	}
	
	if (y < ROWS - 1) {
		printString("                    ", 0, y++, (dim ? &gray : &white), &black, 0);
	}
	return y;
}

void monsterDetails(char buf[], creature *monst) {
	char monstName[COLS], newText[10*COLS];
	short i, j, combatMath, combatMath2, playerKnownAverageDamage, playerKnownMaxDamage, commaCount;
	boolean anyFlags;
	
	buf[0] = '\0';
	commaCount = 0;
	
	monsterName(monstName, monst, true);
	
	sprintf(newText, "%s\n\n", monsterText[monst->info.monsterID].flavorText);
	upperCase(newText);
	strcat(buf, newText);
	
	if (monst->info.damage.upperBound == 0) {
		sprintf(newText, "%s deals no direct damage.\n\n", monstName);
	} else {
		combatMath = (player.currentHP + monst->info.damage.upperBound - 1) / monst->info.damage.upperBound;
		sprintf(newText, "%s typically hits for %i%% of your maximum HP, and at worst could defeat you in %i hit%s.\n\n",
				monstName,
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
			sprintf(newText, "You deal no direct damage.\n\n");
		} else {
			combatMath = (monst->currentHP + playerKnownMaxDamage - 1) / playerKnownMaxDamage;
			sprintf(newText, "You typically hit for %i%% of %s's maximum HP, and at best could defeat it in %i hit%s.\n\n",
					100 * playerKnownAverageDamage / monst->info.maxHP,
					monstName,
					combatMath,
					(combatMath > 1 ? "s" : ""));
		}
		upperCase(newText);
		strcat(buf, newText);
		
		combatMath2 = hitProbability(&player, monst);
		if (!rogue.armor || (rogue.armor->flags & ITEM_IDENTIFIED)) {
			combatMath = hitProbability(monst, &player);
			sprintf(newText, "%s has a %i%% chance to hit you, and you have a %i%% chance to hit it.",
					monstName, combatMath, combatMath2);
		} else {
			sprintf(newText, "You have a %i%% chance to hit %s.",
					combatMath2, monstName);
		}	
	}
		
	upperCase(newText);
	strcat(buf, newText);
	
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
		strcat(buf, "\n\n");
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

#define MIN_MONSTER_DETAILS_WIDTH	38

void printMonsterDetails(creature *monst, cellDisplayBuffer rbuf[COLS][ROWS]) {
	cellDisplayBuffer dbuf[COLS][ROWS];
	char textBuf[COLS * 10];
	short x, y, width, maxY, i, j, dist;
	
	if (monst->xLoc < DCOLS / 2 - 1) {
		x = monst->xLoc + 10 + STAT_BAR_WIDTH;
		width = (DCOLS - monst->xLoc) - 20;
	} else {
		x = STAT_BAR_WIDTH + 10;
		width = monst->xLoc - 20;
	}
	y = MESSAGE_LINES + 2;
	
	if (width < MIN_MONSTER_DETAILS_WIDTH) {
		x -= (MIN_MONSTER_DETAILS_WIDTH - width) / 2;
		width = MIN_MONSTER_DETAILS_WIDTH;
	}
	
	clearDisplayBuffer(dbuf);
	monsterDetails(textBuf, monst);
	maxY = printStringWithWrapping(textBuf, x, y, width, &white, &black, dbuf);
	
	for (i=0; i<COLS; i++) {
		for (j=0; j<ROWS; j++) {
			
			if (i >= x && i <= x + width && j >= y && j <= maxY) {
				dbuf[i][j].opacity = 85;
			} else {
				dist = 0;
				dist += max(0, max(x - i, i - (x + width)));
				dist += max(0, max(y - j, j - maxY));
				dbuf[i][j].opacity = 85 / dist;
			}
		}
	}
	
	overlayDisplayBuffer(dbuf, rbuf);
}
