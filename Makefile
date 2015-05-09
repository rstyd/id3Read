CC=gcc
CFLAGS=-Wall -std=c99 -g

BINS=id3Read

all: $(BINS)

id3Read:  id3.c  
	$(CC) $(CFLAGS)  -o id3Read id3.c  

clean:
	rm $(BINS) 

