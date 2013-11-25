//
//  RogueDriver.m
//  Brogue
//
//  Created by Brian and Kevin Walker on 12/26/08.
//  Copyright 2011. All rights reserved.
//  
//  This file is part of Brogue.
//
//  Brogue is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  Brogue is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with Brogue.  If not, see <http://www.gnu.org/licenses/>.
//

#include <limits.h>
#include <unistd.h>
#include "CoreFoundation/CoreFoundation.h"
#import "RogueDriver.h"

#define BROGUE_VERSION	2

static Viewport *theMainDisplay;
NSDate *pauseStartDate;
short mouseX, mouseY;

@implementation RogueDriver

- (void)awakeFromNib
{
	extern Viewport *theMainDisplay;
	NSSize theSize;
	//NSRect theRect;
	short versionNumber;
	
	versionNumber = [[NSUserDefaults standardUserDefaults] integerForKey:@"Brogue version"];
	if (versionNumber == nil || versionNumber < BROGUE_VERSION) {
		
		[[NSUserDefaults standardUserDefaults] removeObjectForKey:@"NSWindow Frame Brogue main window"];
		
		if (versionNumber != nil) {
			[[NSUserDefaults standardUserDefaults] removeObjectForKey:@"Brogue version"];
		}
		[[NSUserDefaults standardUserDefaults] setInteger:BROGUE_VERSION forKey:@"Brogue version"];
		[[NSUserDefaults standardUserDefaults] synchronize];
	}
		
	
	theMainDisplay = theDisplay;
	[theWindow setFrameAutosaveName:@"Brogue main window"];
	[theWindow useOptimizedDrawing:YES];
	[theWindow setAcceptsMouseMovedEvents:YES];
	
	theSize.height = kROWS;
	theSize.width = kCOLS;
	[theWindow setContentResizeIncrements:theSize];
	
	theSize.height = 7 * VERT_PX * kROWS / FONT_SIZE;
	theSize.width = 7 * HORIZ_PX * kCOLS / FONT_SIZE;
	[theWindow setContentMinSize:theSize];
	
	mouseX = mouseY = 0;
	
	/*theRect = [theWindow contentRectForFrameRect:[theWindow frame]];
	
	if (theRect) {
		[theMainDisplay setHorizPixels:(theWidth / kCOLS) vertPixels:(theHeight / kROWS) fontSize:max(theSize - 2, 9)];
	}*/
}

- (IBAction)playBrogue:(id)sender
{
	[fileMenu setAutoenablesItems:NO];
	[menuNewGame setEnabled:NO];
	rogueMain();
	[menuNewGame setEnabled:YES];
	[fileMenu setAutoenablesItems:YES];	
}

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
	[theWindow makeMainWindow];
	[theWindow makeKeyWindow];
	[self windowDidResize:nil];
	//NSLog(@"\nAspect ratio is %@", [theWindow aspectRatio]);
	[self playBrogue:nil];
}

- (void)windowDidResize:(NSNotification *)aNotification
{
	NSRect theRect;
	NSSize testSizeBox;
	NSMutableDictionary *theAttributes = [[NSMutableDictionary alloc] init];
	short theWidth, theHeight, theSize;
	
	theRect = [theWindow contentRectForFrameRect:[theWindow frame]];
	theWidth = theRect.size.width;
	theHeight = theRect.size.height;
	theSize = min(FONT_SIZE * theWidth / (HORIZ_PX * kCOLS), FONT_SIZE * theHeight / (VERT_PX * kROWS));
	do {
		[theAttributes setObject:[NSFont fontWithName: @"Monaco" size:theSize++] forKey:NSFontAttributeName];
		testSizeBox = [@"a" sizeWithAttributes:theAttributes];
	} while (testSizeBox.width < theWidth / kCOLS && testSizeBox.height < theHeight / kROWS);
	[theMainDisplay setHorizPixels:(theWidth / kCOLS) vertPixels:(theHeight / kROWS) fontSize:max(theSize - 2, 9)];
}

- (NSRect)windowWillUseStandardFrame:(NSWindow *)window
					  defaultFrame:(NSRect)defaultFrame
{
	NSRect theRect;
	if (defaultFrame.size.width > HORIZ_PX * kCOLS) {
		theRect.size.width = HORIZ_PX * kCOLS;
		theRect.size.height = VERT_PX * kROWS;
	} else {
		theRect.size.width = (HORIZ_PX - 1) * kCOLS;
		theRect.size.height = (VERT_PX - 2) * kROWS;
	}
	
	theRect.origin = [window contentRectForFrameRect:[window frame]].origin;
	theRect.origin.y += ([window contentRectForFrameRect:[window frame]].size.height - theRect.size.height);
	
	if (theRect.origin.x + theRect.size.width > defaultFrame.size.width) {
		theRect.origin.x = defaultFrame.size.width - theRect.size.width;
	}
	if (theRect.origin.y + theRect.size.height > defaultFrame.size.height) {
		theRect.origin.y = defaultFrame.size.height - theRect.size.height;
	} 
	theRect = [theWindow frameRectForContentRect:theRect];
	return theRect;
}

