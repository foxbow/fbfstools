EXES=bin/doublet bin/mkm3u bin/fillstick bin/mkplaylists 
# bin/sortlink 

.PRECIOUS: %.o

all: $(EXES)

clean:
	rm -f *.o
	rm -f $(EXES)

bin/%: utils.o %.o
	gcc $^ -o $@

%.o: %.c
	gcc -c $<
