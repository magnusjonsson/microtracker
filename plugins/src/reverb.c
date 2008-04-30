#include "synthdesc.h"

#include <strings.h> // bzero
#include <math.h>
#include <malloc.h>
#include <stdlib.h>

struct reverbdelayprototype {
  float length; // seconds
  float feedback; // 1
  float rotation; // radians
  float gain;
};

struct reverbdelay {
  float* delaypos;
  float* delaystart;
  float* delaystop;
  float gainIn;
  float feedbackRe;
  float feedbackIm;
  float filtercoeff;
  double filterstate0L;
  double filterstate0R;
  double filterstate1L;
  double filterstate1R;
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
  r->gainIn = (1-prototype->feedback)*prototype->gain;
  r->feedbackRe = prototype->feedback * cos(prototype->rotation);
  r->feedbackIm = prototype->feedback * sin(prototype->rotation);
  double variance = damping * delaylength / samplerate;
  variance *= samplerate*samplerate;
  r->filtercoeff = 1.0/(0.5+sqrt(0.25 + variance));
  /*
    geometric distribution
    wikipedia: variance = (1-p)/(p^2)
    v = 1/p2-1/p
    def: d = 1/p
    v = d2-d
    d2-d-v = 0
    d = 1/2 + sqrt(1/4+v)
    p = 1/(1/2 + sqrt(1/4+v));
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
  struct reverb* r = synth;
  float twopi = 2*3.141592;
  float scale = 1.0;
  float lengths[32] = {};
  int numdelays = sizeof(lengths)/sizeof(lengths[0]);
  float sum=0;
  //  numdelays=6;
  for(int i=0;i<numdelays;i++) {
    lengths[i] = 0.020 + 0.080*rand()/RAND_MAX;
    sum += lengths[i];
  }
  int i;
  struct reverbdelayprototype delays[numdelays];
  for (i=0;i<numdelays;i++) {
    delays[i].gain = sqrt(lengths[i]/0.060*4/numdelays);
    delays[i].length = lengths[i]*scale;
    delays[i].feedback = pow(0.001,delays[i].length/2.5);
    //delays[i].rotation = twopi*0.5;
    //delays[i].rotation = 2000*sqrt(lengths[i]) * ((i&i)?-1:1);
    delays[i].rotation=twopi*rand()/(1.0+RAND_MAX);
    //delays[i].rotation = 0.2*twopi * ((i&i)?-1:1);
    //delays[i].rotation = 1 * ((i&i)?-1:1);
    //delays[i].rotation = 1;
  }
  complexinit(r,samplerate,numdelays,delays,1.0e-9);
}

static void reverbdelay_finalize(struct reverbdelay* r) {
  free(r->delaystart);
  bzero(r,sizeof(*r));
}

static void finalize(void* s) {
  struct reverb* const r = s;
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
                                const float* in0, const float* in1,
                                float* out0, float* out1, int length)
{
  float* dpos = r->delaypos;
  float* dstart = r->delaystart;
  float* dstop = r->delaystop;

  float gainIn = r->gainIn;
  float fbRe = r->feedbackRe;
  float fbIm = r->feedbackIm;
  float fc = r->filtercoeff;
  double fs0L = r->filterstate0L;
  double fs0R = r->filterstate0R;
  double fs1L = r->filterstate1L;
  double fs1R = r->filterstate1R;

  for(int i=0;i<length;i++) {
    float inL = in0[i];
    float inR = in1[i];
    float delayOutL = dpos[0];
    delayOutL=fs0L=(1-fc)*fs0L + fc * delayOutL + 1.0e-9;
    delayOutL=fs1L=(1-fc)*fs1L + fc * delayOutL + 1.0e-9;
    float delayOutR = dpos[1];
    delayOutR=fs0R = (1-fc)*fs0R + fc * delayOutR + 1.0e-9;
    delayOutR=fs1R = (1-fc)*fs1R + fc * delayOutR + 1.0e-9;

    float delayOutL2 = fbRe*delayOutL-fbIm*delayOutR;
    float delayOutR2 = fbRe*delayOutR+fbIm*delayOutL;
    float delayInL = gainIn*inL + delayOutL2;
    float delayInR = gainIn*inR + delayOutR2;
    dpos[0] = delayInL;
    dpos[1] = delayInR;
    dpos+=2;
    if(dpos >= dstop) {
      dpos = dstart;
    }
    out0[i]+=delayOutL;
    out1[i]+=delayOutR;
  }
  r->delaypos = dpos;
  r->filterstate0L = fs0L;
  r->filterstate0R = fs0R;
  r->filterstate1L = fs1L;
  r->filterstate1R = fs1R;
}

static void process(void* s,int length,float const* const* in,float* const* out)
{
  struct reverb* const r = s;
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
    reverbdelay_process(&r->delays[i],in0,in1,out0,out1,length);
  }
  for(int i=0;i<length;i++) {
    out0[i]=(in0[i]+out0[i])*0.5;
    out1[i]=(in1[i]+out1[i])*0.5;
  }
}

static int size(float samplerate) {
  return sizeof(struct reverb);
}

struct synthdesc synthdesc = {
  .name = "reverb",
  .numinputs = 2,
  .numoutputs = 2,
  .size = size,
  .init = init,
  .finalize = finalize,
  .process = process,
};
