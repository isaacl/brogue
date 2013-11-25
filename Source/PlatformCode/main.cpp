#ifdef __APPLE__
#include <iostream>
#include "CoreFoundation/CoreFoundation.h"
#endif

#include "libtcod.hpp"
#include "Rogue.h"

//TCOD_renderer_t renderer=TCOD_RENDERER_GLSL;
TCOD_renderer_t renderer=TCOD_RENDERER_OPENGL;
//TCOD_renderer_t renderer=TCOD_RENDERER_SDL;

short brogueFontSize = 3;
short mouseX = 0, mouseY = 0;

extern color gray;
extern color black;
extern playerCharacter rogue;

void init_relative_paths()
{
	// ----------------------------------------------------------------------------
	// This makes relative paths work in C++ in Xcode by changing directory to the Resources folder inside the .app bundle
#ifdef __APPLE__    
    CFBundleRef mainBundle = CFBundleGetMainBundle();
    CFURLRef resourcesURL = CFBundleCopyResourcesDirectoryURL(mainBundle);
    char path[PATH_MAX];
    if (!CFURLGetFileSystemRepresentation(resourcesURL, TRUE, (UInt8 *)path, PATH_MAX))
    {
        // error!
    }
    CFRelease(resourcesURL);
	
    chdir(path);
#endif
	// ----------------------------------------------------------------------------
	return;
}

int main(int argc, char *argv[])
{
	
	// Custom font:
	char font[60];
	
#ifndef __APPLE__  // have to disable this for me because getCurrentResolution crashes
	int screenWidth, screenHeight;
	int fontWidths[5] = {128, 160, 192, 240, 288}; // widths of the font graphics (divide by 16 to get individual character width)

	TCODSystem::getCurrentResolution(&screenWidth, &screenHeight);
	for (brogueFontSize = 5; fontWidths[brogueFontSize - 1] * COLS / 16 >= screenWidth && brogueFontSize > 1; brogueFontSize--);
#endif
	
	init_relative_paths();
	
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--SDL") == 0) {
			renderer = TCOD_RENDERER_SDL;
		}
	}
	
	sprintf(font, "BrogueFont%i.png", brogueFontSize);
	
	TCODConsole::setCustomFont(font, (TCOD_FONT_TYPE_GREYSCALE | TCOD_FONT_LAYOUT_ASCII_INROW), 0, 0);
	TCODConsole::initRoot(COLS, ROWS, "Brogue", false, renderer);
	TCODConsole::mapAsciiCodesToFont(0, 255, 0,0);
	TCODConsole::setKeyboardRepeat(175, 30);
	TCODMouse::showCursor(true);
	
	do {
		rogueMain();
		
		if (!TCODConsole::isWindowClosed()) {
			rogue.gameHasEnded = false; // so that the printString command works
			printString((char *) "                       Press space to play again.                       ", (COLS - 72 + 1) / 2, ROWS - 1, &gray, &black, 0);
			rogue.gameHasEnded = true;
			waitForAcknowledgment();
		}
	} while (!TCODConsole::isWindowClosed());
	
	return 0;
}

