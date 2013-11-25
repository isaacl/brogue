#include "platform.h"

#ifdef BROGUE_TCOD
#include "libtcod.h"
TCOD_renderer_t renderer=TCOD_RENDERER_SDL; // the sdl renderer is more reliable
short brogueFontSize = 3;
#endif

extern playerCharacter rogue;
struct brogueConsole currentConsole;

boolean serverMode = false;
boolean noMenu = false;

void dumpScores();

static boolean endswith(const char *str, const char *ending)
{
	int str_len = strlen(str), ending_len = strlen(ending);
	if (str_len < ending_len) return false;
	return strcmp(str + str_len - ending_len, ending) == 0 ? true : false;
}

static void append(char *str, char *ending, int bufsize) {
	int str_len = strlen(str), ending_len = strlen(ending);
	if (str_len + ending_len + 1 > bufsize) return;
	strcpy(str + str_len, ending);
}

int main(int argc, char *argv[])
{
#ifdef BROGUE_TCOD
		currentConsole = tcodConsole;
#else
		currentConsole = cursesConsole;
#endif

	rogue.nextGame = NG_NOTHING;
	rogue.nextGamePath[0] = '\0';

	int i;
	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--scores") == 0) {
			// just dump the scores and quit!
			dumpScores();
			return 0;
		}

		if (strcmp(argv[i], "--seed") == 0) {
			// pick a seed!
			if (i + 1 < argc) {
				int seed = atoi(argv[i + 1]);
				if (seed != 0) {
					i++;
					continue;
				}
			}
		}

		if(strcmp(argv[i], "-n") == 0) {
			rogue.nextGame = NG_NEW_GAME;
			continue;
		}

		if(strcmp(argv[i], "--no-menu") == 0) {
			rogue.nextGame = NG_NEW_GAME;
			noMenu = true;
			continue;
		}

		if(strcmp(argv[i], "--noteye-hack") == 0) {
			serverMode = true;
			continue;
		}

		if(strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--open") == 0) {
			if (i + 1 < argc) {
				strncpy(rogue.nextGamePath, argv[i + 1], 4096);
				rogue.nextGamePath[4095] = '\0';
				rogue.nextGame = NG_OPEN_GAME;

				if (!endswith(rogue.nextGamePath, GAME_SUFFIX)) {
					append(rogue.nextGamePath, GAME_SUFFIX, 4096);
				}

				i++;
				continue;
			}
		}

		if(strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--view") == 0) {
			if (i + 1 < argc) {
				strncpy(rogue.nextGamePath, argv[i + 1], 4096);
				rogue.nextGamePath[4095] = '\0';
				rogue.nextGame = NG_VIEW_RECORDING;

				if (!endswith(rogue.nextGamePath, RECORDING_SUFFIX)) {
					append(rogue.nextGamePath, RECORDING_SUFFIX, 4096);
				}

				i++;
				continue;
			}
		}

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

		// maybe it ends with .broguesave or .broguerec, then?
		if (endswith(argv[i], GAME_SUFFIX)) {
			strncpy(rogue.nextGamePath, argv[i], 4096);
			rogue.nextGamePath[4095] = '\0';
			rogue.nextGame = NG_OPEN_GAME;
			continue;
		}

		if (endswith(argv[i], RECORDING_SUFFIX)) {
			strncpy(rogue.nextGamePath, argv[i], 4096);
			rogue.nextGamePath[4095] = '\0';
			rogue.nextGame = NG_VIEW_RECORDING;
			continue;
		}
	}
	
	loadKeymap();
	currentConsole.gameLoop();
	
	return 0;
}

