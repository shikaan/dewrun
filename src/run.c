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

result_t run_init(literals_t **list) {
  result_t result = OK;
  const char *path_env = getenv("PATH");

  if (!path_env) {
    log_error("cannot read PATH env variable", NULL);
    return ERR_RUN_LIST;
  }

  // touching directly the path env breaks the environment
  char *pathstr = strdup(path_env);
  set_t items = {0};
  set_t paths = {0};

  result = set_init(&items, 32);
  if (result != OK)
    goto done;

  result = set_init(&paths, 8);
  if (result != OK)
    goto done;

  char *cursor = pathstr;
  while (cursor && *cursor) {
    char *entry_path = cursor;
    char *separator = strchr(cursor, ':');

    if (separator) {
      *separator = 0;
      cursor = separator + 1;
    } else {
      cursor = NULL;
    }

    string_t path = {entry_path, strlen(entry_path)};

    if (set_has(&paths, path))
      continue;
    set_add(&paths, path);

    log_debug("listing files in %s", path.buf);
    DIR *dir = opendir(path.buf);
    if (!dir) {
      log_warn("cannot open path %s: %s; skipping", path.buf,
               strerror(errno));
      continue;
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
  }

  qsort(items.buf, items.cap, sizeof(string_t), &cmp);
  results.buf = (literal_t *)items.buf;
  results.len = items.len;

  *list = &results;
done:
  free(pathstr);
  return result;
}
