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
          len = ntohs(*(unsigned short *)(id + 1));
          break;
        case 4:
          len = ntohl(*(unsigned long *)(id + 1));
          break;
        default:
          // TODO: add error code
	  return -1;
        }
     } else if (code & MP_PLUS) {
       len++;
     }
     return len;
  }
  // TODO: add error code
  return -1;
}

// Internal function for working out the size of map and array.
int nmpsizeof(unsigned char *id, int n, int s) {
  unsigned char *p = id + s;
  while (--n) p += mpsizeof(p);
  return p - id + 1;
}

// returns the size of this field
int mpsizeof(unsigned char *id) {
  int len;
  if (*id <= 0x7f || *id >= 0xe0) {
     return 1;
  } else if (*id <= 0x8f) {
     return nmpsizeof(id, (*id & 0x0f) << 1, 1);
  } else if (*id <= 0x9f) {
     return nmpsizeof(id, *id & 0x0f, 1);
  } else if (*id <= 0xbf) {
     return (*id & 0x1f) + 1;
  } else {
     unsigned char code = mp_lenmap[*id - 0xc0];
     len = code & 0x1f;
     if (code & MP_REF) {
        int rlen;
        switch(len) {
        case 1:
          rlen = *(id+1);
          break;
        case 2:
          rlen = ntohs(*(unsigned short *)(id+1));
          break;
        case 4:
          rlen = ntohl(*(unsigned long *)(id+1));
          break;
        default:
	  return -1;
        }
        if (code & MP_OBJ) {
          if (code & MP_PLUS) rlen <<= 1;
          return nmpsizeof(id, rlen, len + 1);
        } else {
          len += rlen;
        }
     } else if (code & MP_PLUS) {
       len++;
     }
     return len + 1;
  }
  return -1;
}

#ifdef TEST
#include "test.h"
void testsuite(void) {

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
