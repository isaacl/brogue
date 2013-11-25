//
//  Viewport.m
//  Brogue
//
//  Created by Brian and Kevin Walker on 11/28/08.
//  Copyright 2009. All rights reserved.
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

#import "Viewport.h"

@implementation Viewport

NSSize characterSize;

short vPixels = 18;
short hPixels = 9; // was 11

short theFontSize = 13;

- (id)initWithFrame:(NSRect)rect
{
	if ( ![super initWithFrame:rect] ) {
		return nil;
	}
	
	int i, j;
	
	for ( j = 0; j < kROWS; j++ ) {
		for ( i = 0; i < kCOLS; i++ ) {
			letterArray[i][j] = @" ";
			[letterArray[i][j] retain];
			bgColorArray[i][j] = [[NSColor whiteColor] retain];
			
			attributes[i][j] = [[NSMutableDictionary alloc] init];
			[attributes[i][j] setObject:[NSFont fontWithName: @"Monaco" size: theFontSize]
						   forKey:NSFontAttributeName];
			[attributes[i][j] setObject:[NSColor blackColor]
						   forKey:NSForegroundColorAttributeName];			
			rectArray[i][j] = NSMakeRect(HORIZ_PX*i, (VERT_PX * kROWS)-(VERT_PX*(j+1)), HORIZ_PX, VERT_PX);
		}
	}
	
	characterSize = [@"a" sizeWithAttributes:attributes[0][0]]; // no need to do this every time we draw a character
	
	//NSLog(@"in initWithFrame, rect is (%f, %f)", rect.origin.x, rect.origin.y	);
	 
	return self;
}

- (BOOL)isOpaque
{
	return YES;
}

- (void)setString:(NSString *)c 
   withBackground:(NSColor *)bgColor
  withLetterColor:(NSColor *)letterColor
	  atLocationX:(short)x
		locationY:(short)y
{
	[letterArray[x][y] release];
	[bgColorArray[x][y] release];
	letterArray[x][y] = [c retain];
	bgColorArray[x][y] = [bgColor retain];
	[attributes[x][y] setObject:letterColor forKey:NSForegroundColorAttributeName];
	//[self setNeedsDisplayInRect:rectArray[x][y]];
}

- (void)updateCellAtX:(short)x
				  atY:(short)y
{
	[self setNeedsDisplayInRect:rectArray[x][y]];
}

- (void)drawRect:(NSRect)rect
{
	int i, j;
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	for ( j = 0; j < kROWS; j++ ) {
		for ( i = 0; i < kCOLS; i++ ) {
			[bgColorArray[i][j] set];
			[NSBezierPath fillRect:rectArray[i][j]];
			//NSLog(@"bgColorArray[%i][%i] is %@; letter is %@, letter color is %@", i, j, bgColorArray[i][j], letterArray[i][j], [attributes[i][j] objectForKey:NSForegroundColorAttributeName]);
			// very expensive; don't know how to optimize:
			[self drawTheString:letterArray[i][j] centeredIn:rectArray[i][j] withAttributes:attributes[i][j]];
		}
	}
	[pool drain];
}

- (void)drawTheString:(NSString *)theString centeredIn:(NSRect)rect withAttributes:(NSMutableDictionary *)theAttributes
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	NSPoint stringOrigin;
//	NSSize stringSize;
//	stringSize = [theString sizeWithAttributes:theAttributes];	// quite expensive
//	NSLog(@"\nStringSize has width %f and height %f", stringSize.width, stringSize.height);
//	stringOrigin.x = rect.origin.x + (rect.size.width - stringSize.width)/2;
//	stringOrigin.y = rect.origin.y + (rect.size.height - stringSize.height)/2;
	stringOrigin.x = rect.origin.x + (rect.size.width - characterSize.width)/2;
	stringOrigin.y = rect.origin.y + (rect.size.height - characterSize.height)/2;
	[theString drawAtPoint:stringOrigin withAttributes:theAttributes];
	[pool drain];
}

- (short)horizPixels
{
	return hPixels;
}

- (short)vertPixels
{
	return vPixels;
}

- (short)fontSize
{
	return theFontSize;
}

- (void)setHorizPixels:(short)hPx
		   vertPixels:(short)vPx
			 fontSize:(short)size
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	int i, j;
	hPixels = hPx;
	vPixels = vPx;
	theFontSize = size;
	
	for ( j = 0; j < kROWS; j++ ) {
		for ( i = 0; i < kCOLS; i++ ) {
			[attributes[i][j] setObject:[NSFont fontWithName: @"Monaco" size:theFontSize]
			 forKey:NSFontAttributeName];
			rectArray[i][j] = NSMakeRect(hPixels*i, (vPixels * kROWS)-(vPixels*(j+1)), hPixels, vPixels);
		}
	}
	characterSize = [@"a" sizeWithAttributes:attributes[0][0]];
	[pool drain];
}

@end
