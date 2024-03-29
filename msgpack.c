#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include "msgpack.h"

/* msgpack.c
   functions to work with msgpack format data.
*/

const char *mp_typenames[] = {
  "MP_FALSE",
  "MP_TRUE",
  "MP_UINT",
  "MP_INT",
  "MP_BIN",
  "MP_STRING",
  "MP_FLOAT",
  "MP_EXT",
  "MP_ARRAY",
  "MP_MAP",
  "MP_NIL",
  "MP_UNUSED"
};

// The condensed 0xc0->0xdf type mapping
const mp_type mp_typemap[] = {
  MP_NIL, MP_UNUSED,
  MP_FALSE, MP_TRUE,
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

// returns a pointer to the data
mp_object *mpobject(unsigned char *id, mp_object *object) {

  if (*id <= 0x7f) {
     object->type = MP_UINT;
     object->length = 1;
     object->next = id + 1;
     object->byteptr = id;
  } else if (*id >= 0xe0) {
     object->type = MP_INT;
     object->length = 1;
     object->next = id + 1;
     object->byteptr = id;
  } else if (*id <= 0x8f) {
     object->type = MP_MAP;
     object->length = *id & 0x0f;
     object->next = id + 1;
     object->byteptr = id + 1;
  } else if (*id <= 0x9f) {
     object->type = MP_ARRAY;
     object->length = *id & 0x0f;
     object->next = id + 1;
     object->byteptr = id + 1;
  } else if (*id <= 0xbf) {
     object->type = MP_STRING;
     object->length = *id & 0x1f;
     object->next = id + object->length + 1;
     object->byteptr = id + 1;
  } else {
     unsigned char code = mp_lenmap[*id - 0xc0];
     object->type = mp_typemap[*id - 0xc0];
     object->length = code & 0x1f;
     if (code & MP_REF) {
        object->byteptr = id + object->length + 1;
        switch(object->length) {
        case 1:
          object->length = *(id+1);
          break;
        case 2:
          object->length = ntohs(*(unsigned short *)(id + 1));
          break;
        case 4:
          object->length = ntohl(*(unsigned long *)(id + 1));
          break;
        default:
	  return NULL;
        }
        if (code & MP_OBJ) {
          object->next = object->byteptr;
        } else {
          object->next = object->byteptr + object->length;
        }
     } else if (code & MP_PLUS) {
       object->length++;
     }
  }
  return object;
}



#ifdef TEST
#include "test.h"
void testsuite(void) {

  describe("mpobject(map32) ", "\xdf\0\0\0\x01\xa5Hello\x01");
  mp_object obj, key, value;
  mpobject(test_data, &obj);
  expect(int_to_eql, obj.type, MP_MAP);
  expect(int_to_eql, obj.length, 1);
  expect(int_to_eql, obj.next - test_data, 5);

  mpobject(obj.byteptr, &key);
  expect(int_to_eql, key.type, MP_STRING);
  expect(int_to_eql, key.length, 5);
  expect(string_to_eql, "Hello", key.string, key.length);

  mpobject(key.next, &value);
  expect(int_to_eql, value.type, MP_UINT);
  expect(int_to_eql, value.length, 1);
  expect(int_to_eql, *value.byteptr, 1);

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
