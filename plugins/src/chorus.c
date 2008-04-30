#include "synthdesc.h"

#include <strings.h> // bzero
#include <math.h>
#include <malloc.h>
#include <stdlib.h>

#define MAX_DELAY 0.1 /* seconds */
#define MAX_DEPTH 0.1 /* seconds */

struct chorus {
  float* buffer;
  int pos;
  int len;

  double delay;
  double depth;
  double level;
  double rate;

  double phase;
  double samplerate;
};

static void init(void* synth, float samplerate) {
  struct chorus* const c = synth;
  c->len = (int)((MAX_DELAY+MAX_DEPTH)*samplerate)+2;
  c->pos = 0;
  c->buffer = calloc(c->len*2, sizeof(float));

  c->samplerate = samplerate;
  c->phase = 0;

  c->delay = 0.020;
  c->depth = 0.006;
  c->level = 0.4;
  c->rate = 0.3;
}

static void finalize(void* synth) {
  struct chorus* const c = synth;
  free(c->buffer);
  bzero(c,sizeof(struct chorus));
}

static void process(void* synth, int length, float const * const * in, float * const *out) {
  struct chorus* const c = synth;
  float const* inputL = in[0];
  float const* inputR = in[1];
  float* outputL = out[0];
  float* outputR = out[1];

  int pos = c->pos;
  int len = c->len;
  float* buf = c->buffer;
  double rate = c->rate;
  double phase = c->phase;
  double delay = c->delay;
  double depth = c->depth;
  double samplerate = c->samplerate;
  double level = c->level;
  double twopi = 2*3.141592;

  for(int i=0;i<length;i++) {
    float inL = inputL[i];
    float inR = inputR[i];
    
    buf[2*pos+0] = inL;
    buf[2*pos+1] = inR;

    double offsL = ((0.5*cos(phase*twopi)+0.5)*depth+delay)*samplerate;
    double offsR = ((0.5*sin(phase*twopi)+0.5)*depth+delay)*samplerate;

    int wholeL = (int) offsL;
    int wholeR = (int) offsR;

    double fracL = offsL-wholeL;
    double fracR = offsR-wholeR;

    phase += rate / samplerate;
    if (phase > 0.5)
      phase -= 1;

    int pos0L = pos-(int)offsL; pos0L += len & (pos0L>>31);
    int pos0R = pos-(int)offsR; pos0R += len & (pos0R>>31);
    int pos1L = pos0L-1; pos1L += len & (pos1L>>31);
    int pos1R = pos0R-1; pos1R += len & (pos1R>>31);

    double g0L = 1-fracL;
    double g0R = 1-fracR;
    double g1L = fracL;
    double g1R = fracR;

    inL += level * (buf[2*pos0L]*g0L+buf[2*pos1L]*g1L);
    inR += level * (buf[2*pos0R]*g0R+buf[2*pos1R]*g1R);

    pos += 1;
    pos &= (pos-len)>>31;
    
    outputL[i] = inL;
    outputR[i] = inR;
  }
  c->pos = pos;
  c->phase = phase;
}

static int size(float samplerate) {
  return sizeof(struct chorus);
}

struct synthdesc synthdesc = {
  .name = "chorus",
  .size = size,
  .init = init,
  .finalize = finalize,
  .process = process,
};
