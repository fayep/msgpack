#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>

/* Less than or equal #1, call #2
[ 0x7f, posfixint ], // 0x7f value embedded
[ 0x8f, fixmap ], // 0x0f value embedded
[ 0x9f, fixarray ], // 0x0f value embedded
[ 0xbf, fixstr ], // 0x1f value embedded
[ nilvalue, error
  false, true,
  bin8, bin16, bin32,
  ext8, ext16, ext32,
  float32, float64,
  uint8, uint16, uint32, uint64,
  int8, int16, int32, int64,
  fixext1, fixext2, fixext4, fixext8, fixext16,
  str8, str16, str32,
  array16, array32,
  map16, map32 ];
[ 0xff, negfixint ]
*/

// Message Pack Types
typedef enum {
  MP_BOOL,
  MP_UINT,
  MP_INT,
  MP_BIN,
  MP_STRING,
  MP_FLOAT,
  MP_EXT,
  MP_ARRAY,
  MP_MAP,
  MP_NIL,
  MP_UNUSED
} mp_type;

// The condensed 0xc0->0xdf type mapping
const mp_type mp_typemap[] = {
  MP_NIL, MP_UNUSED,
  MP_BOOL, MP_BOOL,
  MP_BIN, MP_BIN, MP_BIN,
  MP_EXT, MP_EXT, MP_EXT,
  MP_FLOAT, MP_FLOAT,
  MP_UINT, MP_UINT, MP_UINT, MP_UINT,
  MP_INT, MP_INT, MP_INT, MP_INT,
  MP_EXT, MP_EXT, MP_EXT, MP_EXT, MP_EXT,
  MP_STRING, MP_STRING, MP_STRING,
  MP_ARRAY, MP_ARRAY,
  MP_MAP, MP_MAP
};

// mask for a reference indicator
const unsigned char MP_REF = 0x80;
// mask for if the reference counts objects
const unsigned char MP_OBJ = 0x20;
// whether we should increment by 1 if not objects
// whether we should double if objects (map)
const unsigned char MP_PLUS = 0x40;
// 0x1f = bytes following the key

const unsigned char mp_lenmap[] = {
  0x40, 0x40,
  0x40, 0x40,
  0x81, 0x82, 0x84,
  0xc1, 0xc2, 0xc4,
  0x04, 0x08,
  0x01, 0x02, 0x04, 0x08,
  0x01, 0x02, 0x04, 0x08,
  0x02, 0x03, 0x05, 0x09, 0x11,
  0x81, 0x82, 0x84,
  0xa2, 0xa4,
  0xe2, 0xe4
};

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
  if (*id <= 0x7f || *id >= 0xe0) {
     return 1;
  } else if (*id <= 0x9f) {
     return *id & 0x0f;
  } else if (*id <= 0xbf) {
     return *id & 0x1f;
  } else {
     unsigned char code = mp_lenmap[*id - 0xc0];
     int len = code & 0x1f;
     if (code & MP_REF) {
        switch(len) {
        case 1:
          len = *(unsigned char *)(id+1);
        case 2:
          len = ntohs(*(unsigned short *)(id+1));
        case 4:
          len = ntohl(*(unsigned long *)(id+1));
        }
     } else if (code & MP_PLUS) {
       len++;
     }
     return len;
  }
  return -1;
}


// Forward declaration of mpsizeof
int mpsizeof(unsigned char *id);

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

/* BLEH
// The length of a datafield in terms of its content
int mplen(unsigned char *id) {
  int len;
  if (*id <= 0x7f || *id >= 0xe0 || (*id >= 0xc0 && *id <= 0xc3)) {
     return 1;
  } else if (*id <= 0x8f) {
     len = (*id & 0x0f) << 1;
     return nmpsizeof(id, len, 1);
  } else if (*id <= 0x9f) {
     len = (*id & 0x0f);
     return nmpsizeof(id, len, 1);
  } else if (*id <= 0xbf) {
     return 1 + (*id & 0x1f);
  } else if (*id <= 0xc6) {
     return (1 << (*id - 0xc4)) + 1;
  } else if (*id <= 0xc9) {
     return (1 << (*id - 0xc7)) + 1;
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
  } else {
     len = ntohl(*(unsigned long *)(id + 1)) << 1;
     return nmpsizeof(id, len, 5);
  }
  printf("Well, that was unexpected: %02x wasn't accounted for.\n", *id);
  exit(1);
}
*/

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

}
#endif
