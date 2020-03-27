CC=gcc
CFLAGS=-Wall
DEPS=bootparser.h
OBJ=bootparser.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

bootparser: bootparser.o
	$(CC) -o $@ $^ $(CFLAGS)

clean: 
	rm bootparser bootparser.o
