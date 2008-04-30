#include "synthdesc.h"

static void process(void* self, int length, float const* const * in, float* const *out) {
  float const* const in0=in[0];
  float const* const in1=in[1];
  float const* const in2=in[2];
  float const* const in3=in[3];
  float* const out0=out[0];
  float* const out1=out[1];
  for(int i=0;i<length;i++) {
    double o0 = in0[i]+in2[i];
    double o1 = in1[i]+in3[i];
    out0[i]=o0;
    out1[i]=o1;
  }
}

struct synthdesc synthdesc = {
  .name = "2add",
  .numinputs = 4,
  .numoutputs = 2,
  .process = process,
};
