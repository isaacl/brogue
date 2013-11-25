/*
 *  RogueMain.c
 *  Brogue
 *
 *  Created by Brian Walker on 12/26/08.
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
#include <time.h>
#include <limits.h>
#include <stdint.h> // C99

#include "Rogue.h"
#include "IncludeGlobals.h"

short randClump(randomRange theRange) {
	return randClumpedRange(theRange.lowerBound, theRange.upperBound, theRange.clumpFactor);
}

// Get a random int between lowerBound and upperBound, inclusive, with probability distribution
// affected by clumpFactor.
short randClumpedRange(short lowerBound, short upperBound, short clumpFactor) {
	if (upperBound <= lowerBound) {
		return lowerBound;
	}
	if (clumpFactor <= 1) {
		return rand_range(lowerBound, upperBound);
	}
	
	short i, total = 0, numSides = (upperBound - lowerBound) / clumpFactor;
	
	for(i=0; i < (upperBound - lowerBound) % clumpFactor; i++) {
		total += rand_range(0, numSides + 1);
	}
	
	for(; i< clumpFactor; i++) {
		total += rand_range(0, numSides);
	}
	
	return (total + lowerBound);
}

// Get a random int between lowerBound and upperBound, inclusive
boolean rand_percent(short percent) {
	return (rand_range(0, 99) < max(0, min(100, percent)));
}

void shuffleList(short *list, short listLength) {
	short i, r, buf;
	for (i=0; i<listLength; i++) {
		r = rand_range(0, listLength-1);
		if (i != r) {
			buf = list[r];
			list[r] = list[i];
			list[i] = buf;
		}
	}
}

///*
// Simple combo, period > 2^60.5
// x(n)=x(n-1)*x(n-2) mod 2^32 added to
// period of x(n)=x(n-1)*x(n-2) is 3*2^29 if both seeds are
// odd, and one is +or-3 mod 8.
// easy to ensure: replace seed x with 8*seed+3, and y with 2*seed+1
// */
//
//static unsigned long combo_x[NUMBER_OF_RNGS] = {3, 3};
//static unsigned long combo_y[NUMBER_OF_RNGS] = {1, 1};
//static unsigned long combo_z[NUMBER_OF_RNGS] = {1, 1};
//static unsigned long combo_v[NUMBER_OF_RNGS] = {0, 0};
//
//void reportRNGState() {
//	printf("\n\nRNG state:\n\t%lu\n\t%lu\n\t%lu\n\t%lu\n\t%lu numbers generated since seeding",
//		   combo_x[0], combo_y[0], combo_z[0], combo_v[0], randomNumbersGenerated);
//}
//
//void seed_rand_combo(unsigned long seed) {
//	short i;
//	if ((seed - 3) / 8 >= ULONG_MAX) {
//		seed /= 10;
//	}
//	for (i=0; i<NUMBER_OF_RNGS; i++) {
//		combo_x[i] = seed * 8 + 3;
//		combo_y[i] = seed * 2 + 1;
//		combo_z[i] = seed | 1;
//		combo_v[i] = 0;
//	}
//#ifdef AUDIT_RNG
//	printf("\n\nSeeding from value %ul:", seed);
//	reportRNGState();
//#endif
//}
//
//unsigned long rand_combo(short RNG) {
//    combo_v[RNG] = combo_x[RNG] * combo_y[RNG];
//    combo_x[RNG] = combo_y[RNG];
//    combo_y[RNG] = combo_v[RNG];
//    combo_z[RNG] = (combo_z[RNG] & 65535U) * 30903U + (combo_z[RNG] >> 16U);
//    return combo_y[RNG] + combo_z[RNG];
//}
//

//typedef unsigned long int  u4;
typedef uint32_t u4;
typedef struct ranctx { u4 a; u4 b; u4 c; u4 d; } ranctx;

static ranctx RNGState[2];

#define rot(x,k) (((x)<<(k))|((x)>>(32-(k))))
u4 ranval( ranctx *x ) {
    u4 e = x->a - rot(x->b, 27);
    x->a = x->b ^ rot(x->c, 17);
    x->b = x->c + x->d;
    x->c = x->d + e;
    x->d = e + x->a;
    return x->d;
}

void raninit( ranctx *x, u4 seed ) {
    u4 i;
    x->a = 0xf1ea5eed, x->b = x->c = x->d = seed;
    for (i=0; i<20; ++i) {
        (void)ranval(x);
    }
}

/* ----------------------------------------------------------------------
 range
 
 returns a number between 0 and N-1
 without any bias.
 
 */

#define RAND_MAX_COMBO ((unsigned long) UINT32_MAX)

int range(int n, short RNG) {
	unsigned long div;
	int r;
	
	div = RAND_MAX_COMBO/n;
	
	do {
		r = ranval(&(RNGState[RNG])) / div;
	} while (r >= n);
	
	return r;
}

// Get a random int between lowerBound and upperBound, inclusive, with uniform probability distribution

#ifdef AUDIT_RNG // debug version
int rand_range(int lowerBound, int upperBound) {
	int retval;
	char RNGMessage[100];
	
	if (upperBound <= lowerBound) {
		return lowerBound;
	}
	retval = lowerBound + range(upperBound-lowerBound+1, rogue.RNG);
	if (rogue.RNG == RNG_SUBSTANTIVE) {
		randomNumbersGenerated++;
		if (randomNumbersGenerated == 3585716 || randomNumbersGenerated == 3588158 || randomNumbersGenerated == 3588163) {
			sprintf(RNGMessage, "\n#%lu, %i to %i: %i", randomNumbersGenerated, lowerBound, upperBound, retval);
			RNGLog(RNGMessage);
		}
	}
	return retval;
}
#else // normal version
int rand_range(int lowerBound, int upperBound) {
	if (upperBound <= lowerBound) {
		return lowerBound;
	}
	if (rogue.RNG == RNG_SUBSTANTIVE) {
		randomNumbersGenerated++;
	}
	return lowerBound + range(upperBound-lowerBound+1, rogue.RNG);
}
#endif

// seeds with the time if called with a parameter of 0; returns the seed regardless.
// All RNGs are seeded simultaneously and identically.
unsigned long seedRandomGenerator(unsigned long seed) {
	if (seed == 0) {
		seed = (unsigned long) time(NULL);
	}
	raninit(&(RNGState[RNG_SUBSTANTIVE]), seed);
	raninit(&(RNGState[RNG_COSMETIC]), seed);
	return seed;
}

