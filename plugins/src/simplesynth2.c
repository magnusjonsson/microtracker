#include "synthdesc.h"
#include "ms20filter.h"
#include <strings.h> // bzero
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <stdint.h>

#define OSCS 1

struct voice {
  struct ms20filter filterL;
  struct ms20filter filterR;
  double phase[OSCS];
  double drift[OSCS];
  double gate;
  double fgain;
  double phaseinc;
  double invphaseinc;
  double smoothedamp;
  int active;
};
enum osctype {
  saw,
  thorn
};

struct synth {
  struct voice voice[128];
  double driftdepth;
  double ampattack;
  double amprelease;
  double whitenoiseamp; // depends on samplerate
  double noiselowpasscoeff;
  double samplerate;
  uint32_t rng_state;
  double bend;
  double invbend;
  double filterscale;
  double resonance;
  double dcfollower;
  int osctype;
  int vcftype;
};

static void init(void* synth, float samplerate) {
  struct synth* s = synth;
  int i,j;
  for(i=0;i<128;i++) {
    struct voice* v = &s->voice[i];
    v->gate=0;
    for(j=0;j<OSCS;j++) {
      ms20filter_init(&v->filterL);
      ms20filter_init(&v->filterR);
      v->phase[j]=-0.5+rand()*(1.0/(RAND_MAX+1.0));
      v->drift[j]=0;
    }
    v->smoothedamp=1.0e-5;
    v->active = 0;
  }
  s->samplerate = samplerate;
  s->ampattack = 0.001;
  s->amprelease = 0.020;
  s->whitenoiseamp = sqrt(samplerate/44100);
  s->noiselowpasscoeff = 3.141592*2/samplerate;
  s->bend = 1.0;
  s->invbend = 1.0;
  s->filterscale=8*(2*3.141592);
  s->driftdepth=0.35;
  s->osctype=saw;
  s->resonance = 0.0;
  s->dcfollower = 1.0e-6;
}

static void process(void* synth, int length, float const* const* in, float* const* out) {
  struct synth* const s = synth;
  double const whitenoiseamp = s->whitenoiseamp;
  double const noiselowpasscoeff = s->noiselowpasscoeff;
  double const driftdepth = s->driftdepth;
  double const drift_ingain = whitenoiseamp * noiselowpasscoeff * (1.0/32768.0/65536.0);
  double const drift_fbgain = 1-noiselowpasscoeff;
  double const bend = s->bend;
  double const invbend = s->invbend;
  int rng_state = s->rng_state;
  double ampattackcoeff = 1.0/(s->ampattack*s->samplerate+0.0000001);
  if (ampattackcoeff > 0.5)
    ampattackcoeff = 0.5;
  double ampreleasecoeff = 1.0/(s->amprelease*s->samplerate+0.0000001);
  if (ampreleasecoeff > 0.5)
    ampreleasecoeff = 0.5;
  int const osctype = s->osctype;
  
  float* restrict outleft=out[0];
  float* restrict outright=out[1];

  for(int sample = 0; sample<length;sample++) {
    outleft[sample]=1.0e-12;
    outright[sample]=1.0e-12;
  }

  for(int voiceno=0;voiceno<128;voiceno++) {
    struct voice* restrict v = &s->voice[voiceno];
    if(v->active) {
      double const phaseinc = v->phaseinc * bend;

      //double const sqrtinvphaseinc = sqrt(v->invphaseinc * invbend);
      //double const gain = 1;
      double gain = pow(440.0/(s->samplerate*phaseinc*bend),0.2);
      //double const gain = sqrt(440.0/(s->samplerate*phaseinc*bend));
      //double const gain = (440.0/(s->samplerate*phaseinc*bend));

      double smoothedamp = v->smoothedamp;
      double gate = v->gate+1.0e-6;
      
      for(int sample = 0; sample<length;sample++) {
        double oscsL = 1.0e-5;
        double oscsR = 1.0e-5;
        
        for(int j=0;j<OSCS;j++) {
          //          double pan = (j+0.5)*(1.0/OSCS);
          double pan = (0.5+(voiceno&3))*(1.0/4.0);
          
          double drift = v->drift[j];
          drift = (int)rng_state * drift_ingain + drift * drift_fbgain;
          rng_state = (rng_state * 196314165u) + 907633515u;
          v->drift[j] = drift;
          double phase = v->phase[j];
          double pinc = phaseinc * (1.0+drift*driftdepth);

          double osc;

          switch(osctype) {
          default:
          case saw:
            osc = phase+pinc*0.5;
            phase += pinc;
            while (phase >= 0.5) {
              osc -= (phase-0.5) / pinc;
              phase -= 1;
            }
            break;
          case thorn:
            phase += pinc;
            while (phase >= 0.5) {
              phase -= 1;
            }
            osc = phase*phase*4-0.3333333333;

            break;
          }

          v->phase[j]=phase;
          double gainL = sqrt(1-pan);
          double gainR = sqrt(pan);
          oscsL += gainL * osc;
          oscsR += gainR * osc;
        }
        double smoothedampdiff = (gate-smoothedamp);
        double env = smoothedamp+=smoothedampdiff*
          (smoothedampdiff > 0 ? ampattackcoeff : ampreleasecoeff);

        double omega = s->filterscale * v->fgain;
        double resonance = s->resonance;

        oscsL = ms20filter_tick(&v->filterL, oscsL, 0, omega, resonance);
        oscsR = ms20filter_tick(&v->filterR, oscsR, 0, omega, resonance);

        env *= gain * sqrt(0.01/OSCS);
        oscsL *= env;
        oscsR *= env;

        outleft[sample] += oscsL;
        outright[sample] += oscsR;
      }
      
      v->smoothedamp = smoothedamp;
      if (!v->gate && smoothedamp < 1.0e-4) {
        v->active = 0;
      }
    }
  }
  for(int sample = 0; sample<length;sample++) {
    outleft[sample] -= s->dcfollower;
    outright[sample] -= s->dcfollower;
    s->dcfollower += (outleft[sample]+outright[sample])*0.5*120/s->samplerate;
  }
    
  s->rng_state = rng_state;
}

