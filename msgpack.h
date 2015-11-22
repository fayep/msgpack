/* msgpack.h
   functions to work with msgpack format data.
*/
#ifndef __MSGPACK_H
#define __MSGPACK_H

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

typedef struct {
  mp_type type;
  int length;
  unsigned char *next;
  union {
    unsigned char *byteptr;
    char *string;
    unsigned short *uwordptr;
    short *wordptr;
    unsigned long *ulongptr;
    long *longptr;
    unsigned long long *ullptr;
    long long *llptr;
    float *floatptr;
    double *doubleptr;
  };
} mp_object;


mp_type mptype(unsigned char *id);

// returns the size of the data for this field
int mplen(unsigned char *id);

// The length of a structure in terms of the space it takes up in msgpack format.
int mpsizeof(unsigned char *id);

// Internal function for working out the size of map and array.
int nmpsizeof(unsigned char *id, int n, int s);

#endif // defined(__MSGPACK_H)
