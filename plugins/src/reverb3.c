#include "synthdesc.h"
#include "shared/bandpass.h"
#include <strings.h> // bzero
#include <math.h>
#include <malloc.h>
#include <stdlib.h>

#define MAX_HEADS 64

struct reverbhead {
  struct bandpasscoeffs bpc;
  struct bandpassstate bps;
  double* pointer;
  struct reverbhead* partner;
};

struct reverb {
  double* bufferstart;
  double* bufferstop;
  int numheads;
  double ingain;
  struct reverbhead heads[MAX_HEADS];
};

static void init(void* synth, float samplerate) {
  struct reverb* const r = synth;
  float twopi = 2*3.141592;

  int length = 0.5 + 2.0 * 2 * samplerate;
  r->bufferstart = (double*)malloc(length*sizeof(double));
  r->bufferstop = r->bufferstart + length;
  bzero(r->bufferstart,length*sizeof(double));

  r->ingain = 0.04 * sqrt (length / samplerate);

  r->numheads = MAX_HEADS;
  for(int i=0;i<r->numheads;i++) {
  retry:
    r->heads[i].pointer = r->bufferstart + rand()%length;
    for(int j=0;j<i;j++) {
      if (r->heads[j].pointer == r->heads[i].pointer) {
        goto retry;
      }        
    }
  }

  double omega = twopi * 700.0 / samplerate;
  for(int i=0;i<r->numheads;i++) {

    int prev = 0;
    int distance = length;
    for(int j=0;j<r->numheads;j++) {
      int this_distance = r->heads[j].pointer - r->heads[i].pointer;
      if (this_distance <= 0)
        this_distance += length;
      if (this_distance < distance) {
        distance = this_distance;
        prev = j;
      }
    }

    double time = distance / samplerate;
    double gain = pow(0.001,time/3.0);
    double q = sqrt(time/16);
    //double q = time;
    //double q = 0;

    bandpassstate_init(&r->heads[i].bps);
    bandpasscoeffs_from_omega_and_q(&r->heads[i].bpc, omega, q);
    //    bandpasscoeffs_identity(&r->heads[i].bpc);

    r->heads[i].bpc.gain *= gain;
  }
}

static void finalize(void* synth) {
  struct reverb* const r = synth;
  free(r->bufferstart);
  bzero(r,sizeof(*r));
}

static void process(void* synth,int length,float const * const * in, float * const *out) {
  struct reverb* const r = synth;
  float const* const in0=in[0];
  float const* const in1=in[1];
  float* const out0=out[0];
  float* const out1=out[1];
  for(int i=0;i<length;i++) {
    struct reverbhead* headsstart = r->heads;
    struct reverbhead* headsstop  = r->heads+r->numheads;

    out0[i]=in0[i]+1.0e-6;
    out1[i]=in1[i]+1.0e-6;

    double sign = 1;

    for(struct reverbhead* h0=headsstart; h0+1<headsstop; h0+=2) {
      struct reverbhead* h1 = h0 + 1;
  
      double* p0 = h0->pointer;
      double* p1 = h1->pointer;
      
      double x0 = *p0;
      double x1 = *p1;

      out0[i] += x0 * sign;
      out1[i] += x1 * sign;
      sign = -sign;
      
      double sum = x0 + x1;
      double dif = x1 - x0;

      x0 = sum * sqrt(0.5);
      x1 = dif * sqrt(0.5);

      x0 = bandpass_tick(&h0->bps,&h0->bpc,x0);
      x1 = bandpass_tick(&h1->bps,&h1->bpc,x1);

      x0 += in0[i] * r->ingain;
      x1 += in1[i] * r->ingain;
      
      *p0 = x0;
      *p1 = x1;

      p0++; if (p0 >= r->bufferstop) p0 = r->bufferstart;
      p1++; if (p1 >= r->bufferstop) p1 = r->bufferstart;

      h0->pointer = p0;
      h1->pointer = p1;
    }
  }
}

static int size(float samplerate) {
  return sizeof(struct reverb);
}

struct synthdesc synthdesc = {
  .name = "reverb3",
  .numinputs = 2,
  .numoutputs = 2,
  .size = size,
  .init = init,
  .finalize = finalize,
  .process = process,
};
