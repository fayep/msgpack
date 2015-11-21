.PHONY: all clean
all: runtest
CC=gcc
CFLAGS=-O2
TESTFLAGS=-d

%.to: %.c
	gcc -g -O2 -c -D TEST -o $@ $<

test: test.o msgpack.to

runtest: test
	./test $(TESTFLAGS)

clean:
	-rm -f *.to
	-rm -f *.o
	-rm -f test
