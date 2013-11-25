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
#include "Rogue.h"
#include "libtcod.h"

extern playerCharacter rogue;
extern TCOD_renderer_t renderer;
extern short brogueFontSize;
extern short mouseX, mouseY;

typedef struct brogueScoreEntry {
	long int score;
	long int dateNumber; // in seconds
	char dateText[COLS]; // in the form mm/dd/yy
	char description[COLS];
} brogueScoreEntry;

brogueScoreEntry scoreBuffer[25];

TCOD_key_t bufferedKey = {TCODK_NONE};
short bufferedMouseClick[2] = {-1, -1};

TCOD_random_t randomNumberGenerator;

void plotChar(uchar inputChar,
			  short xLoc, short yLoc,
			  short foreRed, short foreGreen, short foreBlue,
			  short backRed, short backGreen, short backBlue) {
	
	TCOD_color_t fore;
	TCOD_color_t back;
	
	fore.r = (uint8) foreRed * 255 / 100;
	fore.g = (uint8) foreGreen * 255 / 100;
	fore.b = (uint8) foreBlue * 255 / 100;
	back.r = (uint8) backRed * 255 / 100;
	back.g = (uint8) backGreen * 255 / 100;
	back.b = (uint8) backBlue * 255 / 100;
	
	if (inputChar > 255) {
		switch (inputChar) {
#ifdef USE_UNICODE
			case FLOOR_CHAR:
				inputChar = 128 + 0;
				break;
			case CHASM_CHAR:
				inputChar = 128 + 1;
				break;
			case TRAP_CHAR:
				inputChar = 128 + 2;
				break;
			case FIRE_CHAR:
				inputChar = 128 + 3;
				break;
			case FOLIAGE_CHAR:
				inputChar = 128 + 4;
				break;
			case AMULET_CHAR:
				inputChar = 128 + 5;
				break;
			case SCROLL_CHAR:
				inputChar = 128 + 6;
				break;
			case RING_CHAR:
				inputChar = 128 + 7;
				break;
			case WEAPON_CHAR:
				inputChar = 128 + 8;
				break;
			case GEM_CHAR:
				inputChar = 128 + 9;
				break;
			case TOTEM_CHAR:
				inputChar = 128 + 10;
				break;
//			case TURRET_CHAR: // same as GEM_CHAR
//				inputChar = 128 + 11;
//				break;
			case BAD_MAGIC_CHAR:
				inputChar = 128 + 12;
				break;
			case GOOD_MAGIC_CHAR:
				inputChar = 128 + 13;
				break;
#endif
			default:
				inputChar = ' ';
				break;
		}
	}
	TCOD_console_put_char_ex(NULL, xLoc, yLoc, (int) inputChar, fore, back);
}

void pausingTimerStartsNow() {
	
}

static void initWithFont(int fontSize) {
	char font[80];
	
	sprintf(font,"BrogueFont%i.png",fontSize);
	
	TCOD_console_set_custom_font(font, (TCOD_FONT_TYPE_GREYSCALE | TCOD_FONT_LAYOUT_ASCII_INROW), 0, 0);
	TCOD_console_init_root(COLS, ROWS, "Brogue", false, renderer);
	TCOD_console_map_ascii_codes_to_font(0, 255, 0, 0);
	TCOD_console_set_keyboard_repeat(175, 30);
	refreshScreen();
}

boolean processSpecialKeystrokes(TCOD_key_t k) {
	
	if (k.vk == TCODK_PRINTSCREEN) {
		// screenshot
		TCOD_sys_save_screenshot(NULL);
		return true;
	} else if ( k.lalt && (k.vk == TCODK_ENTER || k.vk == TCODK_KPENTER) ) {
		// switch fullscreen (DISABLED)
		//TCOD_console_set_fullscreen(!TCOD_console_is_fullscreen());
		return true;
	} else if ((k.vk == TCODK_PAGEUP
				|| (k.vk == TCODK_CHAR && (k.c == '=' || k.c == '+')))
			   && brogueFontSize < 5) {
		brogueFontSize++;
		TCOD_console_delete(NULL);
		//TCODConsole::root=NULL;
		initWithFont(brogueFontSize);
		TCOD_console_flush();
		return true;
	} else if ((k.vk == TCODK_PAGEDOWN
				|| (k.vk == TCODK_CHAR && k.c == '-'))
			   && brogueFontSize > 1) {
		brogueFontSize--;
		TCOD_console_delete(NULL);
		//TCODConsole::root=NULL;
		initWithFont(brogueFontSize);
		TCOD_console_flush();
		return true;
	}
	return false;
}

