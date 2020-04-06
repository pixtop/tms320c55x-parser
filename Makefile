CC=gcc
CFLAGS=-Wall
DEPS=bootparser.h coffparser.h

all: bootparser coffparser

bootparser: bootparser.o
	$(CC) -o $@ $^ $(CFLAGS)

coffparser: coffparser.o
	$(CC) -o $@ $^ $(CFLAGS)

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

clean:
	rm bootparser coffparser *.o
