#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include "msgpack.h"

/* msgpack.c
   functions to work with msgpack format data.
*/

// returns the type of this field
mp_type mptype(unsigned char *id) {
  if (*id <= 0x7f) {
     return MP_UINT;
  } else if (*id >= 0xe0) {
     return MP_INT;
  } else if (*id <= 0x8f) {
     return MP_MAP;
  } else if (*id <= 0x9f) {
     return MP_ARRAY;
  } else if (*id <= 0xbf) {
     return MP_STRING;
  } else {
     return mp_typemap[*id - 0xc0];
  }
}

// returns the size of the data for this field
int mplen(unsigned char *id) {
  int len;
  if (*id <= 0x7f || *id >= 0xe0) {
     return 1;
  } else if (*id <= 0x9f) {
     return *id & 0x0f;
  } else if (*id <= 0xbf) {
     return *id & 0x1f;
  } else {
     unsigned char code = mp_lenmap[*id - 0xc0];
     len = code & 0x1f;
     if (code & MP_REF) {
        switch(len) {
        case 1:
          len = *(id+1);
          break;
        case 2:
          len = ntohs(*(unsigned short *)(id+1));
          break;
        case 4:
          len = ntohl(*(unsigned long *)(id+1));
          break;
        default:
	  len = -1;
        }
     } else if (code & MP_PLUS) {
       len++;
     }
     return len;
  }
  return -1;
}

// Internal function for working out the size of map and array.
int nmpsizeof(unsigned char *id, int n, int s) {
  unsigned char *p = id + s;
  while (--n) p += mpsizeof(p);
  return p - id + 1;
}

// The length of a structure in terms of the space it takes up in msgpack format.
int mpsizeof(unsigned char *id) {
  int len;
  if (*id <= 0x7f || *id >= 0xe0 || (*id >= 0xc0 && *id <= 0xc3)) {
     return 1;
  } else if (*id <= 0x8f) { /* map */
     len = (*id & 0x0f) << 1;
     return nmpsizeof(id, len, 1);
  } else if (*id <= 0x9f) { /* array */
     len = (*id & 0x0f);
     return nmpsizeof(id, len, 1);
  } else if (*id <= 0xbf) { /* string */
     return 1 + (*id & 0x1f);
  } else if (*id <= 0xc6) {
     return (1 << (*id - 0xc4)) + 1;
  } else if (*id <= 0xc9) {
     return (1 << (*id - 0xc7)) + 1; // WRONG! Add the value at id+1.
  } else if (*id <= 0xcb) {
     return (1 << (*id - 0xc8)) + 1;
  } else if (*id <= 0xcf) {
     return (1 << (*id - 0xcc)) + 1;
  } else if (*id <= 0xd3) {
     return (1 << (*id - 0xd0)) + 1;
  } else if (*id <= 0xd8) {
     return (1 << (*id - 0xd4)) + 2;
  } else if (*id == 0xd9) {
     return *(id + 1) + 2;
  } else if (*id == 0xda) {
     return ntohs(*(unsigned short *)(id + 1)) + 3;
  } else if (*id == 0xdb) {
     return ntohl(*(unsigned long *)(id + 1)) + 5;
  } else if (*id == 0xdc) {
     len = ntohs(*(unsigned short *)(id + 1));
     return nmpsizeof(id, len, 3);
  } else if (*id == 0xdd) {
     len = ntohl(*(unsigned long *)(id + 1));
     return nmpsizeof(id, len, 5);
  } else if (*id == 0xde) {
     len = ntohs(*(unsigned short *)(id + 1)) << 1;
     return nmpsizeof(id, len, 3);
  } else { /* id == 0xdf */
     len = ntohl(*(unsigned long *)(id + 1)) << 1;
     return nmpsizeof(id, len, 5);
  }
  printf("Well, that was unexpected: %02x wasn't accounted for.\n", *id);
  exit(1);
}

