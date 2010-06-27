#include "synthdesc.h"

#include <strings.h> // bzero
#include <stdlib.h>
#include <stdint.h>
//#include "twopole.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

struct plucksynthvoice {
  double phase0;
  double phase1;
  double cutoff0;
  double cutoff1;
  double state00;
  double state01;
  double state10;
  double state11;
  double drift;
  double gate;
  double phaseinc;
  double invphaseinc;
  double amp;
  double smoothedamp;
  int active;
};

struct plucksynth {
  struct plucksynthvoice voice[128];
  double driftdepth;
  double ampattack;
  double ampdecay;
  double amprelease;
  double whitenoiseamp; // depends on samplerate
  double noiselowpasscoeff;
  double samplerate;
  uint32_t rng_state;
  double bend;
  double invbend;
  double cutoffscaling;
  double kgain;
  int waveform;
  double decayhfdamping0;
  double decayhfdamping1;
  double releasehfdamping;
  double reso0;
  double reso1;
};

static void init(void* synth, float samplerate) {
  struct plucksynth* const s = synth;
  for(int i=0;i<128;i++) {
    struct plucksynthvoice* v = &s->voice[i];
    v->gate=0;
    v->phaseinc=0;
    v->invphaseinc=0;
    // phaseinhc and invphaseinc set by retune call below.
    v->phase0=rand()*(1.0/(RAND_MAX+1.0));
    v->phase1=rand()*(1.0/(RAND_MAX+1.0));
    v->drift=0;
    v->cutoff0 = 2000;
    v->cutoff1 = 2000;
    v->state00 = 0;
    v->state01 = 0;
    v->state10 = 0;
    v->state11 = 0;
    v->amp=1.0e-5;
    v->smoothedamp=v->amp;
    v->active = 0;
  }
  s->samplerate = samplerate;
  s->ampattack = 1000;
  s->ampdecay = 0.1;
  s->amprelease = 50;
  s->whitenoiseamp = sqrt(samplerate/44100);
  s->noiselowpasscoeff = 0.1*3.141592*2/samplerate;
  s->bend = 1.0;
  s->invbend = 1.0;
  s->cutoffscaling = 2*3.141592 / samplerate;
  s->kgain=3.141592*2;
  s->waveform = 1;
  s->decayhfdamping0=0.009;
  s->decayhfdamping1=0.003;
  s->releasehfdamping=0.020;
  s->reso0 = 0.0;
  s->reso1 = 0.0;
  s->driftdepth = 0.01;
}

