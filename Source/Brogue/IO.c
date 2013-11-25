/*
 *  IO.c
 *  Brogue
 *
 *  Created by Brian Walker on 1/10/09.
 *  Copyright 2008-2009. All rights reserved.
 *
 */

#include "Rogue.h"
#include "IncludeGlobals.h"
#include <math.h>

void displayLevel() {
	
	short i, j;
	
	for( i=0; i<DCOLS; i++ ) {
		for( j=0; j<DROWS; j++ ) {
			refreshDungeonCell(i, j);
		}
	}
	// updateDisplay();
}

void getCellAppearance(short x, short y, uchar *returnChar, color *returnForeColor, color *returnBackColor) {	
	short bestPriority;
	uchar cellChar = 0;
	color cellForeColor, cellBackColor, lightMultiplierColor, gasAugmentColor;
	boolean haveForeColor = false, haveBackColor = false;
	short gasAugmentWeight = 0;
	creature *monst;
	item *theItem;
	uchar itemChars[] = {POTION_CHAR, SCROLL_CHAR, FOOD_CHAR, WAND_CHAR,
						STAFF_CHAR, GOLD_CHAR, ARMOR_CHAR, WEAPON_CHAR, RING_CHAR};
	enum dungeonLayers layer;
	
	bestPriority = 10000;
	
	for (layer = 0; layer < NUMBER_TERRAIN_LAYERS; layer++) {
		// gas shows up as a color augment, not directly
		if (pmap[x][y].layers[layer] && layer != GAS && (pmap[x][y].flags & (VISIBLE | CLAIRVOYANT_VISIBLE)
												  || !(tileCatalog[pmap[x][y].layers[layer]].flags & IS_NOT_REMEMBERED))) {
			
			if ((!haveForeColor || tileCatalog[pmap[x][y].layers[layer]].drawPriority < bestPriority) && (tileCatalog[pmap[x][y].layers[layer]].foreColor)) {
				cellForeColor = *(tileCatalog[pmap[x][y].layers[layer]].foreColor);
				haveForeColor = true;
			}
			if ((!haveBackColor || tileCatalog[pmap[x][y].layers[layer]].drawPriority < bestPriority) && (tileCatalog[pmap[x][y].layers[layer]].backColor)) {
				cellBackColor = *(tileCatalog[pmap[x][y].layers[layer]].backColor);
				haveBackColor = true;
			}
			if ((!cellChar || tileCatalog[pmap[x][y].layers[layer]].drawPriority < bestPriority) && (tileCatalog[pmap[x][y].layers[layer]].displayChar)) {
				cellChar = tileCatalog[pmap[x][y].layers[layer]].displayChar;
			}
			if (tileCatalog[pmap[x][y].layers[layer]].drawPriority < bestPriority) {
				bestPriority = tileCatalog[pmap[x][y].layers[layer]].drawPriority;
			}
		}
	}
	
	colorMultiplierFromDungeonLight(x, y, &lightMultiplierColor);
	
	if (pmap[x][y].layers[GAS] && tileCatalog[pmap[x][y].layers[GAS]].backColor
		&& (pmap[x][y].flags & (VISIBLE | CLAIRVOYANT_VISIBLE)
			|| !(tileCatalog[pmap[x][y].layers[GAS]].flags & IS_NOT_REMEMBERED))) {
		gasAugmentColor = *(tileCatalog[pmap[x][y].layers[GAS]].backColor);
		gasAugmentWeight = min(85, 35 + pmap[x][y].volume);
	}
	
	if (pmap[x][y].flags & HAS_MONSTER) {
		monst = monsterAtLoc(x, y);
	}
	
	if (pmap[x][y].flags & HAS_PLAYER) {
		cellChar = player.info.displayChar;
		cellForeColor = *(player.info.foreColor);
	} else if ((pmap[x][y].flags & HAS_ITEM) && (pmap[x][y].flags & ITEM_DETECTED) &&
			   (!(pmap[x][y].flags & (VISIBLE | CLAIRVOYANT_VISIBLE)) ||
				cellHasTerrainFlag(x, y, OBSTRUCTS_ITEMS))) {
		for (theItem = floorItems; (theItem->xLoc != x || theItem->yLoc != y); theItem = theItem->nextItem);
		cellChar = itemMagicChar(theItem);
		cellForeColor = white;
		cellBackColor = black;
	} else if (pmap[x][y].flags & HAS_MONSTER && (pmap[x][y].flags & (VISIBLE | CLAIRVOYANT_VISIBLE)
												  || (monst->info.flags & MONST_IMMOBILE && pmap[x][y].flags & DISCOVERED))
			   && !(monst->info.flags & MONST_INVISIBLE || monst->bookkeepingFlags & MONST_SUBMERGED)) {
		if (player.status.hallucinating > 0 && !(monst->info.flags & MONST_INANIMATE)) {
			cellChar = rand_range('a', 'z') + (rand_range(0, 1) ? 'A' - 'a' : 0);
			cellForeColor = *(monsterCatalog[rand_range(1, NUMBER_MONSTER_KINDS - 1)].foreColor);
		} else {
			cellChar = monst->info.displayChar;
			cellForeColor = *(monst->info.foreColor);
			DEBUG {
				if (monst->bookkeepingFlags & MONST_LEADER) {
					applyColorAverage(&cellBackColor, &purple, 50);
				}
			}
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
			   && (pmap[x][y].flags & (VISIBLE | CLAIRVOYANT_VISIBLE)
				   || (pmap[x][y].flags & (DISCOVERED) && !cellHasTerrainFlag(x, y, MOVES_ITEMS))) ) {
		if (player.status.hallucinating) {
			cellChar = itemChars[rand_range(0, 8)];
			cellForeColor = yellow;
		} else {
			for (theItem = floorItems; (theItem->xLoc != x || theItem->yLoc != y); theItem = theItem->nextItem);
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
	
	if (pmap[x][y].flags & (ITEM_DETECTED) || (pmap[x][y].flags & (HAS_PLAYER) && player.status.blind)
		|| (player.status.telepathic > 0 && pmap[x][y].flags & (HAS_MONSTER) && !(monst->info.flags & MONST_IMMOBILE)
			&& !(pmap[x][y].flags & (VISIBLE | CLAIRVOYANT_VISIBLE)))) {
		// do nothing		
	} else if (!(pmap[x][y].flags & VISIBLE) && pmap[x][y].flags & CLAIRVOYANT_VISIBLE) {
		applyColorMultiplier(&cellForeColor, &clairvoyanceColor);
		applyColorMultiplier(&cellBackColor, &clairvoyanceColor);
	} else if (!(pmap[x][y].flags & DISCOVERED) && pmap[x][y].flags & MAGIC_MAPPED) {
		applyColorMultiplier(&cellForeColor, &magicMapColor);
		applyColorMultiplier(&cellBackColor, &magicMapColor);
	} else if (!(pmap[x][y].flags & VISIBLE)) {
		//desaturate(&cellForeColor, 75);
		//desaturate(&cellBackColor, 75);
		//applyColorAverage(&cellForeColor, &black, 75);
		//applyColorAverage(&cellBackColor, &black, 75);
		if (rogue.inWater) {
			//applyColorMultiplier(&cellForeColor, &deepWaterLightColor);
			//applyColorMultiplier(&cellBackColor, &deepWaterLightColor);
			applyColorAverage(&cellForeColor, &black, 80);
			applyColorAverage(&cellBackColor, &black, 80);
		} else {
			applyColorMultiplier(&cellForeColor, &memoryColor);
			applyColorMultiplier(&cellBackColor, &memoryColor);
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
	*returnChar = cellChar;
	*returnForeColor = cellForeColor;
	*returnBackColor = cellBackColor;
}

void refreshDungeonCell(short x, short y) {
	uchar cellChar;
	color foreColor, backColor;
	getCellAppearance(x, y, &cellChar, &foreColor, &backColor);
	plotCharWithColor(cellChar, x + STAT_BAR_WIDTH, y+1, foreColor, backColor);
}

void applyColorMultiplier(color *baseColor, color *multiplierColor) {
	baseColor->red = baseColor->red * multiplierColor->red / 100;
	baseColor->redRand = baseColor->redRand * multiplierColor->redRand / 100;
	baseColor->green = baseColor->green * multiplierColor->green / 100;
	baseColor->greenRand = baseColor->greenRand * multiplierColor->greenRand / 100;
	baseColor->blue = baseColor->blue * multiplierColor->blue / 100;
	baseColor->blueRand = baseColor->blueRand * multiplierColor->blueRand / 100;
	baseColor->rand = baseColor->rand * multiplierColor->rand / 100;
	baseColor->colorDances *= multiplierColor->colorDances;
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
	//applyColorAugment(&foreColor, hiliteColor, hiliteStrength);
	//applyColorAugment(&backColor, hiliteColor, hiliteStrength);
	plotCharWithColor(displayChar, x + STAT_BAR_WIDTH, y + 1, foreColor, backColor);
}

// takes dungeon coordinates
void hiliteCell(short x, short y, color *hiliteColor, short hiliteStrength) {
	uchar displayChar;
	color foreColor, backColor;
	
	getCellAppearance(x, y, &displayChar, &foreColor, &backColor);
	//applyColorAverage(&foreColor, hiliteColor, hiliteStrength);
	//applyColorAverage(&backColor, hiliteColor, hiliteStrength);
	applyColorAugment(&foreColor, hiliteColor, hiliteStrength);
	applyColorAugment(&backColor, hiliteColor, hiliteStrength);
	plotCharWithColor(displayChar, x + STAT_BAR_WIDTH, y + 1, foreColor, backColor);
}

void colorMultiplierFromDungeonLight(short x, short y, color *editColor) {
	editColor->red = editColor->redRand = tmap[x][y].light[0];
	editColor->green = editColor->greenRand = tmap[x][y].light[1];
	editColor->blue = editColor->blueRand = tmap[x][y].light[2];
	editColor->rand = 0; //(tmap[x][y].light[0] + tmap[x][y].light[1] + tmap[x][y].light[2] / 3);
	editColor->colorDances = false;
}

void plotCharWithColor(uchar inputChar, short xLoc, short yLoc, color cellForeColor, color cellBackColor) {
	short foreRed = cellForeColor.red, foreGreen = cellForeColor.green, foreBlue = cellForeColor.blue,
	backRed = cellBackColor.red, backGreen = cellBackColor.green, backBlue = cellBackColor.blue,
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
		
		plotChar(inputChar, xLoc, yLoc, foreRed, foreGreen, foreBlue, backRed, backGreen, backBlue);
		
		displayBuffer[xLoc][yLoc].character = inputChar;
		displayBuffer[xLoc][yLoc].foreColorComponents[0] = foreRed;
		displayBuffer[xLoc][yLoc].foreColorComponents[1] = foreGreen;
		displayBuffer[xLoc][yLoc].foreColorComponents[2] = foreBlue;
		displayBuffer[xLoc][yLoc].backColorComponents[0] = backRed;
		displayBuffer[xLoc][yLoc].backColorComponents[1] = backGreen;
		displayBuffer[xLoc][yLoc].backColorComponents[2] = backBlue;
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
			for (k=0; k<ROWS; k++) {
				dbuf[i][j].foreColorComponents[k] = 0;
				dbuf[i][j].backColorComponents[k] = 0;
			}
		}
	}
}

/*void funkyFade(color *colorEnd, short stepCount, short x, short y, boolean invert) {
 short i, j, n, weight;
 double x2, y2, x3, y3, percentComplete, phaseShift, colorValue, scalar;
 color tempColor, tempColor2;
 char tempChar;
 short **distanceMap;
 
 distanceMap = allocDynamicGrid();
 calculateDistances(distanceMap, player.xLoc, player.yLoc, OBSTRUCTS_PASSABILITY, 0);
 //logBuffer(distanceMap);
 
 for (n=(invert ? stepCount - 1 : 0); (invert ? n >= 0 : n <= stepCount); n += (invert ? -1 : 1)) {
 //percentComplete = (double) (n) * 100 / stepCount;
 for (i=0; i<COLS; i++) {
 for (j=0; j<ROWS; j++) {
 x2 = (double) ((i - x) * 5.0 / COLS); // was 5 and 2.5
 y2 = (double) ((j - y) * 2.5 / ROWS);
 tempColor = black;
 
 percentComplete = (double) (n) * 100 / stepCount;
 
 phaseShift = (percentComplete + 150) / 100;
 
 if (!invert && coordinatesAreInMap(i - STAT_BAR_WIDTH, j - 1)
 && distanceMap[i - STAT_BAR_WIDTH][j - 1] >= 0 && distanceMap[i - STAT_BAR_WIDTH][j - 1] < 30000) {
 scalar = 1.0 + (100.0 - min(100, distanceMap[i - STAT_BAR_WIDTH][j - 1])) / 100.;
 percentComplete *= scalar;
 }
 
 colorValue = (pow(2, -1*x2*x2) * pow(2, -1*y2*y2) * (.6 + .4 * cos(5*x2*x2) * sin(5*y2*y2))); // can definitely be precomputed
 weight = (short) percentComplete + colorValue * percentComplete * 10;
 weight = min(100, weight);
 //weight = percentComplete;
 
 x3 = x2 * cos(phaseShift) - y2 * sin(phaseShift);
 y3 = x2 * sin(phaseShift) + y2 * cos(phaseShift);
 
 colorValue = (pow(2., -x3*x3) * pow(2., -y3*y3) * (.6 + .4 * sin(5.*x3*x3) * cos(5.*y3*y3)));
 tempColor.red = (short) percentComplete + colorValue * percentComplete * 10;
 
 x3 = x2 * cos(-phaseShift) - y2 * sin(-phaseShift);
 y3 = x2 * sin(-phaseShift) + y2 * cos(-phaseShift);
 
 colorValue = (pow(2, -1*x3*x3) * pow(2, -1*y3*y3) * (.6 + .4 * cos(5*x3*x3) * sin(5*y3*y3)));
 tempColor.green = (short) percentComplete + colorValue * percentComplete * 10;
 
 colorValue = (pow(2, -1*x2*x2) * pow(2, -1*y2*y2) * (.9 + .1 * cos(7*x2*x2) * cos(7*y2*y2)));
 tempColor.blue = (short) percentComplete + colorValue * percentComplete * 10;
 
 colorValue = (pow(2, -1*x2*x2) * pow(2, -1*y2*y2));
 
 if (coordinatesAreInMap(i - STAT_BAR_WIDTH, j - 1) && !invert) {
 hiliteCell(i - STAT_BAR_WIDTH, j - 1, &tempColor, weight);
 } else {
 if (j == 0 && i >= STAT_BAR_WIDTH && i < STAT_BAR_WIDTH + strlen(displayedMessage)) {
 tempChar = displayedMessage[i - STAT_BAR_WIDTH];
 } else {
 tempChar = ' ';
 }
 tempColor2 = black;
 applyColorAverage(&tempColor2, &tempColor, weight);
 plotCharWithColor(tempChar, i, j, (invert ? white : black), tempColor2);
 }
 }
 }
 updateDisplay();
 if (pauseForMilliseconds(1)) {
 n = (invert ? 1 : stepCount - 2);
 }
 }
 freeDynamicGrid(distanceMap);
 }*/

#define bCurve(x)	(((x) * (x) + 11) / (10 * ((x) * (x) + 1)) - 0.1)

// x and y are global coordinates, not within the playing square
void funkyFade(cellDisplayBuffer displayBuf[COLS][ROWS], color *colorStart, color *colorEnd, short stepCount, short x, short y, boolean invert) {
	short i, j, n, weight;
	double x2, y2, weightGrid[COLS][ROWS][3], percentComplete;
	color tempColor, colorMid, foreColor, backColor;
	uchar tempChar;
	short **distanceMap;
	
	distanceMap = allocDynamicGrid();
	calculateDistances(distanceMap, player.xLoc, player.yLoc, OBSTRUCTS_PASSABILITY, 0);
	
	for (i=0; i<COLS; i++) {
		x2 = (double) ((i - x) * 5.0 / COLS);
		for (j=0; j<ROWS; j++) {
			y2 = (double) ((j - y) * 2.5 / ROWS);
			
			weightGrid[i][j][0] = bCurve(x2*x2+y2*y2) * (.7 + .3 * cos(5*x2*x2) * cos(5*y2*y2));
			weightGrid[i][j][1] = bCurve(x2*x2+y2*y2) * (.7 + .3 * sin(5*x2*x2) * cos(5*y2*y2));
			weightGrid[i][j][2] = bCurve(x2*x2+y2*y2);
			
			//weightGrid[i][j][0] = pow(2, -1*x2*x2) * pow(2, -1*y2*y2) * (.6 + .4 * cos(5*x2*x2) * sin(5*y2*y2));
			//weightGrid[i][j][1] = pow(2, -1*x2*x2) * pow(2, -1*y2*y2) * (.6 + .4 * sin(5*x2*x2) * cos(5*y2*y2));
			//weightGrid[i][j][2] = pow(2, -1*x2*x2) * pow(2, -1*y2*y2);
		}	
	}
	
	//	for (i=0; i<COLS; i++) {
	//		for (j=0; j<ROWS; j++) {
	//			tempColor = black;
	//			applyColorAverage(&tempColor, &white, weightGrid[i][j][0] * 100);
	//			plotCharWithColor(' ', i, j, black, tempColor);
	//		}	
	//	}
	//	updateDisplay();
	//	waitForAcknowledgment();
	
	for (n=(invert ? stepCount - 1 : 0); (invert ? n >= 0 : n <= stepCount); n += (invert ? -1 : 1)) {
		for (i=0; i<COLS; i++) {
			for (j=0; j<ROWS; j++) {
				
				percentComplete = (double) (n) * 100 / stepCount;
				
				colorMid = *colorStart;
				if (colorEnd) {
					applyColorAverage(&colorMid, colorEnd, n * 100 / stepCount);
				}
				
				if (!invert && coordinatesAreInMap(i - STAT_BAR_WIDTH, j - 1)
					&& distanceMap[i - STAT_BAR_WIDTH][j - 1] >= 0 && distanceMap[i - STAT_BAR_WIDTH][j - 1] < 30000) {
					percentComplete *= 1.0 + (100.0 - min(100, distanceMap[i - STAT_BAR_WIDTH][j - 1])) / 100.;
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
				
				if (j == 0 && i >= STAT_BAR_WIDTH && i < STAT_BAR_WIDTH + strlen(displayedMessage)) {
					tempChar = displayedMessage[i - STAT_BAR_WIDTH];
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
		updateDisplay();
		if (pauseForMilliseconds(1)) {
			n = (invert ? 1 : stepCount - 2);
		}
	}
	
	freeDynamicGrid(distanceMap);
}

void executeMouseClick(rogueEvent *theEvent) {
	short x, y;
	boolean autoConfirm;
	x = theEvent->param1;
	y = theEvent->param2 - 1;
	autoConfirm = theEvent->controlKey;
	if (coordinatesAreInMap(x - STAT_BAR_WIDTH, y)) {
		travel(x - STAT_BAR_WIDTH, y, autoConfirm);
	}
}

void executeKeystroke(unsigned short keystroke, boolean controlKey) {
	short direction = -1;
	if (messageDisplayed) {
		clearMessage();
	}
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
			useStairs(1);
			refreshSideBar(NULL);
			break;
		case ASCEND_KEY:
			useStairs(-1);
			refreshSideBar(NULL);
			break;
		case REST_KEY:
			rogue.justRested = true;
			playerTurnEnded();
			refreshSideBar(NULL);
			break;
		case AUTO_REST_KEY:
			rogue.justRested = true;
			autoRest();
			break;
		case SEARCH_KEY:
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
			throw();
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
		case HELP_KEY:
			printHelpScreen();
			break;
		case DISCOVERIES_KEY:
			printDiscoveriesScreen();
			break;
		case SEED_KEY:
			//DEBUG {
			//				cellDisplayBuffer dbuf[COLS][ROWS];
			//				copyDisplayBuffer(dbuf, displayBuffer);
			//				funkyFade(dbuf, &white, 0, 100, player.xLoc + STAT_BAR_WIDTH, player.yLoc + 1, false);
			//			}
			// DEBUG victory();
			DEBUG showWaypoints();
			// DEBUG spawnDungeonFeature(player.xLoc, player.yLoc, &dungeonFeatureCatalog[DF_METHANE_GAS_ARMAGEDDON], true);
			printSeed();
			break;
		case EASY_MODE_KEY:
			enableEasyMode();
			break;
		default:
			break;
	}
	if (direction >= 0) { // if it was a movement command
		if (controlKey) {
			playerRuns(direction);
		} else {
			playerMoves(direction);
		}
		refreshSideBar(NULL);
		if (D_SAFETY_VISION) {
			displaySafetyMap();
		}
		// updateDisplay();
	}
}

boolean getInputTextString(char *inputText, char *prompt, short maxLength) {
	short charNum, charStartNum, i;
	char keystroke;
	clearMessage();
	message(prompt, true, false);
	
	for (i=0; i<maxLength; i++) {
		inputText[i] = ' ';
	}
	
	charNum = 0;
	charStartNum = strlen(prompt) + STAT_BAR_WIDTH;
	maxLength = min(maxLength, charStartNum + COLS);
	
	do {
		plotCharWithColor(inputText[charNum], charStartNum + charNum, 0, white, white);
		// updateDisplay();
		keystroke = nextKeyPress();
		if (keystroke >= ' ' && keystroke <= '~') {
			inputText[charNum] = keystroke;
			plotCharWithColor(keystroke, charStartNum + charNum, 0, white, black);
			if (charNum < maxLength) {
				charNum++;
			}
		} else if (keystroke == DELETE_KEY && charNum > 0) {
			plotCharWithColor(' ', charStartNum + charNum, 0, black, black);
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

void waitForAcknowledgment() {
	while (nextKeyPress() != ACKNOWLEDGE_KEY);
}

boolean confirm(char *prompt) {
	char keystroke;
	message(prompt, true, false);
	keystroke = nextKeyPress();
	clearMessage();
	// updateDisplay();
	if (keystroke == 'Y' || keystroke == 'y') {
		return true;
	}
	return false;
}

void message(char *msg1, boolean primaryMessage, boolean requireAcknowledgment) {
	char message[COLS];
	short i;
	
	rogue.disturbed = true;
	
	refreshSideBar(NULL);
	
	if (primaryMessage) {
		displayCombatText();
	}
	
	strcpy(message, msg1);
	
	if (messageDisplayed && primaryMessage) {
		displayMoreSign();
		waitForAcknowledgment();
	}
	upperCase(message);
	for (i=0; message[i] != '\0' && i < COLS - STAT_BAR_WIDTH; i++) {
		plotCharWithColor(message[i], i + STAT_BAR_WIDTH, (primaryMessage ? 0 : ROWS - 1), (primaryMessage ? white : teal), black);
	}
	for (; i< COLS - STAT_BAR_WIDTH; i++) {
		plotCharWithColor(' ', i + STAT_BAR_WIDTH, (primaryMessage ? 0 : ROWS - 1), white, black);
	}
	if (primaryMessage) {
		messageDisplayed = true;
		strcpy(displayedMessage, message);
	}
	// updateDisplay();
	if (requireAcknowledgment) {
		displayMoreSign();
		waitForAcknowledgment();
		clearMessage();
		// updateDisplay();
	}
}

void displayMoreSign() {
	printString("--MORE--", COLS - 8, 0, &black, &white, 0);
	// updateDisplay();
}

void clearMessage() {
	short i, displayedMessageLength;
	displayedMessageLength = strlen(displayedMessage);
	for (i = 0; i < displayedMessageLength; i++) {
		plotCharWithColor(displayedMessage[i], i + STAT_BAR_WIDTH, 0, gray, black);
	}
	for (; i < DCOLS; i++) {
		plotCharWithColor(' ', i + STAT_BAR_WIDTH, 0, black, black);
	}
	messageDisplayed = false;
}

void upperCase(char *theChar) {
	if (*theChar >= 'a' && *theChar <= 'z') {
		(*theChar) += ('A' - 'a');
	}
}

void refreshSideBar(creature *focusMonst) {
	short printY = 1, shortestDistance, i, x, y;
	creature *monst, *closestMonst;
	//item *theItem, *closestItem;
	char buf[COLS];
	
	if (rogue. gameHasEnded) {
		return;
	}
	
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
	for (i=0; theString[i] != '\0' && i <= COLS; i++) {
		if (dbuf) {
			dbuf[x+i][y].foreColorComponents[0] = foreColor->red + rand_range(0, foreColor->redRand) + rand_range(0, foreColor->rand);
			dbuf[x+i][y].foreColorComponents[1] = foreColor->green + rand_range(0, foreColor->greenRand) + rand_range(0, foreColor->rand);
			dbuf[x+i][y].foreColorComponents[2] = foreColor->blue + rand_range(0, foreColor->blueRand) + rand_range(0, foreColor->rand);
			dbuf[x+i][y].backColorComponents[0] = backColor->red + rand_range(0, backColor->redRand) + rand_range(0, backColor->rand);
			dbuf[x+i][y].backColorComponents[1] = backColor->green + rand_range(0, backColor->greenRand) + rand_range(0, backColor->rand);
			dbuf[x+i][y].backColorComponents[2] = backColor->blue + rand_range(0, backColor->blueRand) + rand_range(0, backColor->rand);
			dbuf[x+i][y].character = theString[i];
		} else {
			plotCharWithColor(theString[i], x+i, y, *foreColor, *backColor);
		}
	}
}

char nextKeyPress() {
	rogueEvent theEvent;
	do {
		nextKeyOrMouseEvent(&theEvent);
	} while (theEvent.eventType != KEYSTROKE);
	return theEvent.param1;
}

void printHelpScreen() {
	short i, j;
	char helpText[23][80] = {
		"Commands:",
		"hjklyubn, arrow keys, or numpad:              move or attack",
		"<control> + hjklyubn, arrow keys, or numpad:  run",
		"                         a: apply or activate an item (eat, read, zap)",
		"                         e: equip an item (armor, weapon or ring)",
		"                         r: remove an item (armor, weapon or ring)",
		"                         d: drop an item",
		"                         t: throw an item",
		"                         c: call an item something (i.e. name it)",
		"                         z: rest (do nothing for one turn)",
		"                         Z: sleep (rest until better or interrupted)",
		"                         s: search for secret doors and traps",
		"                         >: descend a flight of stairs",
		"                         <: ascend a flight of stairs",
		"                         f: fight monster",
		"                         F: fight monster to the death",
		"                         D: display discovered items",
		"                   <space>: clear message",
		"                     <esc>: cancel a command",
		"             <mouse click>: travel to specified point",
		" <control> + <mouse click>: travel to specified point without confirmation",
		" ",
		"               -- press space to continue --"
	};
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS + 1; j++) {
			plotCharWithColor(' ', i + STAT_BAR_WIDTH, j, black, black);
		}
	}
	for (i=0; i<20; i++) {
		printString(helpText[i], 1 + STAT_BAR_WIDTH, i+1, &white, &black, 0);
	}
	waitForAcknowledgment();
	displayLevel();
}

void printDiscoveries(itemTable *theTable, short count, short itemCharacter, short x, short y) {
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
			plotCharWithColor(' ', i + STAT_BAR_WIDTH, j, black, black);
		}
	}
	
	printString("-- SCROLLS --", STAT_BAR_WIDTH + 3, y = 2, &teal, &black, 0);
	printDiscoveries(scrollTable, NUMBER_SCROLL_KINDS, SCROLL_CHAR, STAT_BAR_WIDTH + 3, ++y);

	printString("-- WANDS --", STAT_BAR_WIDTH + 3, y += NUMBER_SCROLL_KINDS + 1, &teal, &black, 0);
	printDiscoveries(wandTable, NUMBER_WAND_KINDS, WAND_CHAR, STAT_BAR_WIDTH + 3, ++y);
	
	
	printString("-- POTIONS --", STAT_BAR_WIDTH + 27, y = 2, &teal, &black, 0);
	printDiscoveries(potionTable, NUMBER_POTION_KINDS, POTION_CHAR, STAT_BAR_WIDTH + 27, ++y);
	
	
	printString("-- STAFFS --", STAT_BAR_WIDTH + 50, y = 2, &teal, &black, 0);
	printDiscoveries(staffTable, NUMBER_STAFF_KINDS, STAFF_CHAR, STAT_BAR_WIDTH + 50, ++y);
	
	printString("-- RINGS --", STAT_BAR_WIDTH + 50, y += NUMBER_STAFF_KINDS + 1, &teal, &black, 0);
	printDiscoveries(ringTable, NUMBER_RING_KINDS, RING_CHAR, STAT_BAR_WIDTH + 50, ++y);
	
	displayMoreSign();
	waitForAcknowledgment();
	clearMessage();
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
		sprintf(buf, "%i", list[i].score);
		printString(buf, leftOffset + 5, i + 3, (i == hiliteLineNum ? &teal : &scoreColor), &black, 0);
		
		// date
		printString(list[i].date, leftOffset + 12, i + 3, (i == hiliteLineNum ? &teal : &scoreColor), &black, 0);
		
		// description
		printString(list[i].description, leftOffset + 21, i + 3, (i == hiliteLineNum ? &teal : &scoreColor), &black, 0);
	}
	
	printString("Choose 'New Game' from the File menu to play again.", (COLS - 51 + 1) / 2, ROWS - 1, &gray, &black, 0);
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
	color tempColor;
	
	topRange = -30000;
	bottomRange = 30000;
	tempColor = black;
	
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			if (cellHasTerrainFlag(i, j, WAYPOINT_BLOCKER) || (safetyMap[i][j] == safetyMap[0][0]) || (i == player.xLoc && j == player.yLoc)) {
				continue;
			}
			if (safetyMap[i][j] > topRange) {
				topRange = safetyMap[i][j];
				if (topRange == 0) {
					printf("\ntop is zero at %i,%i", i, j);
				}
			}
			if (safetyMap[i][j] < bottomRange) {
				bottomRange = safetyMap[i][j];
			}
		}
	}
	
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			if (cellHasTerrainFlag(i, j, WAYPOINT_BLOCKER) || (safetyMap[i][j] == safetyMap[0][0]) || (i == player.xLoc && j == player.yLoc)) {
				continue;
			}
			score = 300 - (safetyMap[i][j] - bottomRange) * 300 / max(1, (topRange - bottomRange));
			tempColor.blue = max(min(score, 100), 0);
			score -= 100;
			tempColor.red = max(min(score, 100), 0);
			score -= 100;
			tempColor.green = max(min(score, 100), 0);
			colorBlendCell(i, j, &tempColor, 100);//hiliteCell(i, j, &tempColor, 100);
		}
	}
	printf("\ntop: %i; bottom: %i", topRange, bottomRange);
}