#ifdef TEST
#include "test.h"
void testsuite(void) {

  describe("arithmetic(2+2) ", "2+2");
  expect(int_to_eql, 2+2,4);

  describe("mptype(minposfixint) ", "\x00");
  expect(int_to_eql, mptype(test_data), MP_UINT);

  describe("mptype(maxposfixint) ", "\x7f");
  expect(int_to_eql, mptype(test_data), MP_UINT);

  describe("mptype(maxnegfixint) ", "\xe0");
  expect(int_to_eql, mptype(test_data), MP_INT);

  describe("mptype(minnegfixint) ", "\xff");
  expect(int_to_eql, mptype(test_data), MP_INT);

  describe("mptype(map) ", "\x81\x01\x01");
  expect(int_to_eql, mptype(test_data), MP_MAP);

  describe("mptype(array) ", "\x91\x01");
  expect(int_to_eql, mptype(test_data), MP_ARRAY);

  describe("mptype(string) ", "\xa5Hello");
  expect(int_to_eql, mptype(test_data), MP_STRING);

  describe("mptype(uint8) ", "\xcc\0");
  expect(int_to_eql, mptype(test_data), MP_UINT);

  describe("mptype(uint16) ", "\xcd\0\0");
  expect(int_to_eql, mptype(test_data), MP_UINT);

  describe("mptype(uint32) ", "\xce\0\0\0\0");
  expect(int_to_eql, mptype(test_data), MP_UINT);

  describe("mptype(uint64) ", "\xcf\0\0\0\0\0\0\0\0");
  expect(int_to_eql, mptype(test_data), MP_UINT);

  describe("mptype(float32) ", "\xca\0\0\0\0");
  expect(int_to_eql, mptype(test_data), MP_FLOAT);

  describe("mptype(float64) ", "\xcb\0\0\0\0\0\0\0\0");
  expect(int_to_eql, mptype(test_data), MP_FLOAT);

  describe("mptype(str8) ", "\xd9\x05Hello");
  expect(int_to_eql, mptype(test_data), MP_STRING);

  describe("mptype(str16) ", "\xda\0\x05Hello");
  expect(int_to_eql, mptype(test_data), MP_STRING);

  describe("mptype(str32) ", "\xdb\0\0\0\x05Hello");
  expect(int_to_eql, mptype(test_data), MP_STRING);

  describe("mptype(array16) ", "\xdc\0\x01\x01");
  expect(int_to_eql, mptype(test_data), MP_ARRAY);

  describe("mptype(map32) ", "\xdf\0\0\0\x01\xa5Hello\x01");
  expect(int_to_eql, mptype(test_data), MP_MAP);

  describe("mpsizeof(minposfixint) ", "\x00");
  expect(int_to_eql, mpsizeof(test_data), 1);

  describe("mpsizeof(maxposfixint) ", "\x7f");
  expect(int_to_eql, mpsizeof(test_data), 1);

  describe("mpsizeof(maxnegfixint) ", "\xe0");
  expect(int_to_eql, mpsizeof(test_data), 1);

  describe("mpsizeof(minnegfixint) ", "\xff");
  expect(int_to_eql, mpsizeof(test_data), 1);

  describe("mpsizeof(map) ", "\x81\x01\x01");
  expect(int_to_eql, mpsizeof(test_data), 3);

  describe("mpsizeof(array) ", "\x91\x01");
  expect(int_to_eql, mpsizeof(test_data), 2);

  describe("mpsizeof(string) ", "\xa5Hello");
  expect(int_to_eql, mpsizeof(test_data), 6);

  describe("mpsizeof(uint8) ", "\xcc\0");
  expect(int_to_eql, mpsizeof(test_data), 2);

  describe("mpsizeof(uint16) ", "\xcd\0\0");
  expect(int_to_eql, mpsizeof(test_data), 3);

  describe("mpsizeof(uint32) ", "\xce\0\0\0\0");
  expect(int_to_eql, mpsizeof(test_data), 5);

  describe("mpsizeof(uint64) ", "\xcf\0\0\0\0\0\0\0\0");
  expect(int_to_eql, mpsizeof(test_data), 9);

  describe("mpsizeof(float32) ", "\xca\0\0\0\0");
  expect(int_to_eql, mpsizeof(test_data), 5);

  describe("mpsizeof(float64) ", "\xcb\0\0\0\0\0\0\0\0");
  expect(int_to_eql, mpsizeof(test_data), 9);

  describe("mpsizeof(str8) ", "\xd9\x05Hello");
  expect(int_to_eql, mpsizeof(test_data), 7);

  describe("mpsizeof(str16) ", "\xda\0\x05Hello");
  expect(int_to_eql, mpsizeof(test_data), 8);

  describe("mpsizeof(str32) ", "\xdb\0\0\0\x05Hello");
  expect(int_to_eql, mpsizeof(test_data), 10);

  describe("mpsizeof(array16) ", "\xdc\0\x01\x01");
  expect(int_to_eql, mpsizeof(test_data), 4);

  describe("mpsizeof(map32) ", "\xdf\0\0\0\x01\xa5Hello\x01");
  expect(int_to_eql, mpsizeof(test_data), 12);

  describe("mplen(minposfixint) ", "\x00");
  expect(int_to_eql, mplen(test_data), 1);

  describe("mplen(maxposfixint) ", "\x7f");
  expect(int_to_eql, mplen(test_data), 1);

  describe("mplen(maxnegfixint) ", "\xe0");
  expect(int_to_eql, mplen(test_data), 1);

  describe("mplen(minnegfixint) ", "\xff");
  expect(int_to_eql, mplen(test_data), 1);

  describe("mplen(map) ", "\x81\x01\x01");
  expect(int_to_eql, mplen(test_data), 1);

  describe("mplen(array) ", "\x91\x01");
  expect(int_to_eql, mplen(test_data), 1);

  describe("mplen(string) ", "\xa5Hello");
  expect(int_to_eql, mplen(test_data), 5);

  describe("mplen(uint8) ", "\xcc\0");
  expect(int_to_eql, mplen(test_data), 1);

  describe("mplen(uint16) ", "\xcd\0\0");
  expect(int_to_eql, mplen(test_data), 2);

  describe("mplen(uint32) ", "\xce\0\0\0\0");
  expect(int_to_eql, mplen(test_data), 4);

  describe("mplen(uint64) ", "\xcf\0\0\0\0\0\0\0\0");
  expect(int_to_eql, mplen(test_data), 8);

  describe("mplen(float32) ", "\xca\0\0\0\0");
  expect(int_to_eql, mplen(test_data), 4);

  describe("mplen(float64) ", "\xcb\0\0\0\0\0\0\0\0");
  expect(int_to_eql, mplen(test_data), 8);

  describe("mplen(str8) ", "\xd9\x05Hello");
  expect(int_to_eql, mplen(test_data), 5);

  describe("mplen(str16) ", "\xda\0\x05Hello");
  expect(int_to_eql, mplen(test_data), 5);

  describe("mplen(str32) ", "\xdb\0\0\0\x05Hello");
  expect(int_to_eql, mplen(test_data), 5);

  describe("mplen(array16) ", "\xdc\0\x01\x01");
  expect(int_to_eql, mplen(test_data), 1);

  describe("mplen(map32) ", "\xdf\0\0\0\x01\xa5Hello\x01");
  expect(int_to_eql, mplen(test_data), 1);

}
#endif
