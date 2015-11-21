.PHONY: all clean
all: runtest
CC=gcc
CFLAGS=-O2

%.to: %.c
	gcc -g -O2 -c -D TEST -o $@ $<

test: test.o msgpack.to

runtest: test
	./test

clean:
	-rm -f *.to
	-rm -f *.o
	-rm -f test
