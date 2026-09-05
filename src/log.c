#include "log.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static log_level_t current_level = LOG_LEVEL_ERROR;
static const char *LOG_LEVEL[LOG_LEVELS] = {
    "NONE", "ERROR", "WARN", "INFO", "DEBUG",
};
static char *filter = NULL;

void log_init(log_level_t level) {
  filter = getenv("LOG_FILE");
  current_level = level;
}

void _log_put(log_level_t level, const char *file, int line, const char *fmt,
              ...) {
  if (level > current_level || (filter && !strstr(file, filter)))
    return;

  fprintf(stderr, "[%s:%d] %s - ", file, line, LOG_LEVEL[level]);

  va_list args;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  fprintf(stderr, "\n");
  va_end(args);
}
