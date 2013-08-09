OBJS=doublet.o mkm3u.o recurse.o fail.o fncmp.o activity.o
S_OBJS=sortlink.o recurse.o fail.o activity.o
D_OBJS=doublet.o recurse.o fail.o fncmp.o activity.o
M_OBJS=mkm3u.o recurse.o fail.o activity.o
EXES=bin/doublet bin/mkm3u bin/sortlink
CFLAGS=-c -pedantic -Os -std=c99

all: $(EXES)

clean:
	rm -f $(OBJS)
	rm -f bin/*

bin/sortlink: $(S_OBJS)
	gcc $(S_OBJS) -o bin/sortlink

bin/doublet: $(D_OBJS)
	gcc $(D_OBJS) -o bin/doublet

bin/mkm3u: $(M_OBJS)
	gcc $(M_OBJS) -o bin/mkm3u

.c.o:
	gcc $(CFLAGS) -c $<
