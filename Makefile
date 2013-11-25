
LIBTCODDIR=src/libtcod-1.5.1
CFLAGS=$(FLAGS) -I$(LIBTCODDIR)/include -Isrc/brogue -Isrc/platform -Wall
RELEASENAME=brogue-1.4.3

%.o : %.cpp
	g++ $(CFLAGS) -g -o $@ -c $< 
%.o : %.c
	gcc $(CFLAGS) -g -o $@ -c $< 

OBJS=src/brogue/Architect.o \
	src/brogue/Combat.o \
	src/brogue/Dijkstra.o \
	src/brogue/Globals.o \
	src/brogue/IO.o \
	src/brogue/Items.o \
	src/brogue/Light.o \
	src/brogue/Monsters.o \
	src/brogue/Movement.o \
	src/brogue/Recordings.o \
	src/brogue/RogueMain.o \
	src/brogue/Random.o \
	src/platform/main.o \
	src/platform/platformdependent.o 

all : bin/brogue

clean : 
	rm src/brogue/*.o src/platform/*.o bin/brogue

tar : bin/brogue
	rm -f ${RELEASENAME}.tar.gz
	tar --transform 's,^,${RELEASENAME}/,' -czf ${RELEASENAME}.tar.gz \
	Makefile \
	$(wildcard *.sh) \
	$(wildcard *.rtf) \
	$(wildcard *.txt) \
	bin/brogue \
	$(wildcard bin/*.png) \
	$(wildcard bin/*.so) \
	$(wildcard src/*.sh) \
	$(wildcard src/brogue/*.c*) \
	$(wildcard src/brogue/*.h*) \
	$(wildcard src/platform/*.c*) \
	$(wildcard src/platform/*.h*) 

${LIBTCODDIR} :
	src/get-libtcod.sh

bin/brogue : ${LIBTCODDIR} ${OBJS} 
	g++ -O3 -o bin/brogue ${OBJS} -L. -L${LIBTCODDIR} -ltcod -ltcodxx -Wl,-rpath,.

