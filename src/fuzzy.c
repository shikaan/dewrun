#include "fuzzy.h"
#include "array.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

static literals_t outputs = {0};

static bool match(literal_t term, const char *input) {
  const char *current = input;
  for (size_t i = 0; i < term.len; i++) {
    current = strchr(current, term.buf[i]);
    if (current == NULL)
      return false;
    current++;
  }
  return true;
}

result_t fuz_find(literal_t term, literals_t *in, literals_t **out) {
  outputs.buf = realloc(outputs.buf, in->len * sizeof(outputs.buf[0]));
  outputs.len = 0;
  for (size_t i = 0; i < in->len; i++) {
    if (match(term, in->buf[i].buf)) {
      outputs.buf[outputs.len] = in->buf[i];
      outputs.len++;
    }
  }
  (*out) = &outputs;
  return OK;
}
