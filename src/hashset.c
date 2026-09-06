#include "hashset.h"
#include "utils.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static inline size_t hash(string_t key, size_t len) {
  uint64_t hash = 14695981039346656037U;
  const uint64_t prime = 1099511628211U;

  for (size_t i = 0; i < key.len; i++) {
    hash ^= (uint64_t)(unsigned char)key.buf[i];
    hash *= prime;
  }

  return hash % len;
}

static inline result_t find(const set_t *self, string_t key, size_t *result) {
  assert(self && "set cannot be null");
  const size_t index = hash(key, self->cap);

  for (size_t i = 0; i < self->cap; i++) {
    size_t probed_idx = (index + i) % self->cap;
    const string_t probed_key = self->buf[probed_idx];

    if (probed_key.len == 0) {
      return ERR_SET_NOT_FOUND;
    }

    if (streql(probed_key, key)) {
      *result = probed_idx;
      return OK;
    }
  }

  return ERR_SET_NOT_FOUND;
}

static result_t grow(set_t *self) {
  const size_t new_cap = self->cap * 2;
  string_t *new_buf = calloc(new_cap, sizeof(string_t));
  if (!new_buf) {
    return ERR_SET;
  }

  for (size_t i = 0; i < self->cap; i++) {
    const string_t key = self->buf[i];
    if (key.len == 0)
      continue;

    size_t index = hash(key, new_cap);
    while (new_buf[index].len > 0) {
      index = (index + 1) % new_cap;
    }

    new_buf[index] = key;
  }

  free(self->buf);
  self->buf = new_buf;
  self->cap = new_cap;
  return OK;
}

result_t set_init(set_t *self, size_t size) {
  assert(size != 0 && "size cannot be null");
  assert(self && "set cannot be null");

  self->buf = calloc(size, sizeof(string_t));
  if (!self->buf) {
    return ERR_SET;
  }

  self->cap = size;
  self->len = 0;
  return OK;
}

result_t set_add(set_t *self, string_t key) {
  assert(self && "set cannot be null");

  size_t index = hash(key, self->cap);
  string_t old_key = self->buf[index];

  if (old_key.len > 0 && streql(key, old_key))
    return OK;

  if (old_key.len > 0) {
    for (size_t i = 0; i < self->cap; i++) {
      size_t probed_idx = (index + i) % self->cap;
      const string_t probed_key = self->buf[probed_idx];

      if (probed_key.len == 0) {
        index = probed_idx;
        goto set;
      }

      if (streql(probed_key, key)) {
        return OK;
      }
    }
  }

set:
  if ((float)self->len >= (float)self->cap * 0.75) {
    result_t res = grow(self);
    if (res != OK)
      return res;
    return set_add(self, key);
  }

  self->buf[index].buf = key.buf;
  self->buf[index].len = key.len;
  self->len++;
  return OK;
}

bool set_has(const set_t *self, string_t key) {
  assert(self && "set cannot be null");
  size_t index;
  return find(self, key, &index) == OK;
}

void set_deinit(set_t *self) {
  if (!self)
    return;
  for (size_t i = 0; i < self->cap; i++) {
    if (self->buf[i].len > 0) {
      free(self->buf[i].buf);
      self->buf[i].len = 0;
    }
  }

  free(self->buf);
  self->buf = NULL;
  self->cap = 0;
  self->len = 0;
}