@end

/* plotChar: plots inputChar at (xLoc, yLoc) with specified background and foreground colors.
 * Color components are given in ints from 0 to 100.
 */
void plotChar(uchar inputChar,
			  short xLoc, short yLoc,
			  short foreRed, short foreGreen, short foreBlue,
			  short backRed, short backGreen, short backBlue)
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	[theMainDisplay setString:[NSString stringWithCharacters:&inputChar length:1]
			   withBackground:[NSColor colorWithDeviceRed:((float)backRed/100)
													green:((float)backGreen/100)
													 blue:((float)backBlue/100)
													alpha:(float)1]
			  withLetterColor:[NSColor colorWithDeviceRed:((float)foreRed/100)
													green:((float)foreGreen/100)
													 blue:((float)foreBlue/100)
													alpha:(float)1]
				  atLocationX:xLoc locationY:yLoc];
	[pool drain];
}

void pausingTimerStartsNow() {
	if (pauseStartDate) {
		[pauseStartDate release];
	}
	pauseStartDate = [NSDate date];
	[pauseStartDate retain];
}

// Returns true if the player interrupted the wait with a keystroke; otherwise false.
boolean pauseForMilliseconds(short milliseconds) {
	NSEvent *theEvent;
	NSDate *theDate;
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	
	if (pauseStartDate) {
		theDate = [pauseStartDate addTimeInterval: ((double) milliseconds / 1000)];
		[pauseStartDate release];
		pauseStartDate = NULL;
	} else {
		theDate = [NSDate dateWithTimeIntervalSinceNow: (double) milliseconds / 1000];
	}
	
	do {
		theEvent = [NSApp nextEventMatchingMask:NSAnyEventMask untilDate:theDate
										 inMode:NSDefaultRunLoopMode dequeue:YES];
		if (([theEvent type] == NSKeyDown && !([theEvent modifierFlags] & NSCommandKeyMask))
			|| ([theEvent type] == NSLeftMouseUp || [theEvent type] == NSLeftMouseDown)) {
			[NSApp postEvent:theEvent atStart:TRUE]; // put the event back on the queue
			return true;
		} else if (theEvent != nil) {
			[NSApp sendEvent:theEvent];
		}
	} while (theEvent != nil);
	
	[pool drain];
	return false;
}

void nextKeyOrMouseEvent(rogueEvent *returnEvent, boolean colorsDance) {
	NSEvent *theEvent;
	NSEventType theEventType;
	NSPoint event_location;
	NSPoint local_point;
	short x, y;
	
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	
	for(;;) {
		if (colorsDance) {
			shuffleTerrainColors(3, true);
			commitDraws();
		}
		
		theEvent = [NSApp nextEventMatchingMask:NSAnyEventMask
									  untilDate:[NSDate dateWithTimeIntervalSinceNow: (double) 50 / 1000]
										 inMode:NSDefaultRunLoopMode dequeue:YES];
		theEventType = [theEvent type];
		if (theEventType == NSKeyDown && !([theEvent modifierFlags] & NSCommandKeyMask)) {
			returnEvent->eventType = KEYSTROKE;
			returnEvent->param1 = [[theEvent charactersIgnoringModifiers] characterAtIndex:0];
			returnEvent->param2 = 0;
			returnEvent->controlKey = ([theEvent modifierFlags] & NSControlKeyMask ? 1 : 0);
			returnEvent->shiftKey = ([theEvent modifierFlags] & NSShiftKeyMask ? 1 : 0);
			break;
		} else if (theEventType == NSLeftMouseDown || theEventType == NSLeftMouseUp || theEventType == NSMouseMoved) {
			[NSApp sendEvent:theEvent];
			switch (theEventType) {
				case NSLeftMouseDown:
					returnEvent->eventType = MOUSE_DOWN;
					break;
				case NSLeftMouseUp:
					returnEvent->eventType = MOUSE_UP;
					break;
				case NSMouseMoved:
					returnEvent->eventType = MOUSE_ENTERED_CELL;
					break;
				default:
					break;
			}
			event_location = [theEvent locationInWindow];
			local_point = [theMainDisplay convertPoint:event_location fromView:nil];
			x = local_point.x / [theMainDisplay horizPixels];
			y = ROWS - local_point.y / [theMainDisplay vertPixels];
			// Correct for the fact that truncation occurs in a positive direction when we're below zero:
			if (local_point.x < 0) {
				x--;
			}
			if (ROWS * [theMainDisplay vertPixels] < local_point.y) {
				y--;
			}
			returnEvent->param1 = x;
			returnEvent->param2 = y;
			returnEvent->controlKey = ([theEvent modifierFlags] & NSControlKeyMask ? 1 : 0);
			returnEvent->shiftKey = ([theEvent modifierFlags] & NSShiftKeyMask ? 1 : 0);
			if (theEventType != NSMouseMoved || x != mouseX || y != mouseY) {
				mouseX = x;
				mouseY = y;
				break;
			}
		}
		if (theEvent != nil) {
			[NSApp sendEvent:theEvent]; // pass along the non-keyboard event so, e.g., the menus work
		}
	}
	// printf("\nRogueEvent: eventType: %i, param1: %i, param2: %i, controlKey: %s, shiftKey: %s", returnEvent->eventType, returnEvent->param1,
	//			 returnEvent->param2, returnEvent->controlKey ? "true" : "false", returnEvent->shiftKey ? "true" : "false");
	
	[pool drain];
}

