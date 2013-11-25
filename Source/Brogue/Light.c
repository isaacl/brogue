/*
 *  Light.c
 *  Brogue
 *
 *  Created by Brian Walker on 1/21/09.
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

#include <math.h>
#include "Rogue.h"
#include "IncludeGlobals.h"

void logLights() {
	
	short i, j;
	
	printf("    ");
	for (i=0; i<COLS-2; i++) {
		printf("%i", i % 10);
	}
	printf("\n");
	for( j=0; j<DROWS-2; j++ ) {
		if (j < 10) {
			printf(" ");
		}
		printf("%i: ", j);
		for( i=0; i<DCOLS-2; i++ ) {
			if (tmap[i][j].light[0] == 0) {
				printf(" ");
			} else {
				printf("%i", max(0, tmap[i][j].light[0] / 10 - 1));
			}
		}
		printf("\n");
	}
	printf("\n");
}

void paintLight(lightSource *theLight) {
	short i, j, k, x, y;
	short colorComponents[4], randComponent, lightMultiplier, thisComponent;
	short radius, fadeToPercent;
	char grid[DCOLS][DROWS];
	
	randComponent = theLight->lightColor->rand;
	colorComponents[0] = randComponent + theLight->lightColor->red + rand_range(0, theLight->lightColor->redRand);
	colorComponents[1] = randComponent + theLight->lightColor->green + rand_range(0, theLight->lightColor->greenRand);
	colorComponents[2] = randComponent + theLight->lightColor->blue + rand_range(0, theLight->lightColor->blueRand);
	colorComponents[3] = theLight->maxIntensity;
	
	if (theLight->followsCreature != NULL) {
		x = theLight->followsCreature->xLoc;
		y = theLight->followsCreature->yLoc;
	} else {
		x = theLight->xLoc;
		y = theLight->yLoc;
	}
	
	radius = randClump(theLight->lightRadius);
	
	fadeToPercent = theLight->radialFadeToPercent;
	
	// zero out only the relevant rectangle of the grid
	for (i = max(0, x - radius); i < DCOLS && i < x + radius; i++) {
		for (j = max(0, y - radius); j < DROWS && j < y + radius; j++) {
			grid[i][j] = 0;
		}
	}
	
	getFOVMask(grid, x, y, radius, OBSTRUCTS_VISION, (theLight->passThroughCreatures ? 0 : (HAS_MONSTER | HAS_PLAYER)),
			   (theLight != rogue.minersLight));
	
	for (i = max(0, x - radius); i < DCOLS && i < x + radius; i++) {
		for (j = max(0, y - radius); j < DROWS && j < y + radius; j++) {
			if (grid[i][j]) {
				lightMultiplier = 100 - (100 - fadeToPercent) * (sqrt((i-x) * (i-x) + (j-y) * (j-y)) / (radius));
				for (k=0; k<3; k++) {
					if (tmap[i][j].light[k] < theLight->maxIntensity) {
						thisComponent = colorComponents[k] * lightMultiplier / 100;
						tmap[i][j].light[k] += min(thisComponent, (colorComponents[3] - tmap[i][j].light[k]));
					}
				}
				pmap[i][j].flags &= ~IS_IN_SHADOW;
			}
		}
	}
	
	tmap[x][y].light[0] += colorComponents[0];
	tmap[x][y].light[1] += colorComponents[1];
	tmap[x][y].light[2] += colorComponents[2];
	
	// the miner's light does not dispel IS_IN_SHADOW from the cell that it occupies,
	// so the player can be in shadow despite casting his own light.
	if (theLight != rogue.minersLight) {
		pmap[x][y].flags &= ~IS_IN_SHADOW;
	}
}

void updateLighting() {
	short i, j, k;
	lightSource *theLight, tempLight; // tempLight is NOT a pointer
	enum dungeonLayers layer;
	enum tileType tile;

	// copy Light over oldLight and then zero out Light.
	for (i = 0; i < DCOLS; i++) {
		for (j = 0; j < DROWS; j++) {
			for (k=0; k<3; k++) {
				tmap[i][j].oldLight[k] = tmap[i][j].light[k];
				tmap[i][j].light[k] = 0;
			}
			pmap[i][j].flags |= IS_IN_SHADOW;
		}
	}
	
	// go through all glowing tiles and have them each cast a light of type glowLight
	// with the tile's foreground color at 10% strength.
	for (i = 0; i < DCOLS; i++) {
		for (j = 0; j < DROWS; j++) {
			if (cellHasTerrainFlag(i, j, GLOWS)) {
				for (layer = 0; layer < NUMBER_TERRAIN_LAYERS; layer++) {
					tile = pmap[i][j].layers[layer];
					
					if (tileCatalog[tile].glowLight) {
						tempLight = lightCatalog[tileCatalog[tile].glowLight];
						tempLight.xLoc = i;
						tempLight.yLoc = j;
						paintLight(&tempLight);
					}
				}
			}
		}
	}
	
	// cycle through all light sources and stamp their imprint on Light[][].
	for (theLight = lights->nextLight; theLight != NULL; theLight = theLight->nextLight) {
		paintLight(theLight);
	}
	if (pmap[player.xLoc][player.yLoc].flags & IS_IN_SHADOW) {
		player.info.foreColor = &playerInShadow;
	} else {
		player.info.foreColor = &playerInLight;
	}
}

lightSource *newLight(lightSource *modelLight, short x, short y, creature *followsCreature) {
	lightSource *theLight;
	theLight = (lightSource *) malloc(sizeof(lightSource));
	
	*theLight = *modelLight; // clone the model into the source
	
	if (followsCreature == NULL) {
		theLight->xLoc = x;
		theLight->yLoc = y;
	} else {
		theLight->followsCreature = followsCreature;
	}
	
	theLight->nextLight = lights->nextLight;
	lights->nextLight = theLight;
	return theLight;
}

void deleteLight(lightSource *theLight) {
	lightSource *previousLight;
	for (previousLight = lights; previousLight->nextLight != theLight; previousLight = previousLight->nextLight);
	previousLight->nextLight = theLight->nextLight;
	free(theLight);
}