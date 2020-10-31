CC= gcc
CFLAGS = -Wall -std=c99
PTHREADS = -pthread


all: main

main:
	$(CC) server.c $(CFLAGS) $(PTHREADS) -o server

.PHONY: clean

clean:
	rm -f server *.o
