VERSION:=$(shell git describe --tags --long --dirty --always)
CCFLAGS=-DVERSION=\"${VERSION}\"

EXES=bin/doublet bin/mkm3u bin/fillstick bin/mkplaylists bin/fixdatenames 
# CCFLAGS+=-g

# Keep object files
.PRECIOUS: %.o

all: $(EXES)

clean:
	rm -f *.o
	rm -f $(EXES)

bin/%: utils.o %.o
	gcc $(CCFLAGS) $^ -o $@

%.o: %.c utils.h
	gcc $(CCFLAGS) -c $<
