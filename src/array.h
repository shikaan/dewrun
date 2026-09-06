#pragma once
#include <stddef.h>

#define Slice(Type)                                                            \
  struct {                                                                     \
    Type *buf;                                                                 \
    size_t len;                                                                \
  }

#define List(Slice)                                                            \
  struct {                                                                     \
    Slice slice;                                                               \
    size_t cap;                                                                \
  }

#define Buffer(Type, Len)                                                      \
  struct {                                                                     \
    Type buf[Len];                                                             \
    size_t len;                                                                \
  }

#define ArraySize(Array) sizeof(Array) / sizeof(Array[0])
#define ToSlice(Type, Array) ((Type){(Array).buf, (Array).len})

typedef Slice(const char) literal_t;
typedef Slice(char) string_t;
typedef Slice(literal_t) literals_t;
