#include "synthdesc.h"

static void process(void* self, int length, float const* const * in, float * const *out) {
}

struct synthdesc synthdesc = {
  .name = "drop",
  .numinputs = 1,
  .numoutputs = 0,
  .process = process,
};