void printSeed() {
	char buf[COLS];
	sprintf(buf, "Level sequence ID #%i", rogue.seed);
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
		amtFilled *= 10000;
		amtMax *= 10000;
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
	char buf[COLS], monstName[COLS], playerLevel[COLS];
	uchar monstChar;
	color monstForeColor, monstBackColor;
	short amtFilled, amtMax;
	
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
	sprintf(playerLevel, ", Level %i", rogue.experienceLevel);
	sprintf(buf, ": %s%s", monstName, (monst == &player ? playerLevel : ""));
	printString(buf, 1, y++, (dim ? &gray : &white), &black, 0);
	
	// hit points
	printProgressBar(0, y++, "HP", monst->currentHP, monst->info.maxHP, &blueBar, dim);
	
	if (monst == &player) {
		// strength
		sprintf(buf, "Str: %i/%i", rogue.currentStrength, rogue.maxStrength);
		printProgressBar(0, y++, buf, rogue.currentStrength, rogue.maxStrength, &blueBar, dim);
		
		// nutrition
		printProgressBar(0, y++, "Nutrition", player.status.nutrition, STOMACH_SIZE, &blueBar, dim);
		
		// experience
		sprintf(buf, "%i%s      Exp.      %s%i", rogue.experienceLevel, (rogue.experienceLevel < 10 ? " " : ""),
				(rogue.experienceLevel < 9 ? " " : ""), rogue.experienceLevel + 1);
		amtFilled = rogue.experience - (rogue.experienceLevel == 1 ? 0 : levelPoints[rogue.experienceLevel - 2]);
		amtMax = (rogue.experienceLevel == 1 ? levelPoints[0]
				  : levelPoints[rogue.experienceLevel] - levelPoints[rogue.experienceLevel - 1]);
		printProgressBar(0, y++, buf, amtFilled, amtMax, &blueBar, dim);
	}
	
	if (monst->status.burning) {
		printProgressBar(0, y++, "Burning", monst->status.burning, monst->maxStatus.burning, &redBar, dim);
	}
	if (monst->status.paralyzed) {
		printProgressBar(0, y++, "Paralyzed", monst->status.paralyzed, monst->maxStatus.paralyzed, &redBar, dim);
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
	if (monst->status.telepathic) {
		printProgressBar(0, y++, "Telepathic", monst->status.telepathic, monst->maxStatus.telepathic, &redBar, dim);
	}
	if (monst->status.magicalFear) {
		printProgressBar(0, y++, "Frightened", monst->status.magicalFear, monst->maxStatus.magicalFear, &redBar, dim);
	}
	if (monst->status.nauseous) {
		printProgressBar(0, y++, "Nauseous", monst->status.nauseous, monst->maxStatus.nauseous, &redBar, dim);
	}
	if (monst->status.entranced) {
		printProgressBar(0, y++, "Entranced", monst->status.entranced, monst->maxStatus.entranced, &redBar, dim);
	}
	
	if (monst != &player && !(monst->info.flags & MONST_INANIMATE)) {
		if (monst->bookkeepingFlags & MONST_CAPTIVE && y < ROWS - 1) {
			printString("     (Captive)      ", 0, y++, (dim ? &gray : &white), &black, 0);
		} else if (monst->creatureState == MONSTER_SLEEPING && y < ROWS - 1) {
			printString("     (Sleeping)     ", 0, y++, (dim ? &gray : &white), &black, 0);
		} else if ((monst->creatureState == MONSTER_FLEEING || monst->creatureState == MONSTER_COWERING)
				   && y < ROWS - 1) {
			printString("     (Fleeing)      ", 0, y++, (dim ? &gray : &white), &black, 0);
		} else if ((monst->creatureState == MONSTER_TRACKING_SCENT)
				   && y < ROWS - 1) {
			printString("     (Hunting)      ", 0, y++, (dim ? &gray : &white), &black, 0);
		} else if ((monst->creatureState == MONSTER_WANDERING)
				   && y < ROWS - 1) {
			printString("    (Wandering)     ", 0, y++, (dim ? &gray : &white), &black, 0);
		} else if ((monst->creatureState == MONSTER_ALLY)
				   && y < ROWS - 1) {
			printString("       (Ally)       ", 0, y++, (dim ? &gray : &white), &black, 0);
		}
	} else if (monst == &player) {
		if (y < ROWS - 1) {
			sprintf(buf, "Arm: %i Gold: %i             ", player.info.defense / 10, rogue.gold);
			buf[20] = '\0';
			printString(buf, 0, y++, (dim ? &gray : &white), &black, 0);
		}
//		if (y < ROWS - 1) {
//			sprintf(buf, "Gold:  %i            ", rogue.gold);
//			buf[20] = '\0';
//			printString(buf, 0, y++, (dim ? &gray : &white), &black, 0);
//		}
	}
	
	if (y < ROWS - 1) {
		printString("                    ", 0, y++, (dim ? &gray : &white), &black, 0);
	}
	return y;
}