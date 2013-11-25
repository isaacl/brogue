#ifdef BROGUE_CURSES
#include <ncurses.h>
#include "term.h"
#include <math.h>

#define COLORING(fg,bg) (((fg) & 0x0f) | (((bg) & 0x07) << 4))
#define COLOR_FG(color,fg) (((fg) & 0x0f) + ((color) & 0x70))
#define COLOR_BG(color,bg) (((color) & 0x0f) + (((bg) & 0x07) << 4))
#define COLOR_INDEX(color) (1 + ((color)&0x07) + (((color) >> 1) & 0x38))
#define COLOR_ATTR(color) (COLOR_PAIR(COLOR_INDEX(color)) | (((color)&0x08) ? A_BOLD : 0))

static struct { int curses, color; } videomode = { 0, 0 };

static struct { int width, height; } minsize = { 80, 24 };

static void preparecolor ( ) {
	static int pairParts[8] = {
		COLOR_BLACK, COLOR_RED, COLOR_GREEN, COLOR_YELLOW,
		COLOR_BLUE, COLOR_MAGENTA, COLOR_CYAN, COLOR_WHITE
	};
	
	int fg, bg; 
	for (bg=0; bg<8; bg++) {
		for (fg=0; fg<8; fg++) {
			init_pair(
				COLOR_INDEX(COLORING(fg, bg)),
				pairParts[fg], pairParts[bg]
			);
		}
	}
}

static void term_title(const char *title) {
	printf ("\033]2;\n%s\n\007", title); // ESC ]2; title BEL
}

static int curses_init( ) {
	if (videomode.curses) return 0;
	
	// isterm?
	initscr( );
	if (!has_colors( )) {
		endwin( );
		fprintf (stderr, "Your terminal has no color support.\n");
		return 1;
	}
	
	start_color( );
	clear( );
	curs_set( 0 );
	refresh( );
	leaveok(stdscr, TRUE);
	preparecolor( );
	cbreak( );
	noecho( );

	nodelay(stdscr, TRUE);
	meta(stdscr, TRUE);
	keypad(stdscr, TRUE);

	mousemask(BUTTON1_PRESSED | BUTTON1_RELEASED | REPORT_MOUSE_POSITION | BUTTON_SHIFT | BUTTON_CTRL, NULL);
	mouseinterval(0); //do no click processing, thank you
	
	videomode.curses = 1;

	getmaxyx(stdscr, Term.height, Term.width);

	return 1;
}


static int term_start() {
	return curses_init();
}

static void term_end() {
	clear();
	refresh();
	endwin();
}

typedef struct CIE {
	float X, Y, Z;
	float x, y, z;
} CIE;

typedef struct Lab {
	float L, a, b;
} Lab;

#define DARK 0.0
#define DIM 0.1
#define MID 0.3
#define HALFBRIGHT 0.5
#define BRIGHT 0.9

fcolor palette[16] = {
	{DARK, DARK, DARK},
	{MID, DARK, DARK},
	{DARK, MID, DARK},
	{MID, .8 * MID, DIM},
	{DARK, DARK, MID},
	{MID + DIM, DARK, MID},
	{DARK, MID, MID},
	{HALFBRIGHT, HALFBRIGHT, HALFBRIGHT},

	{MID, MID, MID},
	{BRIGHT, DARK, DARK},
	{DARK, BRIGHT, DARK},
	{BRIGHT, BRIGHT, DARK},
	{HALFBRIGHT, MID, BRIGHT},
	{BRIGHT, HALFBRIGHT, BRIGHT},
	{DARK, BRIGHT, BRIGHT},
	{BRIGHT, BRIGHT, BRIGHT}
};

CIE ciePalette[16];
Lab labPalette[16];
CIE adamsPalette[16];

static CIE toCIE(fcolor c) {
	double a = 0.055;

	// http://en.wikipedia.org/wiki/SRGB_color_space#The_reverse_transformation

	c.r = c.r <= 0.04045 ? c.r / 12.92 : pow((c.r + a) / (1 + a), 2.4);
	c.g = c.g <= 0.04045 ? c.g / 12.92 : pow((c.g + a) / (1 + a), 2.4);
	c.b = c.b <= 0.04045 ? c.b / 12.92 : pow((c.b + a) / (1 + a), 2.4);
	
	CIE cie;
	cie.X = 0.4124 * c.r + 0.3576 * c.g + 0.1805 * c.b;
	cie.Y = 0.2126 * c.r + 0.7152 * c.g + 0.0722 * c.b;
	cie.Z = 0.0193 * c.r + 0.1192 * c.g + 0.9505 * c.b;

	float sum = cie.X + cie.Y + cie.Z;
	if (sum == 0.0) sum = 1.0;
	cie.x = cie.X / sum;
	cie.y = cie.Y / sum;
	cie.z = 1.0 - cie.x - cie.y;

	return cie;
}

