#include "config.h"
#include "cli.h"
#include "log.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <wordexp.h>

#define len(Array) sizeof(Array) / sizeof(Array[0])

#define CONFIG_VALUE_SEPARATOR '='
#define CONFIG_NAMESPACE_SEPARATOR '.'
#define CONFIG_COMMENT '#'

#define CONFIG_NAMESPACE_FONT "font"
#define CONFIG_FONT_FAMILY "family"
#define CONFIG_FONT_SIZE "size"

#ifndef SYSCONFDIR
#define SYSCONFDIR "/etc"
#endif

static config_t config = {0};

static bool file_exists(const char *path) {
  return path && access(path, R_OK) != -1;
}

// Most strings in this config can be distinguished by the first char
static inline bool streql(const char *a, const char *b) {
  return a[0] == b[0] && strcmp(a, b) == 0;
}

static void init(void) {
  config.font.family = "sans-serif";
  config.font.size = 16;
}

char *cfg_path(void) {
  static const char *config_paths[] = {
      "$XDG_CONFIG_HOME/" NAME "/config",
      "$HOME/.config/" NAME "/config",
      SYSCONFDIR "/" NAME "/config",
  };

  wordexp_t p;
  char *path;
  for (size_t i = 0; i < len(config_paths); ++i) {
    if (wordexp(config_paths[i], &p, 0) == 0) {
      path = strdup(p.we_wordv[0]);
      wordfree(&p);
      if (file_exists(path)) {
        return path;
      }
      free(path);
    }
  }

  return NULL;
}

void cfg_read(const char *path, config_t **cfg) {
#define readstr(Prop, Value)                                                   \
  if (streql(key, Value)) {                                                    \
    (Prop) = strdup(value);                                                    \
    continue;                                                                  \
  }
#define readcol(Prop, Value)                                                   \
  if (streql(key, Value)) {                                                    \
    (Prop) = color_from_string(value, Prop);                                   \
    continue;                                                                  \
  }

  init();
  *cfg = &config;

  if (!path) {
    log_info("no configuration file, using defaults", NULL);
    return;
  }

  FILE *config_file = fopen(path, "r");
  if (!config_file) {
    log_warn("failed to load config at '%s', using defaults", path);
    return;
  }

  char *line = NULL;
  size_t line_size = 0;
  ssize_t nread;
  int line_number = 0;
  log_debug("config file:", NULL);
  while ((nread = getline(&line, &line_size, config_file)) != -1) {
    line_number++;

    if (line[nread - 1] == '\n') {
      line[--nread] = '\0';
    }

    log_debug("  %d | %s", line_number, line);
    if (!*line || line[0] == CONFIG_COMMENT) {
      continue;
    }

    char *separator = strchr(line, CONFIG_VALUE_SEPARATOR);
    if (!separator) {
      log_warn("invalid line (missing %s), skipping", CONFIG_VALUE_SEPARATOR);
      continue;
    }

    *separator = '\0';
    char *value = separator + 1;

    char *dot = strchr(line, CONFIG_NAMESPACE_SEPARATOR);
    if (!dot) {
      log_warn("invalid line (missing %s), skipping",
               CONFIG_NAMESPACE_SEPARATOR);
      continue;
    }
    *dot = '\0';
    char *key = dot + 1;
    char *namespace = line;

    if (streql(namespace, CONFIG_NAMESPACE_FONT)) {
      readstr(config.font.family, CONFIG_FONT_FAMILY);

      if (streql(key, CONFIG_FONT_SIZE)) {
        // FIXME: this feels unsafe
        config.font.size = (uint32_t)atol(value);
        continue;
      }
    }
  }

  free(line);
  fclose(config_file);
#undef readcol
#undef readstr
}

void cfg_debug(void) {
  log_debug("configuration:", NULL);
  log_debug("  font.family=%s", config.font.family);
  log_debug("  font.size=%u", config.font.size);
}
