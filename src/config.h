#pragma once

#include "color.h"
#include <stddef.h>
#include <stdint.h>

typedef struct {
  struct {
    const char *family;
    uint32_t size;
  } font;
} config_t;

char *cfg_path(void);
void cfg_read(const char *path, config_t **cfg);
void cfg_debug(void);