static float Labf(float t) {
	return t > ((6.0/29.0) * (6.0/29.0) * (6.0/29.0)) ? pow(t, 1.0/3.0) : ((1.0/3.0) * (29.0 / 6.0) * (29.0 / 6.0)) * t + (4.0 / 29.0);
}

static CIE white;

static Lab toLab(CIE *c) {
	CIE n = (CIE) {Labf(c->X / white.X), Labf(c->Y / white.Y), Labf(c->Z / white.Z)};
	Lab l;

	// http://en.wikipedia.org/wiki/L*a*b*#RGB_and_CMYK_conversions
	l.L = 116.0 * n.Y - 16;
	l.a = 500.0 * (n.X - n.Y);
	l.b = 200.0 * (n.Y - n.Z);

	return l;
}

static float munsellSloanGodlove(float t) {
	return sqrt(1.4742 * t - 0.004743 * t * t);
}

static CIE adams(CIE *v) {
	CIE c;
	c.Y = munsellSloanGodlove(v->Y);
	c.X = munsellSloanGodlove((white.Y / white.X) * v->X) - c.Y;
	c.Z = munsellSloanGodlove((white.Z / white.X) * v->Z) - c.Y;

	return c;
}

#define SQUARE(x) ((x) * (x))

static float CIE76(Lab *L1, Lab *L2) {
	// http://en.wikipedia.org/wiki/Color_difference#CIE76
	float lbias = 1.0;
	return sqrt(lbias * SQUARE(L2->L - L1->L) + SQUARE(L2->a - L1->a) + SQUARE(L2->b - L1->b));
}

static float CIExyY(CIE *L1, CIE *L2) {
	// this does a good job of estimating the difference between two colors, ignoring brightness
	return sqrt(SQUARE(L2->x - L1->x) + SQUARE(L2->y - L1->y));
}

static float adamsDistance(CIE *v1, CIE *v2) {
	// not really the right metric, this
	// return sqrt(SQUARE(v2->X - v1->X) + SQUARE(v2->Y - v1->Y) + SQUARE(v2->Z - v1->Z));
	return sqrt(SQUARE(v2->X - v1->X) + SQUARE(v2->Y - v1->Y) + SQUARE(v2->Z - v1->Z));
}

static int best (fcolor *fg, fcolor *bg) {
	// analyze fg & bg for their contrast
	CIE cieFg = toCIE(*fg);
	CIE cieBg = toCIE(*bg);
	Lab labFg = toLab(&cieFg);
	Lab labBg = toLab(&cieBg);
	CIE adamsFg = adams(&cieFg);
	CIE adamsBg = adams(&cieBg);
	
	float JND = 2.3; // just-noticeable-difference
	int areTheSame = CIE76(&labFg, &labBg) <= 2.0 * JND; // a little extra fudge
	int bestpair = 0;
	float bestrating = 10000.0;

	fcolor sRGB_white = (fcolor) {1, 1, 1};
	white = toCIE(sRGB_white);
	int i, j;

	for (i = 0; i < 16; i++) {
		ciePalette[i] = toCIE(palette[i]);
		labPalette[i] = toLab(&ciePalette[i]);
		adamsPalette[i] = adams(&ciePalette[i]);
	}

	float fgscore[16], bgscore[16];

	for (i = 0; i < 16; i++) {
		fgscore[i] = 0.0;
		fgscore[i] += CIE76(labPalette + i, &labFg);
		//fgscore[i] += CIExyY(ciePalette + i, &cieFg);
		//fgscore[i] += 50 * adamsDistance(adamsPalette + i, &adamsFg);

		bgscore[i] = 0.0;
		bgscore[i] += CIE76(labPalette + i, &labBg);
		//bgscore[i] += CIExyY(ciePalette + i, &cieBg);
		//bgscore[i] += 50 * adamsDistance(adamsPalette + i, &adamsBg);
	}

	for (i = 0; i < 8; i++) {
		for (j = 0; j < 16; j++) {
			if ((i == j && !areTheSame)) continue;
			// if (j == 8) continue; // for terms with no bright black

			// float rating = closeness(ciePalette + i, &cieBg) + closeness(ciePalette + j, &cieFg);
			float rating = fgscore[j] + bgscore[i];

			if (rating < bestrating) {
				bestpair = COLORING(j, i);
				bestrating = rating;
			}
		}
	}
	return bestpair;
}