static void process(void* synth, int length, float const * const * in, float * const * out) {
  struct plucksynth* const s = synth;
  float * outleft = out[0];
  float * outright = out[1];
  double whitenoiseamp = s->whitenoiseamp;
  double noiselowpasscoeff = s->noiselowpasscoeff;
  double driftdepth = s->driftdepth;
  double drift_ingain = whitenoiseamp * noiselowpasscoeff * (1.0/32768.0/65536.0);
  double drift_fbgain = 1-noiselowpasscoeff;
  double bend = s->bend;
  double invbend = s->invbend;
  int rng_state = s->rng_state;
  double cutoffscaling = s->cutoffscaling;
  double kgain = s->kgain;
  int waveform = s->waveform;
  double decayhfdampingcoeff0 = s->decayhfdamping0 / s->samplerate;
  double decayhfdampingcoeff1 = s->decayhfdamping1 / s->samplerate;
  double releasehfdampingcoeff = s->releasehfdamping / s->samplerate;
  double ampattackcoeff = 1-exp(-s->ampattack/s->samplerate-0.0000001);
  double ampdecaycoeff = 1-exp(-s->ampdecay/s->samplerate-0.0000001);
  double ampreleasecoeff = 1-exp(-s->amprelease/s->samplerate-0.0000001);
  double reso0 = s->reso0;
  double reso1 = s->reso1;
  for(int i=0;i<length;i++) {
    outleft[i]=1.0e-5;
    outright[i]=1.0e-5;
  }
  
  for(int i=0;i<128;i++) {
    struct plucksynthvoice* v = &s->voice[i];
    if(v->active) {
      for(int j=0;j<length;j++) {
        double oscsL = 1.0e-5;
        double oscsR = 1.0e-5;
        float k = kgain;
        if (k < 2.0)
          k = 2.0;
        double dc = 1.0/k;
        double g = sqrt(dc)*0.25;
        double phaseinc = v->phaseinc * bend;
        double const sqrtinvphaseinc = sqrt(v->invphaseinc * invbend);
        
        double phase0 = v->phase0;
        double phase1 = v->phase1;
        double drift = v->drift;
        drift = (int)rng_state * drift_ingain + drift * drift_fbgain;
        rng_state = (rng_state * 196314165u) + 907633515u;

        double phaseinc0 = phaseinc*(0.999+drift*driftdepth);
        double phaseinc1 = phaseinc*(1.001+drift*driftdepth);

        double p = 1.0/2.0/3.141592;
 
        double osc0before = phase0 < p ? phase0/p : (1-phase0)/(1-p);
        double osc1before = phase1 < p ? phase1/p : (1-phase1)/(1-p);

        phase0 += phaseinc0;
        phase1 += phaseinc1;

        v->drift = drift;
        if (phase0 >= 1.0) phase0 -= 1;
        v->phase0 = phase0;
        if (phase1 >= 1.0) phase1 -= 1;
        v->phase1 = phase1;

        double osc0after = phase0 < p ? phase0/p : (1-phase0)/(1-p);
        double osc1after = phase1 < p ? phase1/p : (1-phase1)/(1-p);
        
        double osc0=0;
        double osc1=0;
        switch(waveform) {
        case 0:
          osc0 = (osc0after-0.5)*sqrtinvphaseinc;
          osc1 = (osc1after-0.5)*sqrtinvphaseinc;
          break;
        case 1:
          osc0 = (osc0after-osc0before)/phaseinc0*0.25;
          osc1 = (osc1after-osc1before)/phaseinc1*0.25;
          break;
        };
        
        double cutoff0 = v->cutoff0;
        double cutoffgain0 = 1-cutoff0 *
          (v->gate > 0.0001 ? decayhfdampingcoeff0 : releasehfdampingcoeff);
        if (cutoffgain0 < 0.8) {
          cutoffgain0=0.8;
        }
        cutoff0 *= cutoffgain0;
        v->cutoff0 = cutoff0;
        double c0 = cutoff0 * cutoffscaling;
        
        
        double cutoff1 = v->cutoff1;
        double cutoffgain1 = 1-cutoff1 *
          (v->gate > 0.0001 ? decayhfdampingcoeff1 : releasehfdampingcoeff);
        if (cutoffgain1 < 0.8) {
          cutoffgain1=0.8;
        }
        cutoff1 *= cutoffgain1;
        v->cutoff1 = cutoff1;
        double c1 = cutoff1 * cutoffscaling;
        
        if(c0 > 0.9)
          c0 = 0.9;
        double r0= reso0*(2.0-c0);
        double s00 = v->state00;
        double s01 = v->state01;
        double feed00 = osc0 - s01*r0;
        double feed01 = s00  + s01*r0;
        s00 = s00 + c0*(feed00-s00);
        s01 = s01 + c0*(feed01-s01);
        v->state00 = s00;
        v->state01 = s01;
        
        
        if(c1 > 0.9)
          c1 = 0.9;
        double r1= reso1*(2.0-c1);
        double s10 = v->state10;
        double s11 = v->state11;
        double feed10 = osc1 - s11*r1; // !!!! should be osc1
        double feed11 = s10  + s11*r1;
        s10 = s10 + c1*(feed10-s10);
        s11 = s11 + c1*(feed11-s11);
        v->state10 = s10;
        v->state11 = s11;
        
        double osc = s01+s11;
        
        oscsL += osc*g;
        oscsR += osc*g;
        double smoothedamp = v->smoothedamp;
        double smoothedampdiff = (v->gate+1.0e-6-smoothedamp);
        v->gate *= 1-ampdecaycoeff;
        double env = smoothedamp+=smoothedampdiff*(1-ampattackcoeff);
        v->smoothedamp = smoothedamp;

        env *= 0.4;
        outleft[j] += env * oscsL;
        outright[j] += env * oscsR;
        if (v->gate < 1.0e-4 && smoothedamp < 1.0e-4) {
          v->active = 0;
          break;
        }
      }
    }
    s->rng_state = rng_state;
  }
}