void initHighScores() {
	NSMutableArray *scoresArray, *textArray, *datesArray;
	short j, theCount;
	
	if ([[NSUserDefaults standardUserDefaults] arrayForKey:@"high scores scores"] == nil
		|| [[NSUserDefaults standardUserDefaults] arrayForKey:@"high scores text"] == nil
		|| [[NSUserDefaults standardUserDefaults] arrayForKey:@"high scores dates"] == nil) {
		
		scoresArray = [NSMutableArray arrayWithCapacity:25];
		textArray = [NSMutableArray arrayWithCapacity:25];
		datesArray = [NSMutableArray arrayWithCapacity:25];
		
		for (j=0; j<25; j++) {
			[scoresArray addObject:[NSNumber numberWithLong:0]];
			[textArray addObject:[NSString string]];
			[datesArray addObject:[NSDate date]];
		}
		
		[[NSUserDefaults standardUserDefaults] setObject:scoresArray forKey:@"high scores scores"];
		[[NSUserDefaults standardUserDefaults] setObject:textArray forKey:@"high scores text"];
		[[NSUserDefaults standardUserDefaults] setObject:datesArray forKey:@"high scores dates"];
	}
	
	theCount = [[[NSUserDefaults standardUserDefaults] arrayForKey:@"high scores scores"] count];
	
	if (theCount < 25) { // backwards compatibility
		scoresArray = [NSMutableArray arrayWithCapacity:25];
		textArray = [NSMutableArray arrayWithCapacity:25];
		datesArray = [NSMutableArray arrayWithCapacity:25];
		
		[scoresArray setArray:[[NSUserDefaults standardUserDefaults] arrayForKey:@"high scores scores"]];
		[textArray setArray:[[NSUserDefaults standardUserDefaults] arrayForKey:@"high scores text"]];
		[datesArray setArray:[[NSUserDefaults standardUserDefaults] arrayForKey:@"high scores dates"]];
		
		for (j=theCount; j<25; j++) {
			[scoresArray addObject:[NSNumber numberWithLong:0]];
			[textArray addObject:[NSString string]];
			[datesArray addObject:[NSDate date]];
		}
		
		[[NSUserDefaults standardUserDefaults] setObject:scoresArray forKey:@"high scores scores"];
		[[NSUserDefaults standardUserDefaults] setObject:textArray forKey:@"high scores text"];
		[[NSUserDefaults standardUserDefaults] setObject:datesArray forKey:@"high scores dates"];
	}
}

// returns the index number of the most recent score
short getHighScoresList(rogueHighScoresEntry returnList[25]) {
	NSArray *scoresArray, *textArray, *datesArray;
	NSDateFormatter *dateFormatter;
	NSDate *mostRecentDate;
	short i, j, maxIndex, mostRecentIndex;
	long maxScore;
	boolean scoreTaken[25];
	
	// no scores have been taken
	for (i=0; i<25; i++) {
		scoreTaken[i] = false;
	}
	
	initHighScores();
	
	scoresArray = [[NSUserDefaults standardUserDefaults] arrayForKey:@"high scores scores"];
	textArray = [[NSUserDefaults standardUserDefaults] arrayForKey:@"high scores text"];
	datesArray = [[NSUserDefaults standardUserDefaults] arrayForKey:@"high scores dates"];
	
	mostRecentDate = [NSDate distantPast];
	dateFormatter = [[NSDateFormatter alloc] initWithDateFormat:@"%1m/%1d/%y" allowNaturalLanguage:YES];
	
	// store each value in order into returnList
	for (i=0; i<25; i++) {
		// find the highest value that hasn't already been taken
		maxScore = 0;
		for (j=0; j<25; j++) {
			if (scoreTaken[j] == false && [[scoresArray objectAtIndex:j] longValue] >= maxScore) {
				maxScore = [[scoresArray objectAtIndex:j] longValue];
				maxIndex = j;
			}
		}
		// maxIndex identifies the highest non-taken score
		scoreTaken[maxIndex] = true;
		returnList[i].score = [[scoresArray objectAtIndex:maxIndex] longValue];
		strcpy(returnList[i].description, [[textArray objectAtIndex:maxIndex] cStringUsingEncoding:NSASCIIStringEncoding]);
		strcpy(returnList[i].date, [[dateFormatter stringFromDate:[datesArray objectAtIndex:maxIndex]] cStringUsingEncoding:NSASCIIStringEncoding]);
		
		// if this is the most recent score we've seen so far
		if ([mostRecentDate compare:[datesArray objectAtIndex:maxIndex]] == NSOrderedAscending) {
			mostRecentDate = [datesArray objectAtIndex:maxIndex];
			mostRecentIndex = i;
		}
	}
	return mostRecentIndex;
}

