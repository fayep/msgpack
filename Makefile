.PHONY: all clean buildobjects runtest
all: buildobjects
CC ?= gcc
CFLAGS ?= -O2
DEBUGCFLAGS = $(CFLAGS) -g
TESTFLAGS ?= -d
TARGETS = msgpack
TEST_TARGETS = $(addsuffix .t,$(TARGETS))
OBJECTS = $(addsuffix .o,$(TARGETS))

%.t: %.to test.to
	gcc -o $@ $^

%.o: %.c
	gcc $(CFLAGS) -c -o $@ $<

%.to: %.c
	gcc $(DEBUGCFLAGS) -c -D TEST -o $@ $<

runtest: $(TEST_TARGETS)
	@for t in $(TEST_TARGETS); do cmd="./$$t $(TESTFLAGS)"; echo "Running tests: $$t"; $$cmd; done

buildobjects: runtest $(OBJECTS)

clean:
	-rm -f *.to
	-rm -f *.[to]
