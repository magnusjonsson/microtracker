#include "synthdesc.h"

static void process(void* state, int length, float const* const* in, float*const* out) {
  float const* in0 = in[0];
  float const* in1 = in[1];
  float* out0 = out[0];
  float* out1 = out[1];
  for(int i=0;i<length;i++) {
    float s0 = in0[i];
    float s1 = in1[i];
    out0[i] = s1;
    out1[i] = s0;
  }
}

struct synthdesc synthdesc = {
  .name = "swap",
  .numinputs = 2,
  .numoutputs = 2,
  .process = process
};
