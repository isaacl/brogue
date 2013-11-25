/*
 *  Recordings.c
 *  Brogue
 *
 *  Created by Brian Walker on 8/8/10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#include <math.h>
#include <time.h>
#include <limits.h>
#include "Rogue.h"
#include "IncludeGlobals.h"

#define RECORDING_HEADER_LENGTH		32	// bytes at the start of the recording file to store global data

#pragma mark Recording functions

void recordChar(unsigned char c) {
	inputRecordBuffer[locationInRecordingBuffer++] = c;
	recordingLocation++;
	if (locationInRecordingBuffer >= INPUT_RECORD_BUFFER) {
		flushBufferToFile();
	}
}

unsigned char compressKeystroke(uchar c) {
	const uchar ucharTable[] = {UP_ARROW, LEFT_ARROW, DOWN_ARROW, RIGHT_ARROW,
		ESCAPE_KEY, RETURN_KEY, ENTER_KEY, DELETE_KEY, TAB_KEY, NUMPAD_0, NUMPAD_1,
		NUMPAD_2, NUMPAD_3, NUMPAD_4, NUMPAD_5, NUMPAD_6, NUMPAD_7, NUMPAD_8, NUMPAD_9};
	short i;
	
	for (i=0; i<19; i++) {
		if (ucharTable[i] == c) {
			return (unsigned char) (128 + i);
		}
	}
	if (c < 256) {
		return (unsigned char) c;
	}
	return UNKNOWN_KEY;
}

void numberToString(unsigned long number, short numberOfBytes, unsigned char *recordTo) {
	short i;
	unsigned long n;
	
	n = number;
	for (i=numberOfBytes - 1; i >= 0; i--) {
		recordTo[i] = n % 256;
		n /= 256;
	}
	if (n > 0) {
		printf("\nError: the number %li does not fit in %i bytes.", number, numberOfBytes);
	}
}

// numberOfBytes can't be greater than 10, but that's cool as arithmetic generally
// isn't supported above 4 bytes (long int)
void recordNumber(unsigned long number, short numberOfBytes) {
	short i;
	unsigned char c[10];
	
	numberToString(number, numberOfBytes, c);
	for (i=0; i<numberOfBytes; i++) {
		recordChar(c[i]);
	}
}

// Events are recorded as follows:
// Keystrokes: Event type, keystroke value, modifier flags. (3 bytes.)
// All other events: Event type, x-coordinate of the event, y-coordinate of the event, modifier flags. (4 bytes.)
void recordEvent(rogueEvent *event) {
	unsigned char c;
	
	if (rogue.playbackMode) {
		return;
	}
	
	recordChar((unsigned char) event->eventType);
	
	if (event->eventType == KEYSTROKE) {
		// record which key
		c = compressKeystroke(event->param1);
		if (c == UNKNOWN_KEY) {
			return;
		}
		recordChar(c);
	} else {
		recordChar((unsigned char) event->param1);
		recordChar((unsigned char) event->param2);
	}
	
	// record the modifier keys
	c = 0;
	if (event->controlKey) {
		c += Fl(1);
	}
	if (event->shiftKey) {
		c += Fl(2);
	}
	recordChar(c);
}

// For convenience.
void recordKeystroke(uchar keystroke, boolean controlKey, boolean shiftKey) {
	rogueEvent theEvent;
	
	if (rogue.playbackMode) {
		return;
	}
	
	theEvent.eventType = KEYSTROKE;
	theEvent.param1 = keystroke;
	theEvent.controlKey = controlKey;
	theEvent.shiftKey = shiftKey;
	recordEvent(&theEvent);
}

// record a series of keystrokes; string must end with a null terminator
void recordKeystrokeSequence(unsigned char *keystrokeSequence) {
	short i;
	for (i=0; keystrokeSequence[i] != '\0'; i++) {
		recordKeystroke(keystrokeSequence[i], false, false);
	}
}

// For convenience.
void recordMouseClick(short x, short y, boolean controlKey, boolean shiftKey) {
	rogueEvent theEvent;
	
	if (rogue.playbackMode) {
		return;
	}
	
	theEvent.eventType = MOUSE_UP;
	theEvent.param1 = x;
	theEvent.param2 = y;
	theEvent.controlKey = controlKey;
	theEvent.shiftKey = shiftKey;
	recordEvent(&theEvent);
}

void writeHeaderInfo() {
	unsigned char c[RECORDING_HEADER_LENGTH];
	short i;
	FILE *recordFile;
	
	for (i=0; i<RECORDING_HEADER_LENGTH; i++) {
		c[i] = 0;
	}
	
	for (i = 0; BROGUE_VERSION_STRING[i] != 0; i++) {
		c[i] = BROGUE_VERSION_STRING[i];
	}
	i = 16;
	numberToString(rogue.seed, 4, &c[i]);
	i += 4;
	numberToString(rogue.turnNumber, 4, &c[i]);
	i += 4;
	numberToString(rogue.howManyDepthChanges, 4, &c[i]);
	i += 4;
	numberToString(lengthOfPlaybackFile, 4, &c[i]);
	i += 4;
	
	if (!fileExists(currentFilePath)) {
		recordFile = fopen(currentFilePath, "wb");
		fclose(recordFile);
	}
	
	recordFile = fopen(currentFilePath, "r+b");
	rewind(recordFile);
	for (i=0; i<RECORDING_HEADER_LENGTH; i++) {
		putc(c[i], recordFile);
	}
	fclose(recordFile);
	
	if (lengthOfPlaybackFile < RECORDING_HEADER_LENGTH) {
		lengthOfPlaybackFile = RECORDING_HEADER_LENGTH;
	}
}

void flushBufferToFile() {
	short i;
	FILE *recordFile;
	
	if (rogue.playbackMode) {
		return;
	}
	
	lengthOfPlaybackFile += locationInRecordingBuffer;
	writeHeaderInfo();
	
	if (locationInRecordingBuffer != 0) {
		
		recordFile = fopen(currentFilePath, "ab");
		
		for (i=0; i<locationInRecordingBuffer; i++) { // TODO: fix
		//for (i=0; i<locationInRecordingBuffer; i++) {
			putc(inputRecordBuffer[i], recordFile);
		}
		
		fclose(recordFile);
		
		locationInRecordingBuffer = 0;
	}
}

#pragma mark Playback functions

void fillBufferFromFile() {
//	short i;
	FILE *recordFile;
	
	recordFile = fopen(currentFilePath, "rb");
	fseek(recordFile, positionInPlaybackFile, SEEK_SET);
	
	fread((void *) inputRecordBuffer, 1, INPUT_RECORD_BUFFER, recordFile);
	
	positionInPlaybackFile = ftell(recordFile);
	fclose(recordFile);
	
	locationInRecordingBuffer = 0;
}

unsigned char recallChar() {
	unsigned char c;
	if (recordingLocation > lengthOfPlaybackFile) {
		return END_OF_RECORDING;
	}
	c = inputRecordBuffer[locationInRecordingBuffer++];
	recordingLocation++;
	if (locationInRecordingBuffer >= INPUT_RECORD_BUFFER) {
		fillBufferFromFile();
	}
	return c;
}

uchar uncompressKeystroke(uchar c) {
	const uchar ucharTable[] = {UP_ARROW, LEFT_ARROW, DOWN_ARROW, RIGHT_ARROW,
		ESCAPE_KEY, RETURN_KEY, ENTER_KEY, DELETE_KEY, TAB_KEY, NUMPAD_0, NUMPAD_1,
		NUMPAD_2, NUMPAD_3, NUMPAD_4, NUMPAD_5, NUMPAD_6, NUMPAD_7, NUMPAD_8, NUMPAD_9};
	
	if (c >= 128 && c <= UNKNOWN_KEY) {
		return ucharTable[c - 128];
	}
	return (uchar) c;
}

unsigned long recallNumber(short numberOfBytes) {
	short i;
	unsigned long n;
	
	n = 0;
	
	for (i=0; i<numberOfBytes; i++) {
		n *= 256;
		n += (unsigned long) recallChar();
	}
	return n;
}

#define OOS_APOLOGY "Playback of the recording has diverged from the originally recorded game.\n\
This could be caused by recording or playing the file on a modified version of Brogue, or it could \
simply be the result of a bug.  (The recording feature is still in beta for this reason.)\n\
If this is a different computer from the one on which the recording was saved, the recording \
might succeed on the original computer."

void playbackPanic() {
	cellDisplayBuffer rbuf[COLS][ROWS];
	
	if (!rogue.playbackOOS) {
		rogue.playbackFastForward = false;
		rogue.playbackPaused = true;
		rogue.playbackOOS = true;
		displayLevel();
		
		message("Playback is out of sync. The file is corrupted.", true, false);
		
		
		
		printTextBox(OOS_APOLOGY, 0, 0, 0, &white, &black, rbuf);
		
		rogue.playbackMode = false;
		displayMoreSign();
		rogue.playbackMode = true;
		
		overlayDisplayBuffer(rbuf, 0);
		
		
		printf("\n\nPlayback panic at location %li!", recordingLocation - 1);
		
		overlayDisplayBuffer(rbuf, 0);
	}
}

void recallEvent(rogueEvent *event) {
	unsigned char c;
	boolean tryAgain;
	
	do {
		tryAgain = false;
		c = recallChar();
		event->eventType = c;
		
		switch (c) {
			case KEYSTROKE:
				// record which key
				event->param1 = uncompressKeystroke(recallChar());
				event->param2 = 0;
				break;
			case SAVED_GAME_LOADED:
				tryAgain = true;
				flashTemporaryAlert(" Saved game loaded ", 1000);
				break;
			case MOUSE_UP:
			case MOUSE_DOWN:
			case MOUSE_ENTERED_CELL:
				event->param1 = recallChar();
				event->param2 = recallChar();
				break;
			case RNG_CHECK:
			case END_OF_RECORDING:
			case EVENT_ERROR:
			default:
				message("Unrecognized event type in playback.", true, false);
				playbackPanic();
				break;
		}
	} while (tryAgain);
	
	// record the modifier keys
	c = recallChar();
	event->controlKey = (c & Fl(1)) ? true : false;
	event->shiftKey =	(c & Fl(2)) ? true : false;
}

void loadNextAnnotation() {
	unsigned long currentReadTurn;
	short i;
	FILE *annotationFile;
	
	if (rogue.nextAnnotationTurn == -1) {
		return;
	}
	
	annotationFile =  fopen(annotationPathname, "r");
	fseek(annotationFile, rogue.locationInAnnotationFile, SEEK_SET);
	
	for (;;) {
		
		// load turn number
		if (fscanf(annotationFile, "%lu\t", &(currentReadTurn)) != 1) {
			if (feof(annotationFile)) {
				rogue.nextAnnotation[0] = '\0';
				rogue.nextAnnotationTurn = -1;
				break;
			} else {
				// advance to the end of the line
				fgets(rogue.nextAnnotation, 5000, annotationFile);
				continue;
			}
		}
		
		// load description
		fgets(rogue.nextAnnotation, 5000, annotationFile);
		
		if (currentReadTurn > rogue.turnNumber ||
			(currentReadTurn <= 1 && rogue.turnNumber <= 1 && currentReadTurn >= rogue.turnNumber)) {
			rogue.nextAnnotationTurn = currentReadTurn;
			
			// strip the newline off the end
			rogue.nextAnnotation[strlen(rogue.nextAnnotation) - 1] = '\0';
			// strip out any gremlins in the annotation
			for (i=0; i<5000 && rogue.nextAnnotation[i]; i++) {
				if (rogue.nextAnnotation[i] < ' '
					|| rogue.nextAnnotation[i] > '~') {
					rogue.nextAnnotation[i] = ' ';
				}
			}
			break;
		}
	}
	rogue.locationInAnnotationFile = ftell(annotationFile);
	fclose(annotationFile);
}

void displayAnnotation() {
	cellDisplayBuffer rbuf[COLS][ROWS];
	
	if (rogue.playbackMode
		&& rogue.turnNumber == rogue.nextAnnotationTurn) {
		
		if (!rogue.playbackFastForward) {
			refreshSideBar(NULL);
			
			printTextBox(rogue.nextAnnotation, player.xLoc, 0, 0, &black, &white, rbuf);
			
			rogue.playbackMode = false;
			displayMoreSign();
			rogue.playbackMode = true;
			
			overlayDisplayBuffer(rbuf, 0);
		}
		
		loadNextAnnotation();
	}
}

#pragma mark Hybrid and miscellaneous

// creates a game recording file, or if in playback mode,
// initializes based on and starts reading from the recording file
void initRecording() {
	short i;
	char versionString[16], buf[100];
	FILE *recordFile;
	
	initializeBrogueSaveLocation();
		
#ifdef AUDIT_RNG
		if (fileExists(RNG_LOG)) {
			remove(RNG_LOG);
		}
		RNGLogFile = fopen(RNG_LOG, "a");
#endif
		
	locationInRecordingBuffer	= 0;
	positionInPlaybackFile		= 0;
	recordingLocation			= 0;
	maxLevelChanges				= 0;
	rogue.playbackOOS			= false;
	rogue.playbackOmniscience	= false;
	rogue.nextAnnotationTurn	= 0;
	rogue.nextAnnotation[0]		= '\0';
	rogue.locationInAnnotationFile	= 0;
	
	if (rogue.playbackMode) {
		lengthOfPlaybackFile		= 100000; // so recall functions don't freak out
		rogue.playbackDelayPerTurn	= DEFAULT_PLAYBACK_DELAY;
		rogue.playbackDelayThisTurn	= rogue.playbackDelayPerTurn;
		rogue.playbackPaused		= false;
		
		fillBufferFromFile();
		
		for (i=0; i<16; i++) {
			versionString[i] = recallChar();
		}
		
		if (strcmp(versionString, BROGUE_VERSION_STRING)) {
			rogue.playbackMode = false;
			sprintf(buf, "This file is from version %s and cannot be opened in version %s.", versionString, BROGUE_VERSION_STRING);
			message(buf, true, true);
			rogue.playbackMode = true;
			rogue.playbackFastForward = false;
			rogue.playbackPaused = true;
			rogue.playbackOOS = true;
		}
		rogue.seed				= recallNumber(4);			// master random seed
		rogue.howManyTurns		= recallNumber(4);			// how many turns are in this recording
		maxLevelChanges			= recallNumber(4);			// how many times the player changes depths
		lengthOfPlaybackFile	= recallNumber(4);
		seedRandomGenerator(rogue.seed);
		
		if (fileExists(annotationPathname)) {
			loadNextAnnotation();
		} else {
			rogue.nextAnnotationTurn = -1;
		}
	} else {
		lengthOfPlaybackFile = 1;
		remove(currentFilePath);
		recordFile = fopen(currentFilePath, "wb"); // create the file
		fclose(recordFile);
		
		flushBufferToFile(); // header info never makes it into inputRecordBuffer when recording
	}
	rogue.currentTurnNumber = 0;
}

void OOSCheck(unsigned long x, short numberOfBytes) {
	unsigned char eventType;
	unsigned long recordedNumber;
	
	if (rogue.playbackMode) {
		eventType = recallChar();
		recordedNumber = recallNumber(numberOfBytes);
		if (eventType != RNG_CHECK || recordedNumber != x) {
			if (eventType != RNG_CHECK) {
				message("Event type mismatch in RNG check.", true, false);
			}
			playbackPanic();
		}
	} else {
		recordChar(RNG_CHECK);
		recordNumber(x, numberOfBytes);
	}
}

// compare a random number once per player turn so we instantly know if we are out of sync during playback
void RNGCheck() {
	short oldRNG;
	unsigned long randomNumber;
	
	oldRNG = rogue.RNG;
	rogue.RNG = RNG_SUBSTANTIVE;
	
//#ifdef AUDIT_RNG
//reportRNGState();
	//#endif
	
	randomNumber = (unsigned long) rand_range(0, 255);
	OOSCheck(randomNumber, 1);
	
	rogue.RNG = oldRNG;
}

boolean unpause() {
	if (rogue.playbackOOS) {
		flashTemporaryAlert(" Out of sync ", 2000);
	} else if (rogue.playbackPaused) {
		rogue.playbackPaused = false;
		return true;
	}
	return false;
}

void printPlaybackHelpScreen() {
	short i;
	const char helpText[16][80] = {
		"Commands:",
		"",
		"         <space>: pause or unpause playback",
		"   k or up arrow: play back faster",
		" j or down arrow: play back slower",
		"               >: skip to next level",
		"             0-9: skip to specified turn number",
		"l or right arrow: advance one turn",
		"",
		"           <tab>: enable or disable omniscience",
		"               x: examine surroundings",
		"               i: display inventory",
		"               D: display discovered items",
		"               V: view saved recording",
		" ",
		"        -- press space to continue --"
	};
	
	blackOutScreen();
	
	for (i=0; i<16; i++) {
		printString(helpText[i], 1 + STAT_BAR_WIDTH + 4, i + MESSAGE_LINES, &white, &black, 0);
	}
	
	rogue.playbackMode = false;
	waitForAcknowledgment();
	rogue.playbackMode = true;
	refreshSideBar(NULL);
	displayLevel();
	updateFlavorText();
	updateMessageDisplay();
}

void advanceToLocation(short keystroke) {
	char entryText[30], buf[max(30, DCOLS)];
	unsigned long destinationFrame, progressBarInterval, initialFrameNumber;
	rogueEvent theEvent;
	boolean enteredText;
	
	if (!rogue.playbackPaused || unpause()) {
		buf[0] = keystroke;
		buf[1] = '\0';
		
		rogue.playbackMode = false;
		enteredText = getInputTextString(entryText, "Go to turn number: ", log10(ULONG_MAX) - 1, buf, "", TEXT_INPUT_NUMBERS);
		confirmMessages();
		rogue.playbackMode = true;
		
		if (enteredText) {
			sscanf(entryText, "%lu", &destinationFrame);
			if (destinationFrame < rogue.turnNumber) {
				flashTemporaryAlert(" Rewinding not supported ", 3000);
			} else if (destinationFrame >= rogue.howManyTurns) {
				flashTemporaryAlert(" Past end of recording ", 3000);
			} else if (destinationFrame == rogue.turnNumber) {
				sprintf(buf, " Already at turn %li ", destinationFrame);
				flashTemporaryAlert(buf, 1000);
			} else {
				rogue.playbackFastForward = true;
				progressBarInterval = max(1, (destinationFrame - rogue.turnNumber) / 500);
				initialFrameNumber = rogue.turnNumber;
				blackOutScreen();
				while (rogue.turnNumber < destinationFrame && !rogue.gameHasEnded && !rogue.playbackOOS) {
					if (!(rogue.turnNumber % progressBarInterval)) {
						rogue.playbackFastForward = false;
						printProgressBar((COLS - 20) / 2, ROWS / 2, "[     Loading...   ]",
										 rogue.turnNumber - initialFrameNumber,
										 destinationFrame - initialFrameNumber, &darkPurple, false);
						pauseBrogue(1);
						rogue.playbackFastForward = true;
					}
					
					rogue.RNG = RNG_COSMETIC; // dancing terrain colors can't influence recordings
					rogue.playbackDelayThisTurn = 0;
					nextBrogueEvent(&theEvent, true, false);
					rogue.RNG = RNG_SUBSTANTIVE;
					executeEvent(&theEvent);
				}
				rogue.playbackPaused = true;
				rogue.playbackFastForward = false;
				confirmMessages();
				updateMessageDisplay();
				refreshSideBar(NULL);
				displayLevel();
			}
			rogue.playbackPaused = true;
		}
	}
}

// used to interact with playback -- e.g. changing speed, pausing
void executePlaybackInput(rogueEvent *recordingInput) {
	uchar key;
	short newDelay, frameCount;
	unsigned long initialState, destinationFrame;
	boolean pauseState;
	rogueEvent theEvent;
	
	if (!rogue.playbackMode) {
		return;
	}
	
	if (recordingInput->eventType == KEYSTROKE) {
		key = recordingInput->param1;
		
		switch (key) {
			case UP_ARROW:
			case UP_KEY:
				newDelay = max(1, min(rogue.playbackDelayPerTurn / 1.5, rogue.playbackDelayPerTurn - 1));
				if (newDelay != rogue.playbackDelayPerTurn) {
					flashTemporaryAlert(" Faster ", 300);
				}
				rogue.playbackDelayPerTurn = newDelay;
				break;
			case DOWN_ARROW:
			case DOWN_KEY:
				newDelay = min(3000, max(rogue.playbackDelayPerTurn * 1.5, rogue.playbackDelayPerTurn + 1));
				if (newDelay != rogue.playbackDelayPerTurn) {
					flashTemporaryAlert(" Slower ", 300);
				}
				rogue.playbackDelayPerTurn = newDelay;
				break;
			case ACKNOWLEDGE_KEY:
				if (rogue.playbackOOS && rogue.playbackPaused) {
					flashTemporaryAlert(" Out of sync ", 2000);
				} else {
					rogue.playbackPaused = !rogue.playbackPaused;
					if (rogue.playbackPaused) {
						flashTemporaryAlert(" Recording paused ", 1000);
					} else {
						flashTemporaryAlert(" Recording unpaused ", 200);
					}
				}
				break;
			case TAB_KEY:
				rogue.playbackOmniscience = !rogue.playbackOmniscience;
				displayLevel();
				refreshSideBar(NULL);
				if (rogue.playbackOmniscience) {
					flashTemporaryAlert(" Omniscience enabled ", 1000);
				} else {
					flashTemporaryAlert(" Omniscience disabled ", 1000);
				}
				break;
			case ASCEND_KEY:
			case DESCEND_KEY:
				pauseState = rogue.playbackPaused;
				if (!rogue.playbackPaused || unpause()) {
					if (rogue.howManyDepthChanges < maxLevelChanges) {
						displayCenteredAlert(" Loading... ");
						pauseBrogue(5);
						initialState = rogue.depthLevel;
						rogue.playbackFastForward = true;
						while ((initialState == rogue.depthLevel || !rogue.playbackBetweenTurns)
							   && !rogue.gameHasEnded) {
							rogue.RNG = RNG_COSMETIC; // dancing terrain colors can't influence recordings
							nextBrogueEvent(&theEvent, true, false);
							rogue.RNG = RNG_SUBSTANTIVE;
							executeEvent(&theEvent);
						}
						rogue.playbackFastForward = false;
						refreshSideBar(NULL);
						displayLevel();
					} else {
						flashTemporaryAlert(" No further depth changes ", 500);
					}
				}
				rogue.playbackPaused = pauseState;
				break;
			case EXAMINE_KEY:
				examineMode();
				break;
			case INVENTORY_KEY:
				rogue.playbackMode = false;
				displayInventory(ALL_ITEMS, 0, 0, true);
				rogue.playbackMode = true;
				break;
			case RIGHT_KEY:
			case RIGHT_ARROW:
				frameCount = 1;
				if (recordingInput->shiftKey) {
					frameCount *= 5;
					rogue.playbackFastForward = true;
				}
				if (recordingInput->controlKey) {
					frameCount *= 20;
					rogue.playbackFastForward = true;
				}
				destinationFrame = rogue.turnNumber + frameCount;
				if (destinationFrame >= rogue.howManyTurns) {
					destinationFrame = rogue.howManyTurns - 1;
				}
				
				// advance by the right number of turns
				if (!rogue.playbackPaused || unpause()) {
					initialState = rogue.turnNumber;
					while (rogue.turnNumber < destinationFrame && !rogue.gameHasEnded && !rogue.playbackOOS) {
						rogue.RNG = RNG_COSMETIC; // dancing terrain colors can't influence recordings
						rogue.playbackDelayThisTurn = 0;
						nextBrogueEvent(&theEvent, true, false);
						rogue.RNG = RNG_SUBSTANTIVE;
						executeEvent(&theEvent);
					}
					rogue.playbackPaused = true;
					if (rogue.playbackFastForward) {
						rogue.playbackFastForward = false;
						displayLevel();
						updateMessageDisplay();
						refreshSideBar(NULL);
					}
				}
				break;
			case HELP_KEY:
				printPlaybackHelpScreen();
				break;
			case DISCOVERIES_KEY:
				rogue.playbackMode = false;
				printDiscoveriesScreen();
				rogue.playbackMode = true;
				break;
			case VIEW_RECORDING_KEY:
				confirmMessages();
				rogue.playbackMode = false;
				if (openFile("View recording: ", "Recording", RECORDING_SUFFIX)) {
					freeEverything();
					randomNumbersGenerated = 0;
					rogue.playbackMode = true;
					initializeRogue();
					startLevel(rogue.depthLevel, 1);
				} else {
					rogue.playbackMode = true;
				}
				break;
//			case NEW_GAME_KEY:
//				rogue.playbackMode = false;
//				freeEverything();
//				randomNumbersGenerated = 0;
//				strcpy(currentFilePath, LAST_GAME_PATH);
//				initializeRogue();
//				startLevel(rogue.depthLevel, 1);
//				break;
			case SEED_KEY:
				printSeed();
				displayLoops();
				break;
			default:
				if (key >= '0' && key <= '9'
					|| key >= NUMPAD_0 && key <= NUMPAD_9) {
					advanceToLocation(key);
				}
				break;
		}
	}
}

void saveGame() {
	char filePath[FILENAME_MAX];
	boolean askAgain;
	
	if (rogue.playbackMode) {
		return; // call me paranoid, but I'd rather it be impossible to embed malware in a recording
	}
	
	deleteMessages();
	do {
		askAgain = false;
		if (getInputTextString(filePath, "Save game as (<esc> to cancel): ",
							   FILENAME_MAX - strlen(GAME_SUFFIX), "Saved game", GAME_SUFFIX, TEXT_INPUT_NORMAL)) {
			
			strcat(filePath, GAME_SUFFIX);
			if (!fileExists(filePath) || confirm("File of that name already exists. Overwrite? (y/n)", true)) {
				remove(filePath);
				flushBufferToFile();
				rename(currentFilePath, filePath);
				strcpy(currentFilePath, filePath);
				message("Saved.", true, true);
				printHighScores(false);
				commitDraws();
				rogue.gameHasEnded = true;
			} else {
				askAgain = true;
			}
		}
	} while (askAgain);
	deleteMessages();
}

void saveRecording() {
	char filePath[FILENAME_MAX];
	boolean askAgain;
	
	if (rogue.playbackMode) {
		return;
	}
	
	deleteMessages();
	do {
		askAgain = false;
		if (getInputTextString(filePath, "Save recording as (<esc> to cancel): ",
							   FILENAME_MAX - strlen(RECORDING_SUFFIX), "Recording", RECORDING_SUFFIX, TEXT_INPUT_NORMAL)) {
			
			strcat(filePath, RECORDING_SUFFIX);
			if (!fileExists(filePath) || confirm("File of that name already exists. Overwrite? (y/n)", true)) {
				remove(filePath);
				rename(currentFilePath, filePath);
			} else {
				askAgain = true;
			}
		} else { // declined to save
			remove(currentFilePath);
		}
	} while (askAgain);
	deleteMessages();
}

void copyFile(char *fromFilePath, char *toFilePath, unsigned long fromFileLength) {
	unsigned long m, n;
	unsigned char fileBuffer[INPUT_RECORD_BUFFER];
	FILE *fromFile, *toFile;
	
	remove(toFilePath);
	
	fromFile	= fopen(fromFilePath, "rb");
	toFile		= fopen(toFilePath, "wb");
	
	for (n = 0; n < fromFileLength; n += m) {
		m = min(INPUT_RECORD_BUFFER, fromFileLength - n);
		fread((void *) fileBuffer, 1, m, fromFile);
		fwrite((void *) fileBuffer, 1, m, toFile);
	}
	
	fclose(fromFile);
	fclose(toFile);
}

// at the end of loading a saved game, this function transitions into active play mode.
void switchToPlaying() {
	rogue.playbackMode			= false;
	rogue.playbackFastForward	= false;
	rogue.playbackOmniscience	= false;
	locationInRecordingBuffer	= 0;
	copyFile(currentFilePath, LAST_GAME_PATH, recordingLocation);
	
#ifdef DELETE_SAVE_FILE_AFTER_LOADING
	remove(currentFilePath);
#endif
	
	strcpy(currentFilePath, LAST_GAME_PATH);
	
	refreshSideBar(NULL);
	updateMessageDisplay();
	displayLevel();
}

void loadSavedGame() {
	short progressBarInterval;
	rogueEvent theEvent;
	
	blackOutScreen();
	pauseBrogue(1);
	freeEverything();
	randomNumbersGenerated = 0;
	rogue.playbackMode = true;
	rogue.playbackFastForward = true;
	initializeRogue(); // calls initRecording().
	startLevel(rogue.depthLevel, 1);
	
	if (rogue.howManyTurns > 0) {
		
		progressBarInterval = max(1, lengthOfPlaybackFile / 100);
		
		while (recordingLocation < lengthOfPlaybackFile && !rogue.gameHasEnded && !rogue.playbackOOS) {
			rogue.RNG = RNG_COSMETIC;
			nextBrogueEvent(&theEvent, true, false);
			rogue.RNG = RNG_SUBSTANTIVE;
			
			executeEvent(&theEvent);
			
			if (!(recordingLocation % progressBarInterval) && !rogue.playbackOOS) {
				rogue.playbackFastForward = false; // so the progress bar redraws make it to the screen
				printProgressBar((COLS - 20) / 2, ROWS / 2, "[     Loading...   ]", recordingLocation, lengthOfPlaybackFile, &darkPurple, false);
				pauseBrogue(1);
				rogue.playbackFastForward = true;
			}
		}
	}
	
	if (!rogue.gameHasEnded && !rogue.playbackOOS) {
		switchToPlaying();
		recordChar(SAVED_GAME_LOADED);
	}
}

#pragma mark Debug functions

// the following functions are used to create human-readable descriptions of playback files for debugging purposes

void describeKeystroke(unsigned char key, char *description) {
	short i;
	uchar c;
	const uchar ucharList[53] =	{UP_KEY, DOWN_KEY, LEFT_KEY, RIGHT_KEY, UP_ARROW, LEFT_ARROW,
		DOWN_ARROW, RIGHT_ARROW, UPLEFT_KEY, UPRIGHT_KEY, DOWNLEFT_KEY, DOWNRIGHT_KEY,
		DESCEND_KEY, ASCEND_KEY, REST_KEY, AUTO_REST_KEY, SEARCH_KEY, INVENTORY_KEY,
		ACKNOWLEDGE_KEY, EQUIP_KEY, UNEQUIP_KEY, APPLY_KEY, THROW_KEY, DROP_KEY, CALL_KEY,
		FIGHT_KEY, FIGHT_TO_DEATH_KEY, HELP_KEY, DISCOVERIES_KEY, REPEAT_TRAVEL_KEY,
		EXAMINE_KEY, EXPLORE_KEY, AUTOPLAY_KEY, SEED_KEY, EASY_MODE_KEY, ESCAPE_KEY,
		RETURN_KEY, ENTER_KEY, DELETE_KEY, TAB_KEY, PERIOD_KEY, VIEW_RECORDING_KEY, NUMPAD_0,
		NUMPAD_1, NUMPAD_2, NUMPAD_3, NUMPAD_4, NUMPAD_5, NUMPAD_6, NUMPAD_7, NUMPAD_8,
		NUMPAD_9, UNKNOWN_KEY};
	const char descList[54][30] = {"up", "down", "left", "right", "up arrow", "left arrow",
		"down arrow", "right arrow", "upleft", "upright", "downleft", "downright",
		"descend", "ascend", "rest", "auto rest", "search", "inventory", "acknowledge",
		"equip", "unequip", "apply", "throw", "drop", "call", "fight", "fight to death",
		"help", "discoveries", "repeat travel", "examine", "explore", "autoplay", "seed",
		"easy mode", "escape", "return", "enter", "delete", "tab", "period", "open file",
		"numpad 0", "numpad 1", "numpad 2", "numpad 3", "numpad 4", "numpad 5", "numpad 6",
		"numpad 7", "numpad 8", "numpad 9", "unknown", "ERROR!!!"};	
	c = uncompressKeystroke(key);
	for (i=0; ucharList[i] != c && i < 53; i++);
	if (key >= 32 && key <= 126) {
		sprintf(description, "Key: %c\t(%s)", key, descList[i]);
	} else {
		sprintf(description, "Key: %i\t(%s)", key, descList[i]);
	}
}

void appendModifierKeyDescription(char *description) {
	unsigned char c = recallChar();
	
	if (c & Fl(1)) {
		strcat(description, " + CRTL");
	}
	if (c & Fl(2)) {
		strcat(description, " + SHIFT");
	}
}

void parseFile() {
	FILE *descriptionFile;
	unsigned long oldFileLoc, oldRecLoc, oldLength, oldBufLoc, i, seed, numTurns, numDepths, fileLength, startLoc;
	unsigned char c;
	char description[1000], versionString[500];
	short x, y;
	
	if (openFile("Parse recording: ", "Recording.broguerec", "")) {
		
		oldFileLoc = positionInPlaybackFile;
		oldRecLoc = recordingLocation;
		oldBufLoc = locationInRecordingBuffer;
		oldLength = lengthOfPlaybackFile;
		
		positionInPlaybackFile = 0;
		locationInRecordingBuffer = 0;
		recordingLocation = 0;
		lengthOfPlaybackFile = 10000000; // hack so that the recalls don't freak out
		fillBufferFromFile();
		
		descriptionFile = fopen("Recording Description.txt", "w");
		
		for (i=0; i<16; i++) {
			versionString[i] = recallChar();
		}
		
		seed		= recallNumber(4);
		numTurns	= recallNumber(4);
		numDepths	= recallNumber(4);
		fileLength	= recallNumber(4);
		
		fprintf(descriptionFile, "Parsed file \"%s\":\n\tVersion:\n\tSeed: %li\n\tNumber of turns: %li\n\tNumber of depth changes: %li\n\tFile length: %li\n",
				currentFilePath,
				versionString,
				seed,
				numTurns,
				numDepths,
				fileLength);
		for (i=0; recordingLocation < fileLength; i++) {
			startLoc = recordingLocation;
			c = recallChar();
			switch (c) {
				case KEYSTROKE:
					describeKeystroke(recallChar(), description);
					appendModifierKeyDescription(description);
					break;
				case MOUSE_UP:
				case MOUSE_DOWN:
				case MOUSE_ENTERED_CELL:
					x = (short) recallChar();
					y = (short) recallChar();
					sprintf(description, "Mouse click: (%i, %i)", x, y);
					appendModifierKeyDescription(description);
					break;
				case RNG_CHECK:
					sprintf(description, "\tRNG check: %i", (short) recallChar());
					break;
				case SAVED_GAME_LOADED:
					strcpy(description, "Saved game loaded");
					break;
				default:
					sprintf(description, "UNKNOWN EVENT TYPE: %i", (short) c);
					break;
			}
			fprintf(descriptionFile, "\nEvent %li, loc %li, length %li:%s\t%s", i, startLoc, recordingLocation - startLoc, (i < 10 ? " " : ""), description);
		}
		
		fclose(descriptionFile);
		
		positionInPlaybackFile = oldFileLoc;
		recordingLocation = oldRecLoc;
		lengthOfPlaybackFile = oldLength;
		locationInRecordingBuffer = oldBufLoc;
		message("File parsed.", true, false);
	} else {
		confirmMessages();
	}
}

void RNGLog(char *message) {
#ifdef AUDIT_RNG
	fputs(message, RNGLogFile);
#endif
}