// returns true if input is acceptable
boolean processKeystroke(TCOD_key_t key, rogueEvent *returnEvent) {
	
	if (processSpecialKeystrokes(key)) {
		return false;
	}
	
	returnEvent->eventType = KEYSTROKE;
	returnEvent->controlKey = (key.rctrl || key.lctrl);
	returnEvent->shiftKey = key.shift;
	switch (key.vk) {
		case TCODK_CHAR:
		case TCODK_0:
		case TCODK_1:
		case TCODK_2:
		case TCODK_3:
		case TCODK_4:
		case TCODK_5:
		case TCODK_6:
		case TCODK_7:
		case TCODK_8:
		case TCODK_9:
			returnEvent->param1 = (unsigned short) key.c;
			if (returnEvent->shiftKey && returnEvent->param1 >= 'a' && returnEvent->param1 <= 'z') {
				returnEvent->param1 += 'A' - 'a';
			}
			break;
		case TCODK_SPACE:
			returnEvent->param1 = ACKNOWLEDGE_KEY;
			break;
		case TCODK_ESCAPE:
			returnEvent->param1 = ESCAPE_KEY;
			break;
		case TCODK_UP:
			returnEvent->param1 = UP_ARROW;
			break;
		case TCODK_DOWN:
			returnEvent->param1 = DOWN_ARROW;
			break;
		case TCODK_RIGHT:
			returnEvent->param1 = RIGHT_ARROW;
			break;
		case TCODK_LEFT:
			returnEvent->param1 = LEFT_ARROW;
			break;
		case TCODK_ENTER:
			returnEvent->param1 = RETURN_KEY;
			break;
		case TCODK_KPENTER:
			returnEvent->param1 = ENTER_KEY;
			break;
		case TCODK_BACKSPACE:
			returnEvent->param1 = DELETE_KEY;
			break;
		case TCODK_TAB:
			returnEvent->param1 = TAB_KEY;
			break;
		case TCODK_KP0:
            returnEvent->param1 = NUMPAD_0;
            break;
		case TCODK_KP1:
            returnEvent->param1 = NUMPAD_1;
            break;
        case TCODK_KP2:
            returnEvent->param1 = NUMPAD_2;
            break;
        case TCODK_KP3:
            returnEvent->param1 = NUMPAD_3;
            break;
        case TCODK_KP4:
            returnEvent->param1 = NUMPAD_4;
            break;
        case TCODK_KP5:
            returnEvent->param1 = NUMPAD_5;
            break;
        case TCODK_KP6:
            returnEvent->param1 = NUMPAD_6;
            break;
        case TCODK_KP7:
            returnEvent->param1 = NUMPAD_7;
            break;
        case TCODK_KP8:
            returnEvent->param1 = NUMPAD_8;
            break;
        case TCODK_KP9:
            returnEvent->param1 = NUMPAD_9;
            break;
		default:
			return false;
	}
	return true;
}

#define PAUSE_BETWEEN_EVENT_POLLING		34//17

void nextKeyOrMouseEvent(rogueEvent *returnEvent, boolean colorsDance) {
	boolean tryAgain;
	TCOD_key_t key;
	TCOD_mouse_t mouse;
	uint32 theTime, waitTime;
	short x, y;
	
	TCOD_console_flush();
	
	for (;;) {
		
		theTime = TCOD_sys_elapsed_milli();
		
		if (TCOD_console_is_window_closed()) {
			rogue.gameHasEnded = true; // causes the game loop to terminate quickly
			returnEvent->eventType = KEYSTROKE;
			returnEvent->param1 = ACKNOWLEDGE_KEY;
			return;
		}
		
		tryAgain = false;
		
		if (bufferedKey.vk != TCODK_NONE && processKeystroke(bufferedKey, returnEvent)) {
			bufferedKey.vk = TCODK_NONE;
			return;
		}
		
		if (bufferedMouseClick[0] >= 0 && bufferedMouseClick[1] >= 0) {
			returnEvent->eventType = MOUSE_UP;
			returnEvent->param1 = bufferedMouseClick[0];
			returnEvent->param2 = bufferedMouseClick[1];
			if (TCOD_console_is_key_pressed(TCODK_CONTROL)) {
				returnEvent->controlKey = true;
			}
			if (TCOD_console_is_key_pressed(TCODK_SHIFT)) {
				returnEvent->shiftKey = true;
			}
			
			bufferedMouseClick[0] = -1;
			return;
		}
		
		if (colorsDance) {
			shuffleTerrainColors(3, true);
			commitDraws();
		}
		
		TCOD_console_flush();
		key = TCOD_console_check_for_keypress(TCOD_KEY_PRESSED);
		if (processKeystroke(key, returnEvent)) {
			return;
		}
		
		mouse = TCOD_mouse_get_status();
		x = mouse.cx;
		y = mouse.cy;
		if (x > 0 && y > 0 && x < COLS && y < ROWS
			&& mouse.lbutton_pressed || mouseX !=x || mouseY != y) {
			returnEvent->param1 = x;
			returnEvent->param2 = y;
			mouseX = x;
			mouseY = y;
			if (TCOD_console_is_key_pressed(TCODK_CONTROL)) {
				returnEvent->controlKey = true;
			}
			if (TCOD_console_is_key_pressed(TCODK_SHIFT)) {
				returnEvent->shiftKey = true;
			}
			if (mouse.lbutton_pressed) {
				returnEvent->eventType = MOUSE_UP;
			} else {
				returnEvent->eventType = MOUSE_ENTERED_CELL;
			}
			return;
		}
		waitTime = PAUSE_BETWEEN_EVENT_POLLING + theTime - TCOD_sys_elapsed_milli();
		
		if (waitTime > 0 && waitTime <= PAUSE_BETWEEN_EVENT_POLLING) {
			TCOD_sys_sleep_milli(waitTime);
		}
	}
}

boolean pauseForMilliseconds(short milliseconds) {
	TCOD_mouse_t mouse;
	TCOD_console_flush();
	TCOD_sys_sleep_milli((unsigned int) milliseconds);
	bufferedKey = TCOD_console_check_for_keypress(TCOD_KEY_PRESSED);
	
	
	mouse = TCOD_mouse_get_status();
	if (mouse.lbutton_pressed) {
		bufferedMouseClick[0] = mouse.cx;
		bufferedMouseClick[1] = mouse.cy;
	}
	
	return (bufferedKey.vk != TCODK_NONE || bufferedMouseClick[0] >= 0);
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

void initializeBrogueSaveLocation() {
    // char path[PATH_MAX];
    // chdir(path);
	return;
}

