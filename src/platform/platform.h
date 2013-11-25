#include "Rogue.h"

struct brogueConsole {
	void (*gameLoop)();
	boolean (*pauseForMilliseconds)(short milliseconds);
	void (*nextKeyOrMouseEvent)(rogueEvent *returnEvent, boolean colorsDance);
	void (*plotChar)(uchar, short, short, short, short, short, short, short, short);
};


#ifdef BROGUE_TCOD
extern struct brogueConsole tcodConsole;
#endif

#ifdef BROGUE_CURSES
extern struct brogueConsole cursesConsole;
#endif

extern struct brogueConsole currentConsole;

