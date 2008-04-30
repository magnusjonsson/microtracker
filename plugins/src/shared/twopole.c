#include "twopole.h"
#include <math.h>
#include <strings.h> // bzero

static void twopole_updatecoeffs(struct twopole* f) {
  f->re = cos(f->theta)*f->radius;
  f->im = sin(f->theta)*f->radius;
}

void twopole_init(struct twopole* f, float theta, float radius) {
  f->state0L=0;
  f->state0R=0;
  f->state1L=0;
  f->state1R=0;
  f->theta=theta;
  f->radius=radius;
  twopole_updatecoeffs(f);
};

void twopole_finalize(struct twopole* f) {
  bzero(f,sizeof(*f));
}

void twopole_radius(struct twopole* f, float radius) {
  f->radius=radius;
  twopole_updatecoeffs(f);
}

void twopole_theta(struct twopole* f, float theta) {
  f->theta = theta;
  twopole_updatecoeffs(f);
}

void twopole_tick(struct twopole* f, float* inoutleft, float* inoutright) {
#define READSTATE \
  double state0L=f->state0L;\
  double state0R=f->state0R;\
  double state1L=f->state1L;\
  double state1R=f->state1R;\
  double ingain=1-f->radius;\
  double re=f->re;\
  double im=f->im;
  READSTATE

#define UPDATESTATE(inleft,inright)                                   \
  double newstate0L = 1.0e-12 + state0L*re - state1L*im + inleft * ingain;\
  double newstate0R = 1.0e-12 + state0R*re - state1R*im + inright * ingain;\
  double newstate1L = 1.0e-12 + state1L*re + state0L*im;\
  double newstate1R = 1.0e-12 + state1R*re + state0R*im;\
  state0L=newstate0L;\
  state0R=newstate0R;\
  state1L=newstate1L;\
  state1R=newstate1R;
  UPDATESTATE(*inoutleft,*inoutright)

#define SAVESTATE \
  f->state0L = state0L;\
  f->state0R = state0R;\
  f->state1L = state1L;\
  f->state1R = state1R;
  SAVESTATE

  *inoutleft = (float)state0L;
  *inoutright = (float)state0R;
}

void twopole_process(struct twopole* f, int length, float const* inL, float const* inR, float* outL, float* outR) {
  READSTATE;
  for(int i=0;i<length;i++) {
    UPDATESTATE(inL[i],inR[i]);
    outL[i]=(float)state0L;
    outR[i]=(float)state0R;
  }
  SAVESTATE;
}

void twopole_processadding(struct twopole* f, int length, float gain, float const* inL, float const* inR, float* outL, float* outR) {
  READSTATE;
  for(int i=0;i<length;i++) {
    UPDATESTATE(inL[i],inR[i]);
    outL[i]+=(float)(state0L*gain);
    outR[i]+=(float)(state0R*gain);
  }
  SAVESTATE;
}

