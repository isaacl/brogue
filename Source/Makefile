LIBTCODDIR=libtcod-1.5.1
CFLAGS=$(FLAGS) -I$(LIBTCODDIR)/include -IBrogueCode -IPlatformCode -Wall

%.o : %.cpp
	g++ $(CFLAGS) -O2 -s -o $@ -c $< 
%.o : %.c
	gcc $(CFLAGS) -O2 -s -o $@ -c $< 

OBJS=BrogueCode/Architect.o \
	BrogueCode/Combat.o \
	BrogueCode/Dijkstra.o \
	BrogueCode/Globals.o \
	BrogueCode/IO.o \
	BrogueCode/Items.o \
	BrogueCode/Light.o \
	BrogueCode/Monsters.o \
	BrogueCode/Movement.o \
	BrogueCode/RogueMain.o \
	BrogueCode/Random.o \
	BrogueCode/Recordings.o \
	PlatformCode/main.o \
	PlatformCode/platformdependent.o 

all : brogue

brogue-icon.o : brogue-icon.ico icon.rc
	windres icon.rc brogue-icon.o

brogue : ${OBJS} brogue-icon.o 
	g++ -o brogue.exe ${OBJS} brogue-icon.o -L. -ltcod-mingw -mwindows
