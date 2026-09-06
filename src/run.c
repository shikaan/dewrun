#include "run.h"
#include "log.h"
#include <dirent.h>
#include <errno.h>
#include <src/result.h>
#include <stdlib.h>
#include <string.h>

static List(literals_t) results;

static result_t init(void) {
  results.slice.len = 0;
  results.cap = 4;
  results.slice.buf = calloc(results.cap, sizeof(literal_t));
  if (!results.slice.buf) {
    log_error("cannot allocate memory for list entries", NULL);
    return ERR_RUN_LIST;
  }
  return OK;
}

static result_t push(char *item) {
  if (results.slice.len >= results.cap) {
    results.cap *= 2;
    results.slice.buf =
        realloc(results.slice.buf, results.cap * sizeof(literal_t));
    if (!results.slice.buf) {
      log_error("cannot allocate memory for list entries", NULL);
      return ERR_RUN_LIST;
    }
  }

  // TODO: dedupe

  results.slice.buf[results.slice.len].buf = strdup(item);
  if (!results.slice.buf[results.slice.len].buf) {
    log_error("cannot allocate memory for list entries", NULL);
    return ERR_RUN_LIST;
  }
  results.slice.buf[results.slice.len].len = strlen(item);
  results.slice.len++;
  return OK;
}

result_t run_list(literals_t **list) {
  result_t result = OK;
  const char *path = getenv("PATH");

  if (!path) {
    log_error("cannot read PATH env variable", NULL);
    return ERR_RUN_LIST;
  }

  result = init();
  if (result != OK)
    return result;

  while (path) {
    char *separator = strchr(path, ':');

    if (separator) {
      *separator = 0;
    }

    // TODO: dedupe path

    log_debug("listing files in %s", path);
    DIR *dir = opendir(path);
    if (!dir) {
      log_error("cannot open path %s: %s", path, strerror(errno));
      return ERR_RUN_LIST;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
      if (entry->d_name[0] != '.') {
        result = push(entry->d_name);
        if (result != OK)
          return result;
      }
    }
    if (!separator)
      break;
    path = separator + 1;
  }

  *list = &results.slice;
  return result;
}
