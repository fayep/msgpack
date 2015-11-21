# msgpack

A work in progress for efficient handling of msgpack data, designed for use in pebble projects and therefore with minimal dependencies (std*) and written in C.

## Building

You can build msgpack test suite and run it by running

``` make ```

This will build the test harness and build the tests for the msgpack.c source.  It will then run the tests.

Eventually, we will only build a msgpack.o if the tests pass.

## Usage

The intent is that I'll be able to use msgpack instead of JSON for my API responses and basically use the data structure inline to conserve memory and CPU.

