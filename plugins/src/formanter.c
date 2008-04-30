#include "formanter.h"
#include "twopole.h"
#include "math.h"

#define FORMANTS 3

struct formanter {
  struct twopole formant[3];
  float samplerate;
};

static double widthtoradius(double width) {
  return exp(-width); // ugh, a guess
};

static void init(void* synth, float samplerate) {
  struct formanter* const f = synth;
  f->samplerate=samplerate;
  double center[FORMANTS] = { 592, 1766, 3500 };
  double width[FORMANTS]  = { 400, 400, 400 };
  double twopi = 2*3.141592;
  double k = twopi/samplerate;
  for(int i=0;i<FORMANTS;i++) {
    twopole_init(&f->formant[i],k*center[i],widthtoradius(k*width[i]));
  }
}

static void finalize(void* synth) {
}

static void process(void* synth, int length, float const* const* in, float * const* out) {
  struct formanter* const f = synth;
  twopole_process(&f->formant[0],length,in[0],in[1],out[0],out[1]);
  for(int i=1;i<FORMANTS;i++) {
    twopole_processadding(&f->formant[i],length,1.0,in[0],in[1],out[0],out[1]);
  }
}

static int size(float samplerate) {
  return sizeof(struct formanter);
}

struct synthdesc synthdesc = {
  .name = "formanter",
  .numinputs = 2,
  .numoutputs = 2,
  .size = size,
  .init = init,
  .finalize = finalize,
  .process = process,
};
