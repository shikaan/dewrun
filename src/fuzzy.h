#pragma once

#include "array.h"
#include "result.h"
#include <stddef.h>

enum { MAX_RESULTS = 5 };
typedef Buffer(literal_t, MAX_RESULTS) fuz_output_t;

result_t fuz_find(literal_t term, literals_t* in, fuz_output_t ** out);
