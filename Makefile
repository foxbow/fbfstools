OBJS=doublet.o mkm3u.o utils.o 
S_OBJS=sortlink.o utils.o
D_OBJS=doublet.o utils.o
M_OBJS=mkm3u.o utils.o
F_OBJS=fillstick.o utils.o
EXES=bin/doublet bin/mkm3u bin/sortlink bin/fillstick
CFLAGS=-c -pedantic -Os -std=c99

all: $(EXES)

clean:
	rm -f $(OBJS)
	rm -f bin/*

bin/fillstick: $(F_OBJS)
	gcc $(F_OBJS) -o bin/fillstick

bin/sortlink: $(S_OBJS)
	gcc $(S_OBJS) -o bin/sortlink

bin/doublet: $(D_OBJS)
	gcc $(D_OBJS) -o bin/doublet

bin/mkm3u: $(M_OBJS)
	gcc $(M_OBJS) -o bin/mkm3u

.c.o:
	gcc $(CFLAGS) -c $<
