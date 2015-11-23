# msgpack

A work in progress for efficient handling of msgpack data, designed for use in pebble projects and therefore with minimal dependencies (std*) and written in C.

## Building

You can build msgpack test suite and run it by running

``` make ```

This will build the test harness and build the tests for the msgpack.c source.  It will then run the tests.

It will only build a msgpack.o if the tests pass.

## Usage

The intent is that I'll be able to use msgpack instead of JSON for my API responses and basically use the data structure inline to conserve memory and CPU.

### API

`mpobject` populates an `mp_object` structure with the type, length, data pointer and next pointer.

With container structures, the next pointer is the same as the data pointer, ie. NOT at the end of the container data.

The intent is to make this like a file api.

| Function              | Operation                                |
| --------------------- | ---------------------------------------- |
| `mpopen(buffer,size)` | Open buffer as msgpack                   |
| `mpread(mp,obj)`      | Read a msgpack object, pointer moves to next object (first member for containers) |
| `mpseek(mp,type,pos)` | Seek within buffer: absolute (by pointer), relative (only positive integers), start of current container, end of current container. |
| `mppos(mp, type)`     |                                          |

Implementation will maintain a stack of container start locations and object positions, backing out as the end of each container is reached.

