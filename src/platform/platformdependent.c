/*
 *  platformdependent.c
 *  Brogue
 *
 *  Created by Brian Walker on 4/13/10.
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

#include <stdio.h>
#include <string.h>
#include <time.h>
#include "platform.h"

extern playerCharacter rogue;
extern short brogueFontSize;
extern short mouseX, mouseY;

int isFullScreen = false;
int hasMouseMoved = false;

typedef struct brogueScoreEntry {
	long int score;
	long int dateNumber; // in seconds
	char dateText[COLS]; // in the form mm/dd/yy
	char description[COLS];
} brogueScoreEntry;

brogueScoreEntry scoreBuffer[25];

void plotChar(uchar inputChar,
			  short xLoc, short yLoc,
			  short foreRed, short foreGreen, short foreBlue,
			  short backRed, short backGreen, short backBlue) {
	currentConsole.plotChar(inputChar, xLoc, yLoc, foreRed, foreGreen, foreBlue, backRed, backGreen, backBlue);
}

void pausingTimerStartsNow() {
	
}

void nextKeyOrMouseEvent(rogueEvent *returnEvent, boolean colorsDance) {
	currentConsole.nextKeyOrMouseEvent(returnEvent, colorsDance);
}

boolean pauseForMilliseconds(short milliseconds) {
	return currentConsole.pauseForMilliseconds(milliseconds);
}

// creates an empty high scores file
void initScores() {
	short i;
	FILE *scoresFile;
	
	scoresFile = fopen("BrogueHighScores.txt", "w");
	for (i=0; i<25; i++) {
		fprintf(scoresFile, "%li\t%li\t%s", (long) 0, (long) 0, "(empty entry)\n");
	}
	fclose(scoresFile);
}

// sorts the entries of the scoreBuffer global variable by score in descending order;
// returns the sorted line number of the most recent entry
short sortScoreBuffer() {
	short i, j, highestUnsortedLine, mostRecentSortedLine;
	long highestUnsortedScore, mostRecentDate;
	brogueScoreEntry sortedScoreBuffer[25];
	boolean lineSorted[25];
	
	mostRecentDate = 0;
	
	for (i=0; i<25; i++) {
		lineSorted[i] = false;
	}
	
	for (i=0; i<25; i++) {
		highestUnsortedLine = 0;
		highestUnsortedScore = 0;
		for (j=0; j<25; j++) {
			if (!lineSorted[j] && scoreBuffer[j].score >= highestUnsortedScore) {
				highestUnsortedLine = j;
				highestUnsortedScore = scoreBuffer[j].score;
			}
		}
		sortedScoreBuffer[i] = scoreBuffer[highestUnsortedLine];
		lineSorted[highestUnsortedLine] = true;
	}
	
	// copy the sorted list back into scoreBuffer, remember the most recent entry
	for (i=0; i<25; i++) {
		scoreBuffer[i] = sortedScoreBuffer[i];
		if (scoreBuffer[i].dateNumber > mostRecentDate) {
			mostRecentDate = scoreBuffer[i].dateNumber;
			mostRecentSortedLine = i;
		}
	}
	return mostRecentSortedLine;
}

// loads the BrogueHighScores.txt file into the scoreBuffer global variable
// score file format is: score, tab, date in seconds, tab, description, newline.
short loadScoreBuffer() {
	short i;
	FILE *scoresFile;
	time_t rawtime;
	struct tm * timeinfo;
	
	scoresFile = fopen("BrogueHighScores.txt", "r");
	
	if (scoresFile == NULL) {
		initScores();
		scoresFile = fopen("BrogueHighScores.txt", "r");
	}
	
	for (i=0; i<25; i++) {
		// load score and also the date in seconds
		fscanf(scoresFile, "%li\t%li\t", &(scoreBuffer[i].score), &(scoreBuffer[i].dateNumber));
		
		// load description
		fgets(scoreBuffer[i].description, COLS, scoresFile);
		// strip the newline off the end
		scoreBuffer[i].description[strlen(scoreBuffer[i].description) - 1] = '\0';
		
		// convert date to mm/dd/yy format
		rawtime = (time_t) scoreBuffer[i].dateNumber;
		timeinfo = localtime(&rawtime);
		strftime(scoreBuffer[i].dateText, DCOLS, "%m/%d/%y", timeinfo);
	}
	fclose(scoresFile);
	return sortScoreBuffer();
}

// saves the scoreBuffer global variable into the BrogueHighScores.txt file,
// thus overwriting whatever is already there.
// The numerical version of the date is what gets saved; the "mm/dd/yy" version is ignored.
// Does NOT do any sorting.
void saveScoreBuffer() {
	short i;
	FILE *scoresFile;
	
	scoresFile = fopen("BrogueHighScores.txt", "w");
	
	for (i=0; i<25; i++) {
		// save the entry
		fprintf(scoresFile, "%li\t%li\t%s\n", scoreBuffer[i].score, scoreBuffer[i].dateNumber, scoreBuffer[i].description);
	}
	
	fclose(scoresFile);
}

short getHighScoresList(rogueHighScoresEntry returnList[25]) {
	short i, mostRecentLineNumber;
	
	mostRecentLineNumber = loadScoreBuffer();
	
	for (i=0; i<25; i++) {
		returnList[i].score =				scoreBuffer[i].score;
		strcpy(returnList[i].date,			scoreBuffer[i].dateText);
		strcpy(returnList[i].description,	scoreBuffer[i].description);
	}
	
	return mostRecentLineNumber;
}

boolean saveHighScore(rogueHighScoresEntry theEntry) {
	short i, lowestScoreIndex;
	long lowestScore;
	
	loadScoreBuffer();
	
	for (i=0; i<25; i++) {
		if (scoreBuffer[i].score < lowestScore || i == 0) {
			lowestScore = scoreBuffer[i].score;
			lowestScoreIndex = i;
		}
	}
	
	if (lowestScore > theEntry.score) {
		return false;
	}
	
	scoreBuffer[lowestScoreIndex].score	=				theEntry.score;
	scoreBuffer[lowestScoreIndex].dateNumber =			(long) time(NULL);
	strcpy(scoreBuffer[lowestScoreIndex].description,	theEntry.description);
	
	saveScoreBuffer();
	
	return true;
}

// not needed in libtcod
void initializeBrogueSaveLocation() {
    
}