// saves the high scores entry over the lowest-score entry if it qualifies.
// returns whether the score qualified for the list.
// This function ignores the date passed to it in theEntry and substitutes the current
// date instead.
boolean saveHighScore(rogueHighScoresEntry theEntry) {
	NSMutableArray *scoresArray, *textArray, *datesArray;
	NSNumber *newScore;
	NSString *newText;
	
	short j, minIndex = -1;
	long minScore = theEntry.score;
	
	// generate high scores if prefs don't exist or contain no high scores data
	initHighScores();
	
	scoresArray = [NSMutableArray arrayWithCapacity:25];
	textArray = [NSMutableArray arrayWithCapacity:25];
	datesArray = [NSMutableArray arrayWithCapacity:25];
	
	[scoresArray setArray:[[NSUserDefaults standardUserDefaults] arrayForKey:@"high scores scores"]];
	[textArray setArray:[[NSUserDefaults standardUserDefaults] arrayForKey:@"high scores text"]];
	[datesArray setArray:[[NSUserDefaults standardUserDefaults] arrayForKey:@"high scores dates"]];
	
	// find the lowest value
	for (j=0; j<25; j++) {
		if ([[scoresArray objectAtIndex:j] longValue] < minScore) {
			minScore = [[scoresArray objectAtIndex:j] longValue];
			minIndex = j;
		}
	}
	
	if (minIndex == -1) { // didn't qualify
		return false;
	}
	
	// minIndex identifies the score entry to be replaced
	newScore = [NSNumber numberWithLong:theEntry.score];
	newText = [NSString stringWithCString:theEntry.description encoding:NSASCIIStringEncoding];
	[scoresArray replaceObjectAtIndex:minIndex withObject:newScore];
	[textArray replaceObjectAtIndex:minIndex withObject:newText];
	[datesArray replaceObjectAtIndex:minIndex withObject:[NSDate date]];
	
	[[NSUserDefaults standardUserDefaults] setObject:scoresArray forKey:@"high scores scores"];
	[[NSUserDefaults standardUserDefaults] setObject:textArray forKey:@"high scores text"];
	[[NSUserDefaults standardUserDefaults] setObject:datesArray forKey:@"high scores dates"];
	[[NSUserDefaults standardUserDefaults] synchronize];
	
	return true;
}

void initializeBrogueSaveLocation() {
//	char path[PATH_MAX];
//	CFBundleRef mainBundle = CFBundleGetMainBundle();
//    CFURLRef bundleURL = CFBundleCopyBundleURL(mainBundle);
//	CFURLRef saveURL = CFURLCreateCopyDeletingLastPathComponent(NULL, bundleURL);
//	
//    if (!CFURLGetFileSystemRepresentation(saveURL, TRUE, (UInt8 *)path, PATH_MAX)) {
//        // error!
//    }
//	
//    CFRelease(saveURL);
//    CFRelease(bundleURL);
//	
//    chdir(path);
	
    NSFileManager *manager = [NSFileManager defaultManager];
    
    // Look up the full path to the user's Application Support folder (usually ~/Library/Application Support/).
    NSString *basePath = [NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES) objectAtIndex: 0];
    
    // Use a folder under Application Support named after the application.
    NSString *appName = [[NSBundle mainBundle] objectForInfoDictionaryKey: @"CFBundleName"];
    NSString *supportPath = [basePath stringByAppendingPathComponent: appName];
    
    // Create our folder the first time it is needed.
    if (![manager fileExistsAtPath: supportPath])
    {
        [[NSFileManager defaultManager] createDirectoryAtPath: supportPath attributes: nil];
    }
    
    // Set the working directory to this path, so that savegames and recordings will be stored here.
    [manager changeCurrentDirectoryPath: supportPath];
}
