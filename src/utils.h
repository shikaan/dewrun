#pragma once

#include "array.h"
#include <stdbool.h>
#include <stddef.h>

static inline bool streql(string_t a, string_t b) {
  if (a.len != b.len)
    return false;

  for (size_t i = 0; i < a.len; i++) {
    if (a.buf[i] != b.buf[i])
      return false;
  }

  return true;
}
