#pragma once

#include "array.h"
#include "result.h"
#include <stdbool.h>

typedef struct {
  size_t cap;
  size_t len;
  string_t *buf;
} set_t;

// The set borrows keys: it never copies or frees them, so a key has to outlive
// the set. set_deinit only releases the table.
result_t set_init(set_t *self, size_t size);
result_t set_add(set_t *self, string_t key);
bool set_has(const set_t *self, string_t key);
void set_deinit(set_t *self);
