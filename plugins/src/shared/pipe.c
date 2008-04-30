#include "pipe.h"
//#include "lagrange.h"
#include "thiran.h"
#include <math.h>
#include <stdlib.h>

static void calcdelaylength(float samplerate, float frequency,
                            int* out_whole, float* out_frac) {
  float length_in_samples = 0.5 * samplerate / frequency;
  while(length_in_samples < 2) {
    length_in_samples*=2;
    frequency*=0.5;
  }
  length_in_samples -= 0.01;

  int whole_length = (int)(length_in_samples);
  float frac_length = length_in_samples-whole_length;
  while(frac_length < 0.4 && whole_length > 1) {
    frac_length++;
    whole_length--;
  }
  if (out_whole)
    *out_whole = whole_length;
  if (out_frac)
    *out_frac = frac_length;
}

int pipe_memoryneeded(float samplerate, float frequency) {
  int whole=0;
  calcdelaylength(samplerate,frequency,&whole,NULL);
  return delay_memoryneeded(whole);
}

void pipe_init(struct pipe *p, float samplerate, float frequency,
	       float reedfactor, float reflectionfactor, float airfactor,
               void* memory) {
  int whole_length=0;
  float frac_length=0;
  calcdelaylength(samplerate,frequency,&whole_length,&frac_length);
  /*
  for(int i=0;i<4;i++) {
    p->fir_state[i]=0;
  }
  lagrange4coeffs(p->fir_coeff,frac_length-1);
  */
  p->fd_state = 0.0;
  p->fd_coeff = thiran1_coeff(frac_length);
  delay_init(&p->delay,whole_length,memory);

  onepole_init(&p->reflectionfilter, 1);
  onepole_init(&p->reedfilter, 1);

  float omega = 2*3.141592*frequency/samplerate;

  onepole_setcoeff(&p->airlossfilter1, onepole_coeff_for_omega(omega*airfactor));
  onepole_setcoeff(&p->airlossfilter2, onepole_coeff_for_omega(omega*airfactor));
  onepole_setcoeff(&p->reflectionfilter, onepole_coeff_for_omega(omega*reflectionfactor));
  onepole_setcoeff(&p->reedfilter, onepole_coeff_for_omega(omega*reedfactor));
  
  p->gain = 1.0/sqrt(frequency/440);
  p->airflow = 1.0e-6f;
  p->airflowtarget = 1.0e-5f;
  p->airflowspeed = 2*3.14592 * 20 / samplerate;
  p->silencecounter = 0;
}

void pipe_finalize(struct pipe* p) {
  p->silencecounter=0;
  p->gain = 0;
  p->airflow = 0.0f;
  p->airflowtarget = 0.0f;
  p->airflowspeed = 0;
  delay_finalize(&p->delay);
  /*
  for(int i=0;i<4;i++) {
    p->fir_state[i] = 0;
  }
  for(int i=0;i<4;i++) {
    p->fir_coeff[i] = 0;
  }
  */
  onepole_finalize(&p->reflectionfilter);
  onepole_finalize(&p->reedfilter);
  onepole_finalize(&p->airlossfilter1);
  onepole_finalize(&p->airlossfilter2);
}

float fastexp(float x) {
  x = 1+x/128;
  x*=x;
  x*=x;
  x*=x;
  x*=x;
  x*=x;
  x*=x;
  x*=x;
  return x;
}

void pipe_keydown(struct pipe* p) {
  p->airflowtarget = 1.0;
}
void pipe_keyup(struct pipe* p) {
  p->airflowtarget = 1.0e-6;
}
int pipe_isactive(struct pipe* p) {
  return p->airflow > 1.0e-4 || p->airflowtarget > 1.0e-4 || p->silencecounter > 0;
}

float pipe_tick(struct pipe* p) {
  int silencecounter = p->silencecounter;
  double airflow = p->airflow;
  double const airflowtarget = p->airflowtarget;

  airflow += (airflowtarget-airflow) * p->airflowspeed;

  /*
  p->fir_state[3]=p->fir_state[2];
  p->fir_state[2]=p->fir_state[1];
  p->fir_state[1]=p->fir_state[0];
  p->fir_state[0]=delay_read(&p->delay);
  
  double const fdout
    = 1.0e-6
    + p->fir_state[0]*p->fir_coeff[0]
    + p->fir_state[1]*p->fir_coeff[1]
    + p->fir_state[2]*p->fir_coeff[2]
    + p->fir_state[3]*p->fir_coeff[3];
  */

  double const fdout = thiran1_tick(&p->fd_state, p->fd_coeff,
                                    delay_read(&p->delay));

  double const delayout =
    onepole_tick(&p->airlossfilter1,
                 onepole_tick(&p->airlossfilter2,
                              fdout));
  
  double const pipeout = onepole_tick(&p->reflectionfilter,delayout);
  double const reflected = pipeout - delayout;
  double const r = reflected-0.5;
  
  double const pipein = airflow * fastexp(-r*r) * (0.9+0.2*rand()/RAND_MAX);
  double const delayin = pipein + reflected;
  delay_write(&p->delay,delayin);
  if(airflow > 1.0e-4 || fabs(pipeout) > 1.0e-5) {
    silencecounter = p->delay.length*2+1;
  }
  else {
    silencecounter = silencecounter - 1;
  }
  p->silencecounter = silencecounter;
  p->airflow = airflow;
  return pipeout*p->gain;
}
