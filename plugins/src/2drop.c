#include "synthdesc.h"

static void process(void* self, int length, float const* const * in, float * const *out) {
}

struct synthdesc synthdesc = {
  .name = "2drop",
  .numinputs = 2,
  .numoutputs = 0,
  .process = process,
};
