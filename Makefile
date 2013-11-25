
LIBTCODDIR=src/libtcod-1.5.1
CFLAGS=-Isrc/brogue -Isrc/platform -Wall -Wno-parentheses ${DEFINES}
RELEASENAME=brogue-1.6.1

%.o : %.c
	gcc $(CFLAGS) -g -o $@ -c $< 

BROGUEFILES=src/brogue/Architect.o \
	src/brogue/Combat.o \
	src/brogue/Dijkstra.o \
	src/brogue/Globals.o \
	src/brogue/IO.o \
	src/brogue/Items.o \
	src/brogue/Light.o \
	src/brogue/Monsters.o \
	src/brogue/Buttons.o \
	src/brogue/Movement.o \
	src/brogue/Recordings.o \
	src/brogue/RogueMain.o \
	src/brogue/Random.o \
	src/brogue/MainMenu.o \
	src/platform/main.o \
	src/platform/platformdependent.o \
	src/platform/curses-platform.o \
	src/platform/tcod-platform.o \
	src/platform/term.o

TCOD_DEF = -DBROGUE_TCOD -I$(LIBTCODDIR)/include
TCOD_DEP = ${LIBTCODDIR}
TCOD_LIB = -L. -L${LIBTCODDIR} -ltcod

CURSES_DEF = -DBROGUE_CURSES
CURSES_LIB = -lncurses -lm

all : both

tcod : DEPENDENCIES = ${TCOD_DEP}
tcod : DEFINES = ${TCOD_DEF}
tcod : LIBRARIES = ${TCOD_LIB}

curses : DEFINES = ${CURSES_DEF}
curses : LIBRARIES = ${CURSES_LIB}

both : DEPENDENCIES += ${TCOD_DEP}
both : DEFINES += ${TCOD_DEF} ${CURSES_DEF}
both : LIBRARIES += ${TCOD_LIB} ${CURSES_LIB}

both : bin/brogue

tcod : bin/brogue

curses : bin/brogue




bin/brogue : ${DEPENDENCIES} ${BROGUEFILES}
	gcc -O3 -o bin/brogue ${BROGUEFILES} ${LIBRARIES} -Wl,-rpath,.

clean : 
	rm -f src/brogue/*.o src/platform/*.o bin/brogue

${LIBTCODDIR} :
	src/get-libtcod.sh

tar : both
	rm -f ${RELEASENAME}.tar.gz
	tar --transform 's,^,${RELEASENAME}/,' -czf ${RELEASENAME}.tar.gz \
	Makefile \
	brogue \
	$(wildcard *.sh) \
	$(wildcard *.rtf) \
	readme \
	$(wildcard *.txt) \
	bin/brogue \
	bin/keymap \
	$(wildcard bin/fonts/*.png) \
	$(wildcard bin/*.so) \
	$(wildcard src/*.sh) \
	$(wildcard src/brogue/*.c) \
	$(wildcard src/brogue/*.h) \
	$(wildcard src/platform/*.c) \
	$(wildcard src/platform/*.h)

