#include "platform.h"

#ifdef BROGUE_TCOD
#include "libtcod.h"
TCOD_renderer_t renderer=TCOD_RENDERER_OPENGL;
short brogueFontSize = 3;
#endif

extern playerCharacter rogue;
struct brogueConsole currentConsole;

int main(int argc, char *argv[])
{
#ifdef BROGUE_TCOD
		currentConsole = tcodConsole;
#else
		currentConsole = cursesConsole;
#endif

	int i;
	for (i = 1; i < argc; i++) {
#ifdef BROGUE_TCOD
		if (strcmp(argv[i], "--SDL") == 0 || strcmp(argv[i], "-s") == 0) {
			renderer = TCOD_RENDERER_SDL;
			currentConsole = tcodConsole;
		}
		if (strcmp(argv[i], "--opengl") == 0 || strcmp(argv[i], "-gl") == 0) {
			renderer = TCOD_RENDERER_OPENGL;
			currentConsole = tcodConsole;
		}
#endif
#ifdef BROGUE_CURSES
		if (strcmp(argv[i], "--term") == 0 || strcmp(argv[i], "-t") == 0) {
			currentConsole = cursesConsole;
		}
#endif
	}
	
	currentConsole.gameLoop();
	
	return 0;
}

