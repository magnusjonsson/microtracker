#include "synthdesc.h"
#include "shared/onepole.h"
#include "shared/moogfilter.h"
#include "shared/fasttanh.h"
#include <strings.h> // bzero
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <stdint.h>

#define LPFREQ2 16000

#define RESONANCE 0.1

#define CUTOFF 4000
#define DC_GAIN 0.5

#define GAIN_TRACKING 0.20
#define CUTOFF_TRACKING 0.7

struct voice {
  struct moogfilter filter;
  double lpstate1;
  double lpstate2;
  double phase1;
  double drift1;
  double gate;
  double freq;
  double cutoff;
  double smoothedfreq;
  double smoothedcutoff;
  double smoothedamp;
  int active;
};

struct synth {
  struct voice voice[128];
  double driftdepth;
  double ampattack;
  double amprelease;
  double freqattack;
  double filterattack;
  double filterrelease;
  double whitenoiseamp; // depends on samplerate
  double noiselowpasscoeff;
  double samplerate;
  uint32_t rng_state;
  double bend;
  double invbend;
  double mod;
  double resonance;
  double dcfollower;
  int vcftype;
};

static void init(void* synth, float samplerate) {
  struct synth* s = synth;
  int i;
  for(i=0;i<128;i++) {
    struct voice* v = &s->voice[i];
    moogfilter_init(&v->filter);
    v->phase1=-0.5+rand()*(1.0/(RAND_MAX+1.0));
    v->drift1=0;
    v->freq=220;
    v->cutoff=CUTOFF;
    v->smoothedfreq=220;
    v->smoothedcutoff=v->cutoff;
    v->gate=0;
    v->smoothedamp=1.0e-5;
    v->active=0;
  }
  s->samplerate = samplerate;
  s->ampattack = 0.003;
  s->amprelease = 0.010;
  s->freqattack = 0.003;
  s->filterattack = 0.010;
  s->whitenoiseamp = sqrt(samplerate/44100);
  s->noiselowpasscoeff = 5*2*3.141592/samplerate;
  s->bend = 1.0;
  s->invbend = 1.0;
  s->mod = 0.35;
  s->driftdepth=0.1;
  s->resonance = RESONANCE;
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
  int rng_state = s->rng_state;
  double ampattackcoeff = 1.0/(s->ampattack*s->samplerate+0.0000001);
  if (ampattackcoeff > 0.5)
    ampattackcoeff = 0.5;
  double ampreleasecoeff = 1.0/(s->amprelease*s->samplerate+0.0000001);
  if (ampreleasecoeff > 0.5)
    ampreleasecoeff = 0.5;
  double freqattackcoeff = 1.0/(s->freqattack*s->samplerate+0.0000001);
  if (freqattackcoeff > 0.5)
    freqattackcoeff = 0.5;
  double filterattackcoeff = 1.0/(s->filterattack*s->samplerate+0.0000001);
  if (filterattackcoeff > 0.5)
    filterattackcoeff = 0.5;
  
  float* restrict outleft=out[0];
  float* restrict outright=out[1];

  for(int sample = 0; sample<length;sample++) {
    outleft[sample]=1.0e-12;
    outright[sample]=1.0e-12;
  }
  double const omega2 = LPFREQ2 * 2 * 3.141592 / s->samplerate;
  //double const lpcoeff2 = omega2 / (omega2 + 1);

  for(int voiceno=0;voiceno<128;voiceno++) {
    struct voice* restrict v = &s->voice[voiceno];
    if(v->active) {
      double const freq = v->freq * bend;
      double smoothedfreq = v->smoothedfreq;

      double smoothedamp = v->smoothedamp;
      double const gate = v->gate+1.0e-6;
      
      double const pan = ((voiceno&3)+0.5)/4.0;
      double const gainL = sqrt(1-pan);
      double const gainR = sqrt(pan);
          
      for(int sample = 0; sample<length;sample++) {
        
	double drift1 = v->drift1;
	drift1 = (int)rng_state * drift_ingain + drift1 * drift_fbgain;
	rng_state = (rng_state * 196314165u) + 907633515u;
	v->drift1 = drift1;
	double phase1 = v->phase1;
	smoothedfreq += (freq - smoothedfreq) * freqattackcoeff * smoothedfreq / 440.0;
	double phaseinc = smoothedfreq / s->samplerate; 
	double pinc1 = phaseinc * (1.0+drift1*driftdepth);
	
	/*
	phase1 += pinc1;
	while (phase1 >= 1.0) { phase1 -= 1; }
	double osc = phase1;
	*/


	/* double osc = phase1 + pinc1*0.5; */
	/* phase1 += pinc1; */
	/* while (phase1 >= 1.0) { osc -= (phase1-1.0) / pinc1; phase1 -= 1; } */
        phase1 += pinc1;
	double k = -omega2;
	double ek = exp(k);
	v->lpstate2 *= ek;
	{
	  double c_a = 1-ek;
	  double c_b = - ek - c_a / k;
	  double a = phase1;
	  double b = -pinc1;

	  v->lpstate2 += c_a * a + c_b * b;
	}
	while (phase1 >= 1.0) {
	  double t = (phase1 - 1.0) / pinc1;
	  v->lpstate2 -= 1 - exp(k*t);
	  phase1 -= 1.0;
	}
	double osc = v->lpstate2 - 0.5;
	//double osc = v->phase1 - 0.5;

        double cutoff = v->cutoff * powf(smoothedfreq/440,CUTOFF_TRACKING);;
	v->smoothedcutoff += (cutoff - v->smoothedcutoff) * filterattackcoeff * v->smoothedcutoff / 10000;

	v->phase1=phase1;

        double smoothedampdiff = (gate - smoothedamp);
        double env = smoothedamp+=smoothedampdiff*
          (smoothedampdiff > 0 ? ampattackcoeff : ampreleasecoeff);

        double const gain = DC_GAIN * powf(smoothedfreq/440,-GAIN_TRACKING);

	//double freq_above_gain = smoothedfreq/GAIN_CORNER;
	//double gain = DC_GAIN / sqrt(1.0 + freq_above_gain * freq_above_gain);

        osc *= env;
	double omega = cutoff * 2 * 3.141592 / s->samplerate;
	osc *= 4;
	double lpcoeff1 = omega / (omega + 1);
	osc = v->lpstate1 += fasttanh(osc - v->lpstate1) * lpcoeff1;
	//osc = moogfilter_tick(&v->filter, osc, omega, s->resonance);
	osc *= 0.125;
	osc *= gain;
	
	//osc = v->lpstate1 += fasttanh(osc - v->lpstate1) * lpcoeff1;
	//osc = v->lpstate2 += (osc - v->lpstate2) * lpcoeff2;

        outleft[sample] += osc * gainL;
        outright[sample] += osc * gainR;
      }
      
      v->smoothedfreq = smoothedfreq;
      v->smoothedamp = smoothedamp;
      if (!v->gate && smoothedamp < 1.0e-6) {
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
  v->freq = freq;

  v->gate = 1.0;
  v->cutoff = CUTOFF * velocity * 2;
  v->active=1;
};

static void noteoff(void* synth, int key) {
  struct synth* const s = synth;
  struct voice* const v = &s->voice[key];
  v->gate=0.0;
};

static void vol(void* synth, float vol) {
  struct synth* s = synth;
  s->resonance = vol;
}

static void mod(void* synth, float mod) {
  struct synth* s = synth;
  s->mod = mod;
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
  .name = "simplesynth",
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
