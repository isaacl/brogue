
LIBTCODDIR=../../libtcod-1.5.1
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
	PlatformCode/main.o \
	PlatformCode/platformdependent.o 

all : brogue

clean :
	\rm -f ${OBJS} brogue

brogue : ${OBJS}
	g++ -o brogue ${OBJS} -L. -ltcod -ltcodxx -Wl,-rpath,.

