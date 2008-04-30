#include "synthdesc.h"
#include "shared/moogfilter.h"
#include <math.h>

struct synth {
  struct moogfilter filter;
  double hpstate1;
  double hpstate2;
  double hpcoeff;
  double phase1;
  double phase2;
  double phaseinc;
  double phaseinctarget;
  double samplerate;
  double gate;
  double ampenv;
  double ampenvtarget;
  double cutoffenv;
  double cutoffenvtarget;
  int lastkey;
  float freqtable[128];
  float bendcoeff;
  double mod;
  double reso;
};

static void init(void* synth, float samplerate) {
  struct synth* const s = synth;
  //ms20filter_init(&s->filter);
  moogfilter_init(&s->filter);
  s->phase1 = 0;
  s->phase2 = 0;
  s->hpstate1 = 0;
  s->hpstate2 = 0;
  s->hpcoeff = 10.0*2*3.141592/samplerate;
  s->samplerate = samplerate;
  s->phaseinctarget = 440.0/samplerate;
  s->phaseinc = s->phaseinctarget;
  s->gate = 0;
  s->ampenv=0;
  s->ampenvtarget=0;
  s->cutoffenv=0;
  s->cutoffenvtarget=0;
  s->lastkey = -1;
  s->mod = 0.5;
  s->reso = 0.6;
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
  double phase1 = s->phase1;
  double phase2 = s->phase2;
  double phaseinctarget = s->phaseinctarget;
  double phaseinc = s->phaseinc;
  double hpstate1 = s->hpstate1;
  double hpstate2 = s->hpstate2;
  double hpcoeff = s->hpcoeff;
  double gate = s->gate;
  for(int i=0;i<length;i++) {
    phaseinc += (phaseinctarget-phaseinc)*500/s->samplerate;
    double osc =
      ticksaw(&phase1,phaseinc*s->bendcoeff*0.998);
    //+ ticksaw(&phase2,phaseinc*s->bendcoeff*2.001);
    osc -= hpstate1; hpstate1 += osc*hpcoeff;
    if (gate > 0) {
      s->cutoffenvtarget -= 1*s->cutoffenvtarget*s->cutoffenvtarget/s->samplerate;
    }
    else {
      s->cutoffenvtarget -= 16*s->cutoffenvtarget*s->cutoffenvtarget/s->samplerate;
    }
    s->cutoffenv += (s->cutoffenvtarget-s->cutoffenv) * 500 / s->samplerate;
    double cutoff = s->cutoffenv * (s->mod*12000 * 2 * 3.141592 / s->samplerate);
    //double sound = ms20filter_tick(&s->filter, osc, 0, cutoff, s->reso*1.2);
    osc *= 0.5;
    double sound = moogfilter_tick(&s->filter, osc, cutoff, s->reso*1.2);
    if (gate > 0) {
      s->ampenvtarget *= 1 - 0.2/s->samplerate;
    }
    else {
      s->ampenvtarget *= 1 - 4/s->samplerate;
    }
    s->ampenv += (s->ampenvtarget-s->ampenv) * 500 / s->samplerate;
    double g = s->ampenv * 0.5;
    sound -= hpstate2; hpstate2 += sound*hpcoeff;
    sound = tanh(sound*g);
    outL[i] = sound;
    outR[i] = sound;
  }
  s->phase1 = phase1;
  s->phase2 = phase2;
  s->phaseinc = phaseinc;
  s->hpstate1 = hpstate1;
  s->hpstate2 = hpstate2;
}
static void noteon(void* synth, int key, float freq, float velocity) {
  struct synth* const s = synth;
  s->phaseinctarget = s->freqtable[key]/s->samplerate;
  s->ampenvtarget = velocity;
  //s->ampenvtarget = sqrt(s->ampenvtarget*s->ampenvtarget+velocity*velocity);
  //s->cutoffenvtarget = sqrt(s->cutoffenvtarget*s->cutoffenvtarget+velocity*velocity);
  //s->ampenvtarget += velocity;
  s->cutoffenvtarget = sqrt(velocity);
  s->gate = velocity;
  s->lastkey = key;
}
static void noteoff(void* synth, int key) {
  struct synth* const s = synth;
  if (key == s->lastkey)
    s->gate = 0;
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

