#include "fuzzy.h"
#include "array.h"
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

static fuz_output_t outputs = {0};

static bool match(literal_t term, const char *input) {
  const char *current = input;
  for (size_t i = 0; i < term.len; i++) {
    current = strchr(current, term.buf[i]);
    if (current == NULL)
      return false;
  }
  return true;
}

result_t fuz_find(literal_t term, literals_t *in, fuz_output_t **out) {
  memset(outputs.buf, 0, sizeof(outputs.buf));
  outputs.len = 0;
  for (size_t i = 0; i < in->len && outputs.len < ArraySize(outputs.buf); i++) {
    if (match(term, in->buf[i].buf)) {
      outputs.buf[outputs.len] = in->buf[i];
      outputs.len++;
    }
  }
  (*out) = &outputs;
  return OK;
}
