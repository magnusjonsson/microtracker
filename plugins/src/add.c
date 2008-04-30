#include "synthdesc.h"

static void process(void* self, int length, float const* const * in, float* const *out) {
  float const* const in0=in[0];
  float const* const in1=in[1];
  float* const out0=out[0];
  for(int i=0;i<length;i++) {
    out0[i]=in0[i]+in1[i];
  }
}

struct synthdesc synthdesc = {
  .name = "add",
  .numinputs = 2,
  .numoutputs = 1,
  .process = process,
};
