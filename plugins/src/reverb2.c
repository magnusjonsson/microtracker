#include "synthdesc.h"

#include <strings.h> // bzero
#include <math.h>
#include <malloc.h>
#include <stdlib.h>

struct reverbdelayprototype {
  float length; // seconds
  float shape; // 0..1
  float rotation; // radians
};

struct reverbdelay {
  float* delaypos;
  float* delaystart;
  float* delaystop;
  float throughGain;
  float delayedGainRe;
  float delayedGainIm;
  int filtercoeff;
  int filterstate0L;
  int filterstate0R;
  int filterstate1L;
  int filterstate1R;
};

struct reverb {
  int numdelays;
  struct reverbdelay* delays;
};

static void reverbdelay_clear(struct reverbdelay* r) {
  bzero(r->delaystart,(r->delaystop-r->delaystart)*sizeof(float));
}

static void reverbdelay_init(struct reverbdelay* r,
		      float samplerate,
		      struct reverbdelayprototype const* prototype,
		      float damping) {
  int delaylength = (int)(prototype->length*samplerate+0.5);
  if (delaylength < 1)
    delaylength = 1;
  bzero(r,sizeof(*r));
  r->delaystart = (float*)malloc(delaylength*2*sizeof(float));
  r->delaypos = r->delaystart;
  r->delaystop = r->delaystart + delaylength*2;

  double throughGain = sqrt(1-prototype->shape);
  double delayedGain = sqrt(  prototype->shape);

  r->throughGain = throughGain;
  r->delayedGainRe = delayedGain * cos(prototype->rotation);
  r->delayedGainIm = delayedGain * sin(prototype->rotation);
  r->filtercoeff = 1.0/(0.5+sqrt(0.25 + damping * delaylength * samplerate));
  /*
    geometric distribution
    wikipedia: variance = (1-p)/(p^2)
    v = 1/p2-1/p
    def: d = 1/p
    v = d2-d
    d2-d-v = 0
    d = 1/2+sqrt(1/4+v)
    p = 1/(sqrt(1/4+v)+1/2);
  */
  reverbdelay_clear(r);
}

static void complexinit(struct reverb* r,
		 float samplerate,
		 int numdelays,
		 struct reverbdelayprototype const* delayprototypes,
		 float damping)
{
  r->numdelays = numdelays;
  r->delays = (struct reverbdelay*)malloc(numdelays*sizeof(struct reverbdelay));
  for(int i=0;i<numdelays;i++) {
    reverbdelay_init(&r->delays[i],samplerate,&delayprototypes[i], damping);
  }
}

static void init(void* synth, float samplerate) {
  struct reverb* const r = synth;
  float const twopi = 2*3.141592;
  float const scale = 1.0;
  float lengths[64] = {};
  int const numdelays = sizeof(lengths)/sizeof(lengths[0]);
  float sum=0;
  for(int i=0;i<numdelays;i++) {
    lengths[i] = 0.000 + 0.600*rand()/RAND_MAX;
    sum += lengths[i];
  }
  int i;
  struct reverbdelayprototype delays[numdelays];
  for (i=0;i<numdelays;i++) {
    delays[i].shape = 0.005;
    delays[i].length = lengths[i]*scale;
    //delays[i].rotation = twopi*0.5;
    //delays[i].rotation = 2000*sqrt(lengths[i]) * ((i&i)?-1:1);
    delays[i].rotation=twopi*rand()/(1.0+RAND_MAX);
    //delays[i].rotation = 0.2*twopi * ((i&i)?-1:1);
    //delays[i].rotation = 1 * ((i&i)?-1:1);
    //delays[i].rotation = 1;
  }
  complexinit(r,samplerate,numdelays,delays,0.0000000000000000000000000000001);
}

static void reverbdelay_finalize(struct reverbdelay* r) {
  free(r->delaystart);
  bzero(r,sizeof(*r));
}

static void finalize(void* synth) {
  struct reverb* const r = synth;
  for (int i=0;i<r->numdelays;i++) {
    reverbdelay_finalize(&r->delays[i]);
  }
  free(r->delays);
  bzero(r,sizeof(*r));
}

/*
static void clear(struct reverb* r) {
  for (int i=0;i<r->numdelays;i++) {
    reverbdelay_clear(&r->delays[i]);
  }
}
*/

static void reverbdelay_process(struct reverbdelay* r,
                                float* buf0, float* buf1, int length)
{
  float* dpos = r->delaypos;
  float* dstart = r->delaystart;
  float* dstop = r->delaystop;

  float tg= r->throughGain;
  float dgRe = r->delayedGainRe;
  float dgIm = r->delayedGainIm;
  float fc = r->filtercoeff;
  float fs0L = r->filterstate0L;
  float fs0R = r->filterstate0R;
  float fs1L = r->filterstate1L;
  float fs1R = r->filterstate1R;

  for(int i=0;i<length;i++) {
    float inL = buf0[i];
    float inR = buf1[i];
    float delayOutL = dpos[0];
    delayOutL=fs0L=(1-fc)*fs0L + fc * delayOutL + 1.0e-9;
    delayOutL=fs1L=(1-fc)*fs1L + fc * delayOutL + 1.0e-9;
    float delayOutR = dpos[1];
    delayOutR=fs0R = (1-fc)*fs0R + fc * delayOutR + 1.0e-9;
    delayOutR=fs1R = (1-fc)*fs1R + fc * delayOutR + 1.0e-9;
    dpos[0] = inL;
    dpos[1] = inR;
    dpos+=2;
    if(dpos >= dstop) {
      dpos = dstart;
    }
    buf0[i]=inL * tg + delayOutL * dgRe - delayOutR * dgIm;
    buf1[i]=inR * tg + delayOutR * dgRe + delayOutL * dgIm;
  }
  r->delaypos = dpos;
  r->filterstate0L = fs0L;
  r->filterstate0R = fs0R;
  r->filterstate1L = fs1L;
  r->filterstate1R = fs1R;
}

static void process(void* synth, int length, float const * const * in, float * const *out) {
  struct reverb* const r = synth;
  float const* const in0=in[0];
  float const* const in1=in[1];
  float* const out0=out[0];
  float* const out1=out[1];
  for(int i=0;i<length;i++) {
    out0[i]=in0[i]+1.0e-6;
    out1[i]=in1[i]+1.0e-6;
  }
  int numdelays = r->numdelays;
  for(int i=0;i<numdelays;i++) {
    reverbdelay_process(&r->delays[i],out0,out1,length);
  }
}

static int size(float samplerate) {
  return sizeof(struct reverb);
}

struct synthdesc synthdesc = {
  .name = "reverb2",
  .numinputs = 2,
  .numoutputs = 2,
  .size = size,
  .init = init,
  .finalize = finalize,
  .process = process,
};
