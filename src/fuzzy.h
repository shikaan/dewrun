#pragma once

#include "array.h"
#include "result.h"
#include <stddef.h>

result_t fuz_find(literal_t term, literals_t* in, literals_t ** out);