static void noteon(void* synth, int key, float freq, float velocity) {
  struct synth* const s = synth;
  struct voice* const v = &s->voice[key];
  v->phaseinc = freq/s->samplerate;
  v->invphaseinc = 440.0/44100.0/v->phaseinc;

  //v->gate = velocity;
  v->gate = 1.0;
  float base = 440 * pow(freq / 440, 0.2);
  v->fgain=base*velocity / s->samplerate;
  v->active=1;
};

static void noteoff(void* synth, int key) {
  struct synth* const s = synth;
  struct voice* const v = &s->voice[key];
  v->gate=0;
};

static void vol(void* synth, float vol) {
  struct synth* s = synth;
  s->resonance = vol;
}

static void mod(void* synth, float mod) {
  struct synth* s = synth;
  //s->driftdepth = mod*2.0;
  s->filterscale = mod*16.0*(2*3.141592);
}
static void pitchbend(void* synth, float cents) {
  struct synth* s = synth;
  s->bend = pow(2.0,cents/1200.0);
  s->invbend = 1.0/s->bend;
}

static void attack(void* synth, float seconds) {
  struct synth* s = synth;
  s->ampattack = seconds;
}
static void release(void* synth, float seconds) {
  struct synth* s = synth;
  s->amprelease = seconds;
}

int cmd_values(void* actiondata, void* v, char* line) {
  struct synth* s = v;
  printf("attack: %lf (s)\n",s->ampattack);
  printf("release: %lf (s)\n",s->amprelease);
  return 0;
}

static struct paramdesc params[] = {
  { .name="attack", .unit="s", .min=0.0, .max=10.0, .set = attack },
  { .name="release", .unit="s", .min=0.0, .max=10.0, .set = release },
  { }
};

static int size(float samplerate) {
  return sizeof(struct synth);
}

struct synthdesc synthdesc = {
  .name = "simplesynth2",
  .numinputs = 0,
  .numoutputs = 2,
  .size = size,
  .params = params,
  .init = init,
  .process = process,
  .noteon = noteon,
  .noteoff = noteoff,
  .pitchbend = pitchbend,
  .mod = mod,
  .vol = vol,
};
