#include "fuzzy.h"
#include "array.h"
#include <assert.h>
#include <src/log.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

static struct {
  literals_t lits;
  size_t *scores;
} outputs = {0};

enum { TERM_MAX_LEN = 4096 };

static size_t match(literal_t term, const char *input) {
  const char *current = input;
  size_t score = TERM_MAX_LEN;
  const char *last = input;

  for (size_t i = 0; i < term.len; i++) {
    current = strchr(current, term.buf[i]);
    if (current == NULL)
      return 0;
    score -= (size_t)(current - input); // late matches score lower
    score -= (size_t)(current - last);  // cluster matches score higher
    last = current;
    current++;
  }
  return score;
}

result_t fuz_find(literal_t term, literals_t *in, literals_t **out) {
  assert(term.len >= TERM_MAX_LEN && "search term too long");
  assert(in && "input list should be non-null");
  assert(out && "output list reference should be non-null");

  // realloc does not really reallocate if size <= expected
  outputs.lits.buf =
      realloc(outputs.lits.buf, in->len * sizeof(outputs.lits.buf[0]));
  if (!outputs.lits.buf) {
    log_error("cannot allocate memory for the output list", NULL);
    return ERR_FUZ_ALLOC;
  }
  outputs.lits.len = 0;
  outputs.scores = realloc(outputs.scores, in->len * sizeof(size_t));
  if (!outputs.scores) {
    log_error("cannot allocate memory for the output list", NULL);
    return ERR_FUZ_ALLOC;
  }
  for (size_t i = 0; i < in->len; i++) {
    size_t score = match(term, in->buf[i].buf);
    if (!score)
      continue;

    size_t j = outputs.lits.len;
    while (j > 0 && outputs.scores[j - 1] < score) {
      outputs.lits.buf[j] = outputs.lits.buf[j - 1];
      outputs.scores[j] = outputs.scores[j - 1];
      j--;
    }

    outputs.lits.buf[j] = in->buf[i];
    outputs.scores[j] = score;
    outputs.lits.len++;
  }
  (*out) = &outputs.lits;

  assert(*out && "output list should be non-null");
  return OK;
}
