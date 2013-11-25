#ifdef BROGUE_TCOD
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "libtcod.h"
#include "platform.h"

extern playerCharacter rogue;
extern TCOD_renderer_t renderer;
extern short brogueFontSize;

short mouseX, mouseY;

static int isFullScreen = false;
static int hasMouseMoved = false;

static TCOD_key_t bufferedKey = {TCODK_NONE};
static short bufferedMouseClick[2] = {-1, -1};

static void gameLoop()
{
	char font[60];
	
	int screenWidth, screenHeight;
	int fontWidths[5] = {128, 160, 192, 240, 288}; // widths of the font graphics (divide by 16 to get individual character width)

	TCOD_sys_get_current_resolution(&screenWidth, &screenHeight);
	for (brogueFontSize = 5; fontWidths[brogueFontSize - 1] * COLS / 16 >= screenWidth && brogueFontSize > 1; brogueFontSize--);

	sprintf(font, "BrogueFont%i.png", brogueFontSize);
	
	TCOD_console_set_custom_font(font, (TCOD_FONT_TYPE_GREYSCALE | TCOD_FONT_LAYOUT_ASCII_INROW), 0, 0);
	TCOD_console_init_root(COLS, ROWS, "Brogue", false, renderer);
	TCOD_console_map_ascii_codes_to_font(0, 255, 0, 0);
	TCOD_console_set_keyboard_repeat(175, 30);
	TCOD_mouse_show_cursor(1);
	
	do {
		rogueMain();
		
		if (!TCOD_console_is_window_closed()) {
			rogue.gameHasEnded = true;
			waitForAcknowledgment();
		}
	} while (!TCOD_console_is_window_closed());
}

void tcod_plotChar(uchar inputChar,
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
	
	if (inputChar == STATUE_CHAR) {
		inputChar = 223;
	} else if (inputChar > 255) {
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

static void initWithFont(int fontSize) {
	char font[80];
	
	sprintf(font,"BrogueFont%i.png",fontSize);
	
	TCOD_console_set_custom_font(font, (TCOD_FONT_TYPE_GREYSCALE | TCOD_FONT_LAYOUT_ASCII_INROW), 0, 0);
	TCOD_console_init_root(COLS, ROWS, "Brogue", isFullScreen, renderer);
	TCOD_console_map_ascii_codes_to_font(0, 255, 0, 0);
	TCOD_console_set_keyboard_repeat(175, 30);
	refreshScreen();
}

boolean processSpecialKeystrokes(TCOD_key_t k) {
	
	if (k.vk == TCODK_PRINTSCREEN) {
		// screenshot
		TCOD_sys_save_screenshot(NULL);
		return true;
	} else if ( (k.vk == TCODK_F12) || (k.lalt && (k.vk == TCODK_ENTER || k.vk == TCODK_KPENTER) )) {
		// switch fullscreen (DISABLED)
		// isFullScreen = !isFullScreen;
		// TCOD_console_delete(NULL);
		// initWithFont(brogueFontSize);
		// TCOD_console_flush();
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

static boolean tcod_pauseForMilliseconds(short milliseconds) {
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


#define PAUSE_BETWEEN_EVENT_POLLING		34//17

static void tcod_nextKeyOrMouseEvent(rogueEvent *returnEvent, boolean colorsDance) {
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
		if (mouse.lbutton_pressed || mouseX !=x || mouseY != y) {
			if (x > 0 && y > 0 && x < COLS && y < ROWS) {
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
				if (returnEvent->eventType == MOUSE_ENTERED_CELL && !hasMouseMoved) {
					hasMouseMoved = true;
				} else {
					return;
				}
			}
		}
		waitTime = PAUSE_BETWEEN_EVENT_POLLING + theTime - TCOD_sys_elapsed_milli();
		
		if (waitTime > 0 && waitTime <= PAUSE_BETWEEN_EVENT_POLLING) {
			TCOD_sys_sleep_milli(waitTime);
		}
	}
}

struct brogueConsole tcodConsole = {
	gameLoop,
	tcod_pauseForMilliseconds,
	tcod_nextKeyOrMouseEvent,
	tcod_plotChar
};

#endif

