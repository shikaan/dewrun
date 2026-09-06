#include "run.h"
#include "hashset.h"
#include "log.h"
#include <dirent.h>
#include <errno.h>
#include <src/array.h>
#include <src/result.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static literals_t results;

static int cmp(const void *a, const void *b) {
  const string_t *s = (const string_t *)a;
  const string_t *t = (const string_t *)b;

  if (s->len == 0 || t->len == 0)
    return (s->len == 0) - (t->len == 0);
  
  return strcmp(s->buf, t->buf);
}

result_t run_list(literals_t **list) {
  result_t result = OK;
  char *path = getenv("PATH");

  if (!path) {
    log_error("cannot read PATH env variable", NULL);
    return ERR_RUN_LIST;
  }

  set_t items = {0};
  set_t paths = {0};

  result = set_init(&items, 32);
  if (result != OK)
    goto done;

  result = set_init(&paths, 8);
  if (result != OK)
    goto done;

  while (path) {
    char *separator = strchr(path, ':');

    if (separator) {
      *separator = 0;
    }

    string_t path_key = {path, strlen(path)};

    if (set_has(&paths, path_key))
      continue;
    set_add(&paths, path_key);

    log_debug("listing files in %s", path);
    DIR *dir = opendir(path);
    if (!dir) {
      log_error("cannot open path %s: %s", path, strerror(errno));
      return ERR_RUN_LIST;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
      if (entry->d_name[0] != '.') {
        string_t entry_key = {strdup(entry->d_name), strlen(entry->d_name)};
        if (!set_has(&items, entry_key)) {
          result = set_add(&items, entry_key);
          if (result != OK)
            goto done;
        }
      }
    }
    if (!separator)
      break;
    path = separator + 1;
  }

  qsort(items.buf, items.cap, sizeof(string_t), &cmp);
  results.buf = (literal_t *)items.buf;
  results.len = items.len;

  *list = &results;
done:
  set_deinit(&paths);
  return result;
}
