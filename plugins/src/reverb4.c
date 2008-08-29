#include "synthdesc.h"
#include "shared/onepole.h"
#include <math.h>

struct reverb {
  int length;
  int pos;
  float phase;
  float phaseinc;
  float lpcoeff;
  float hpcoeff;
  double lpstateL;
  double lpstateR;
  double hpstateL;
  double hpstateR;
  float buffer[0];
};

static int bufferlength(float samplerate) {
  return (int)(samplerate*0.5+0.5);
}

static int size(float samplerate) {
  return sizeof(struct reverb) + sizeof(float) * bufferlength(samplerate);
}

static void init(void* synth, float samplerate) {
  struct reverb* const r = synth;
  r->length = bufferlength(samplerate);
  r->pos = 0;
  r->phase = 0;
  r->lpcoeff = onepole_coeff_for_omega(10000*2*3.141592 / samplerate);
  r->hpcoeff = onepole_coeff_for_omega(30*2*3.141592 / samplerate);
  r->lpstateL = 0;
  r->lpstateR = 0;
  r->hpstateL = 0;
  r->hpstateR = 0;
  r->phaseinc = 1 / samplerate;
}

static void finalize(void* synth) {
  //  struct reverb* const r = synth;
}

static float* index(struct reverb* r, int offset) {
  int p = r->pos - offset;
  if (p < 0)
    p += r->length;
  return &r->buffer[p];
}

static float* index2(struct reverb* r, float offset) {
  return index(r,(int)(offset*r->length));
}

static void rot(float* a, float* b) {
  if (a != b) {
    float va = *a;
    float vb = *b;
    float sqrthalf = 0.7071067811865476*0.97;
    *a = (va+vb) * sqrthalf;
    *b = (va-vb) * sqrthalf;
  }
}

static void rot2(struct reverb* r, float a, float b) {
  rot(index2(r,a), index2(r,b));
}

static void copy(struct reverb* r, float a, float b) {
  a *= r->length;
  b *= r->length;
  int wholea = a;
  float fraca = a - wholea;
  *index(r,b) = *index(r,wholea)*(1-fraca) + *index(r,wholea+1)*fraca;
}

static void varidelay(struct reverb* r, float a, float b, float x) {
  copy(r,a*(1-x)+b*x,b);
}

static const float posl[5] = { 0.0, 0.1458, 0.2592, 0.358592, 0.36 };
static const float posr[5] = { 0.5, 0.6692, 0.7432, 0.838492, 0.86 };

static void process(void* synth,int length,float const * const * in, float * const *out) {
  struct reverb* const r = synth;
  float const* const in0=in[0];
  float const* const in1=in[1];
  float* const out0=out[0];
  float* const out1=out[1];
  for(int i=0;i<length;i++) {
    float* l0 = index2(r,posl[0]);
    float* r0 = index2(r,posr[0]);

    float ol = *l0;
    float or = *r0;

    // lowpass filter
    r->lpstateL += (ol - r->lpstateL)*r->lpcoeff;
    r->lpstateR += (or - r->lpstateR)*r->lpcoeff;
    ol = r->lpstateL;
    or = r->lpstateR;

    // highpass filter
    r->hpstateL += (ol - r->hpstateL)*r->hpcoeff;
    r->hpstateR += (or - r->hpstateR)*r->hpcoeff;
    ol -= r->hpstateL;
    or -= r->hpstateR;

    // feedback + input
    *l0 = ol * 0.8 + in1[i];
    *r0 = or * 0.8 + in0[i];

    // output
    out0[i] = in0[i] + ol * 0.25;
    out1[i] = in1[i] + or * 0.25;

    // allpass filters
    float phase = r->phase;
    phase += r->phaseinc;
    if (phase > 0.5)
      phase -= 1.0;
    r->phase = phase;

    float cs = cos(phase*2*3.141592);
    float sn = sin(phase*2*3.141592);

    rot2(r,posl[0],posl[1]);
    rot2(r,posl[1],posl[2]);
    rot2(r,posl[2],posl[3]);
    varidelay(r,posl[3],posl[4],0.5+0.5*cs);
    rot2(r,posr[0],posr[1]);
    rot2(r,posr[1],posr[2]);
    rot2(r,posr[2],posr[3]);
    varidelay(r,posr[3],posr[4],0.5+0.01*sn);

    int pos = r->pos + 1;
    if (pos >= r->length)
      pos -= r->length;
    r->pos = pos;
  }
}

struct synthdesc synthdesc = {
  .name = "reverb4",
  .numinputs = 2,
  .numoutputs = 2,
  .size = size,
  .init = init,
  .finalize = finalize,
  .process = process,
};