static int coerce (fcolor *color, float dark, float saturation, float brightcut, float grey) {
	float bright = color->r;
	if (color->g > bright) bright = color->g;
	if (color->b > bright) bright = color->b;
	if (bright < dark) {
		if (bright > grey) {
			return 8;
		}
		return 0;
	}
	float cut = bright * saturation;

	int r = color->r > cut, g = color->g > cut, b = color->b > cut;
	return r + g * 2 + b * 4 + ((bright > brightcut) ? 8 : 0);
}

static int coerce_color (fcolor *fg, fcolor *bg) {
	int f = coerce(fg, 0.3, 0.8, .80, .1);
	int b = 7 & coerce(bg, 0.3, 0.35, 1, 0);
	if (f == b) f ^= 8;
	return COLORING(f, b);
}

static void term_mvaddch(int x, int y, int ch, fcolor *fg, fcolor *bg) {
	// int c = coerce_color(fg, bg);
	int c = best(fg, bg);
	attrset(COLOR_ATTR(c));
	mvaddch(y, x, ch);
}

static void term_refresh() {
	// to set up a 256-color terminal, see:
	// http://push.cx/2008/256-color-xterms-in-ubuntu
	if (0 && can_change_color()) {
		int i;
		for (i = 0; i < 16; i++) {
			short r = palette[i].r * 1000;
			short g = palette[i].g * 1000;
			short b = palette[i].b * 1000;
			if (r < 0) r = 0;
			if (g < 0) g = 0;
			if (b < 0) b = 0;
			init_color(i + 1, r, g, b);
		}
	}
	if (0) {
		int i;
		short r, g, b;
		for (i = 0; i < 8; i++) {
			color_content(i, &r, &g, &b);
			palette[i].r = r * .001;
			palette[i].g = g * .001;
			palette[i].b = b * .001;
		}
	}
	refresh();
}

static void ensure_size( );

static int term_getkey( ) {
	Term.mouse.justPressed = 0;
	Term.mouse.justReleased = 0;
	Term.mouse.justMoved = 0;

	while (1) {
		int got = getch();
		if (got == KEY_RESIZE) {
			ensure_size( );
		} else if (got == KEY_MOUSE) {
			MEVENT mevent;
			getmouse (&mevent);
			Term.mouse.x = mevent.x;
			Term.mouse.y = mevent.y;
			Term.mouse.shift = (mevent.bstate & BUTTON_SHIFT) != 0;
			Term.mouse.control = (mevent.bstate & BUTTON_CTRL) != 0;
			if (mevent.bstate & BUTTON1_PRESSED) {
				Term.mouse.justPressed = 1;
				Term.mouse.isPressed = 1;
			} else if (mevent.bstate & BUTTON1_RELEASED) {
				if (Term.mouse.isPressed) {
					Term.mouse.justReleased = 1;
					Term.mouse.isPressed = 0;
				}
			} else {
				Term.mouse.justMoved = 1;
			}
			return TERM_MOUSE;
		} else {
			if (got == KEY_ENTER) got = 13; // KEY_ENTER -> ^M for systems with odd values for KEY_ENTER
			if (got == ERR) return TERM_NONE;
			else return got;
		}
	}
}

static int term_has_key() {
	int ch = getch();
	if (ch != ERR) {
		ungetch(ch);
		return 1;
	} else {
		return 0;
	}
}

static void ensure_size( ) {
	int w = minsize.width, h = minsize.height;

	getmaxyx(stdscr, Term.height, Term.width);
	if (Term.height < h || Term.width < w) {
		// resize_term(h, w);
		getmaxyx(stdscr, Term.height, Term.width);
		nodelay(stdscr, FALSE);
		while (Term.height < h || Term.width < w) {
			erase();
			attrset(COLOR_ATTR(7));

			mvprintw(1,0,"Brogue needs a terminal window that is at least [%d x %d]", w, h);

			attrset(COLOR_ATTR(15));
			mvprintw(2,0,"If your terminal can be resized, resize it now.\n");

			attrset(COLOR_ATTR(7));
			mvprintw(3,0,"Press ctrl-c at any time to quit.\n");
#ifdef BROGUE_TCOD
			mvprintw(5,0,"To use libtcod, start the game with the -gl or -s.\n\n");
#endif

			printw("Width:  %d/%d\n", Term.width, w);
			printw("Height: %d/%d\n", Term.height, h);
			
			getch();
			getmaxyx(stdscr, Term.height, Term.width);
		}
		nodelay(stdscr, TRUE);
		erase();
		refresh();
	}
}

static void term_resize(int w, int h) {
	minsize.width = w;
	minsize.height = h;
	ensure_size();
}

static void term_wait(int ms) {
	napms(ms);
}

struct term_t Term = {
	term_start,
	term_end,
	term_mvaddch,
	term_refresh,
	term_getkey,
	term_wait,
	term_has_key,
	term_title,
	term_resize,
	{KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_BACKSPACE, KEY_DC, KEY_F(12)}
};
#endif

