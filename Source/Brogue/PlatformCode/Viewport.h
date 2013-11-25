//
//  Viewport.h
//  Brogue
//
//  Created by Brian and Kevin Walker.
//  Copyright 2012. All rights reserved.
//
//  This file is part of Brogue.
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU Affero General Public License as
//  published by the Free Software Foundation, either version 3 of the
//  License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Affero General Public License for more details.
//
//  You should have received a copy of the GNU Affero General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#import <Cocoa/Cocoa.h>

#define	VERT_PX		18
#define	HORIZ_PX	11

#define kROWS		(30+3+1)
#define kCOLS		100

// This is only used as a starting point for the calculation after the
// window resizes.
#define FONT_SIZE	10
// This is only the basic font.  There are also fixed-name fallback fonts.
#define FONT_NAME	@"Monaco"

@interface Viewport : NSView
{
	NSString *letterArray[kCOLS][kROWS];
	NSColor *bgColorArray[kCOLS][kROWS];
	NSMutableDictionary *attributes[kCOLS][kROWS];
	NSMutableDictionary *characterSizeDictionary;
	NSRect rectArray[kCOLS][kROWS];
}

- (BOOL)isOpaque;

- (void)setString:(NSString *)c
   withBackground:(NSColor *)bgColor
  withLetterColor:(NSColor *)letterColor
	  atLocationX:(short)x
		locationY:(short)y
    withFancyFont:(bool)fancyFont;

- (void)drawTheString:(NSString *)theString centeredIn:(NSRect)rect withAttributes:(NSMutableDictionary *)theAttributes;

- (short)horizWindow;
- (short)vertWindow;
- (short)fontSize;
- (NSString *)fontName;
- (void)setHorizWindow:(short)hPx
			vertWindow:(short)vPx
			  fontSize:(short)size;

@end