static void finalize(void* synth) {
  struct plucksynth* const s = synth;
  bzero(s,sizeof(*s));
}

static void noteon(void* synth, int key, float freq, float velocity) {
  struct plucksynth* const s = synth;
  struct plucksynthvoice* const v = &s->voice[key];
  v->phaseinc = freq/s->samplerate;
  v->invphaseinc = 440.0/44100/v->phaseinc;
  v->gate = sqrt(velocity);
  v->active = 1;
  v->cutoff0 = 12000*velocity;
  v->cutoff1 = 12000*velocity;
  //  v->phase0 = 0;
  //  v->phase1 = 0;
};

static void noteoff(void* synth, int key) {
  struct plucksynth* const s = synth;
  s->voice[key].gate=0;
};

static void vol(void* synth, float vol) {
  struct plucksynth* const s = synth;
  s->kgain=2*vol;
}

static void mod(void* synth, float mod) {
  struct plucksynth* const s = synth;
  s->driftdepth = mod*2.0;
}
static void pitchbend(void* synth, float bend) {
  struct plucksynth* const s = synth;
  s->bend = pow(9.0/8.0,bend);
  s->invbend = 1.0/s->bend;
}

static void attack(void* synth, float seconds) {
  struct plucksynth* const s = synth;
  s->ampattack = seconds;
}
static void release(void* synth, float seconds) {
  struct plucksynth* const s = synth;
  s->amprelease = seconds;
}

static void decayhfdamping0(void* synth, float decayhfdamping) {
  struct plucksynth* s = synth;
  s->decayhfdamping0=decayhfdamping;
}
static void decayhfdamping1(void* synth, float decayhfdamping) {
  struct plucksynth* s = synth;
  s->decayhfdamping1=decayhfdamping;
}
static void releasehfdamping(void* synth, float releasehfdamping) {
  struct plucksynth* s = synth;
  s->releasehfdamping=releasehfdamping;
}
static void reso0(void* synth, float reso0) {
  struct plucksynth* s = synth;
  s->reso0 = reso0;
}
static void reso1(void* synth, float reso1) {
  struct plucksynth* s = synth;
  s->reso1 = reso1;
}

static struct paramdesc params[] = {
  { .name = "attack", .min = 0, .max = 10, .set = attack },
  { .name = "release", .min = 0, .max = 10, .set = release },
  { .name = "decayhfdamping0", .min = 0, .max = 0.1, .set = decayhfdamping0 },
  { .name = "decayhfdamping1", .min = 0, .max = 0.1, .set = decayhfdamping1 },
  { .name = "releasehfdamping", .min = 0, .max = 0.1, .set = releasehfdamping },
  { .name = "reso0", .min = 0, .max = 1, .set = reso0 },
  { .name = "reso1", .min = 0, .max = 1, .set = reso1 },
  { }
};

static int size(float samplerate) {
  return sizeof(struct plucksynth);
}

struct synthdesc synthdesc = {
  .name = "plucksynth",
  .numinputs = 0,
  .numoutputs = 2,
  .size = size,
  .init = init,
  .finalize = finalize,
  .process = process,
  .noteon = noteon,
  .noteoff = noteoff,
  .params = params,
  .vol = vol,
  .mod = mod,
  .pitchbend = pitchbend,
};
