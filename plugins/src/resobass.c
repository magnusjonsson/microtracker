#include "synthdesc.h"
#include "shared/moogfilter.h"
#include <math.h>

struct voice {
  struct moogfilter filter;
  double hpstate1;
  double hpstate2;
  double hpcoeff;
  double phase1;
  double phaseinc;
  double phaseinctarget;
  double samplerate;
  double gate;
  double ampenv;
  double ampenvtarget;
  double cutoffenv;
  double cutoffenvtarget;
  float freqtable[128];
};

struct synth {
  struct voice voices[128];
  double mod;
  double reso;
  float bendcoeff;
};

static void init(void* synth, float samplerate) {
  struct synth* const s = synth;
  //ms20filter_init(&s->filter);
  for (int i = 0; i < 128; i++) {
    struct voice* v = &s->voices[i];
    moogfilter_init(&v->filter);
    v->hpstate1 = 0;
    v->hpstate2 = 0;
    v->hpcoeff = 10.0*2*3.141592/samplerate;
    v->phase1 = 0;
    v->phaseinctarget = 440.0/samplerate;
    v->phaseinc = v->phaseinctarget;
    v->samplerate = samplerate;
    v->gate = 0;
    v->ampenv=0;
    v->ampenvtarget=0;
    v->cutoffenv=0;
    v->cutoffenvtarget=0;
  }
  s->mod = 0.33;
  s->reso = 0.0;
  s->bendcoeff = 1.0;
}

static void finalize(void* synth) {
}

static double ticksaw(double* phase, double phaseinc) {
  double p = *phase;
  double v1 = p*p;
  p += phaseinc;
  if (p >= 0.5)
    p -= 1.0;
  double v2 = p*p;
  *phase = p;
  return (v2-v1)*(1/phaseinc);
}

static void process(void* synth, int length, float const* const* in, float * const* out) {
  struct synth* const s = synth;
  float* outL = out[0];
  float* outR = out[1];
  
  for(int i=0;i<length;i++) {
    outL[i] = 0.0;
    outR[i] = 0.0;
  }

  for (int j=0;j<128;j++) {
    
    struct voice* v = &s->voices[j];
    
    double phase1 = v->phase1;
    double phaseinctarget = v->phaseinctarget;
    double phaseinc = v->phaseinc;
    double hpstate1 = v->hpstate1;
    double hpstate2 = v->hpstate2;
    double hpcoeff = v->hpcoeff;
    double gate = v->gate;

    if (gate < 1.0e-5 &&
	v->ampenvtarget < 1.0e-5 &&
	v->ampenv < 1.0e-5)
      continue;

    for(int i=0;i<length;i++) {
      //phaseinc = phaseinctarget;
      phaseinc += (phaseinctarget-phaseinc)*500/v->samplerate;
      double osc =
	ticksaw(&phase1,phaseinc*s->bendcoeff);
      osc -= hpstate1; hpstate1 += osc*hpcoeff;
      if (gate > 0) {
	v->cutoffenvtarget -= 2*v->cutoffenvtarget*v->cutoffenvtarget/v->samplerate;
      }
      else {
	v->cutoffenvtarget -= 16*v->cutoffenvtarget*v->cutoffenvtarget/v->samplerate;
      }
      v->cutoffenv += (v->cutoffenvtarget-v->cutoffenv) * 500 / v->samplerate;
      double cutoff = v->cutoffenv * (s->mod*12000 * 2 * 3.141592 / v->samplerate);
      osc *= 0.25;
      double sound = moogfilter_tick(&v->filter, osc, cutoff, s->reso*1.2);
      if (gate > 0) {
	v->ampenvtarget *= 1 - 0.2/v->samplerate;
      }
      else {
	v->ampenvtarget *= 1 - 4/v->samplerate;
      }
      //v->ampenv = v->ampenvtarget;
      v->ampenv += (v->ampenvtarget-v->ampenv) * 500 / v->samplerate;
      double g = v->ampenv;
      sound -= hpstate2; hpstate2 += sound*hpcoeff;
      sound = sound*g;
      outL[i] += sound;
      outR[i] += sound;
    }
    v->phase1 = phase1;
    v->phaseinc = phaseinc;
    v->hpstate1 = hpstate1;
    v->hpstate2 = hpstate2;
  }
}
static void noteon(void* synth, int key, float freq, float velocity) {
  struct synth* const s = synth;
  struct voice* v = &s->voices[key];

  v->phaseinctarget = freq/v->samplerate;
  v->ampenvtarget = velocity;
  //v->ampenvtarget = sqrt(v->ampenvtarget*v->ampenvtarget+velocity*velocity);
  //v->cutoffenvtarget = sqrt(v->cutoffenvtarget*v->cutoffenvtarget+velocity*velocity);
  //v->ampenvtarget += velocity;
  v->cutoffenvtarget = sqrt(velocity);
  v->gate = velocity;
}
static void noteoff(void* synth, int key) {
  struct synth* const s = synth;
  struct voice* v = &s->voices[key];
  v->gate = 0;
}
static void vol(void* synth, float vol) {
  struct synth* const s = synth;
  s->reso = vol*0.99;
}

static void mod(void* synth, float mod) {
  struct synth* const s = synth;
  s->mod = mod * mod;
}
static void pitchbend(void* synth, float cents) {
  struct synth* const s = synth;
  s->bendcoeff = powf(2.0,cents/1200.0);
}

static int size(float samplerate) {
  return sizeof(struct synth);
}

struct synthdesc synthdesc = {
  .name = "resobass",
  .numinputs = 0,
  .numoutputs = 2,
  .size = size,
  .init = init,
  .finalize = finalize,
  .process = process,
  .noteon = noteon,
  .noteoff = noteoff,
  .pitchbend = pitchbend,
  .mod = mod,
  .vol = vol,
};

