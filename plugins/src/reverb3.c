#include "synthdesc.h"
#include "shared/bandpass.h"
#include <strings.h> // bzero
#include <math.h>
#include <malloc.h>
#include <stdlib.h>

#define MAX_HEADS 16

const double head_positions[MAX_HEADS] = {
  0.833473,
  0.836124,
  0.735470,
  0.973049,
  0.785700,
  0.594285,
  0.110890,
  0.135423,
  0.984112,
  0.348193,
  0.694258,
  0.439388,
  0.896053,
  0.301309,
  0.828817,
  0.144724,
};

struct reverbhead {
  struct bandpasscoeffs bpc;
  struct bandpassstate bps;
  double* pointer;
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

  int length = 0.5 + 0.5 * 2 * samplerate;
  r->bufferstart = (double*)malloc(length*sizeof(double));
  r->bufferstop = r->bufferstart + length;
  bzero(r->bufferstart,length*sizeof(double));

  r->ingain = 1.0 * sqrt (length / samplerate) / MAX_HEADS;

  r->numheads = MAX_HEADS;
  for(int i=0;i<r->numheads;i++) {
  retry:
    r->heads[i].pointer = r->bufferstart + (int)(length * head_positions[i]);
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
    double gain = pow(0.001,time/1.5);
    double q = sqrt(time / 32);
    //double q = time / 8;
    //double q = time*time * 16;
    //double q = 0;

    bandpassstate_init(&r->heads[i].bps);
    bandpasscoeffs_from_omega_and_q(&r->heads[i].bpc, omega, q);
    //bandpasscoeffs_identity(&r->heads[i].bpc);

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

    double input0 = in0[i];
    double input1 = in1[i];

    double output0 = 1.0e-6;
    double output1 = 1.0e-6;

    double sign = 1.0;

    for(struct reverbhead* h0=headsstart; h0+1<headsstop; h0+=2) {
      struct reverbhead* h1 = h0 + 1;
  
      double* p0 = h0->pointer;
      double* p1 = h1->pointer;
      
      double x0 = *p0;
      double x1 = *p1;

      output0 += x0;
      output1 += x1;

      double sum = x0 + x1 * sign;
      double dif = x1 - x0 * sign;

      sign = -sign;

      x0 = sum * sqrt(0.5);
      x1 = dif * sqrt(0.5);

      x0 = bandpass_tick(&h0->bps,&h0->bpc,x0);
      x1 = bandpass_tick(&h1->bps,&h1->bpc,x1);

      x0 += input0 * r->ingain;
      x1 += input1 * r->ingain;
      
      *p0 = x0;
      *p1 = x1;

      p0++; if (p0 >= r->bufferstop) p0 = r->bufferstart;
      p1++; if (p1 >= r->bufferstop) p1 = r->bufferstart;

      h0->pointer = p0;
      h1->pointer = p1;
    }
    out0[i] = input0 + output0;
    out1[i] = input1 + output1;
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
