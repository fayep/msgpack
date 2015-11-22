.PHONY: all clean buildobjects runtest
all: buildobjects
CC ?= gcc
CFLAGS ?= -O2
DEBUGFLAGS = -g3
TESTFLAGS ?= -d
TARGETS = test msgpack
TEST_TARGETS = $(addsuffix .t,$(TARGETS))
OBJECTS = $(addsuffix .o,$(TARGETS))

test.t: test.to
	gcc $(DEBUGFLAGS) -o $@ $<

%.t: %.to test.to
	gcc $(DEBUGFLAGS) -o $@ $^

%.o: %.c
	gcc $(CFLAGS) -c -o $@ $<

%.to: %.c
	gcc $(DEBUGFLAGS) -c -D TEST -o $@ $<

runtest: $(TEST_TARGETS)
	@for t in $(TEST_TARGETS); do cmd="./$$t $(TESTFLAGS)"; echo "Running tests: $$t"; $$cmd; done

buildobjects: runtest $(OBJECTS)

clean:
	-rm -f *.to
	-rm -f *.[to]
