/* msgpack.h
   functions to work with msgpack format data.
*/
#ifndef __MSGPACK_H
#define __MSGPACK_H

// Message Pack Types
typedef enum {
  MP_FALSE,
  MP_TRUE,
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

// Message Pack Types
extern const char *mp_typenames[];

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

mp_object *mpobject(unsigned char *id, mp_object *object);

#endif // defined(__MSGPACK_H)
