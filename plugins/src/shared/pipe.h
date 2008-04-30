#include "delay.h"
#include "onepole.h"
#include "fractionaldelay.h"

struct pipe {
  int silencecounter;
  float gain;
  double airflow;
  float airflowtarget;
  float airflowspeed;
  struct delay delay;
  //double fir_state[4];
  //double fir_coeff[4];
  //struct fractionaldelay fractionaldelay;
  double fd_state; // fractional delay state
  double fd_coeff; // fractional delay coeff
  struct onepole reflectionfilter;
  struct onepole reedfilter;
  struct onepole airlossfilter1;
  struct onepole airlossfilter2;
};

int pipe_memoryneeded(float samplerate, float frequency);

// memory is optional.
// if memory is null, then the pipe will allocate and clear its own memory
// if memory is not null, the pipe will use this memory. The memory must be
// cleared by the caller before calling pipe_init.
void pipe_init(struct pipe *p, float samplerate, float frequency,
	       float reedfactor, float reflectionfactor, float airfactor,
               void* memory);
void pipe_finalize(struct pipe* p);

void pipe_keydown(struct pipe* p);
void pipe_keyup(struct pipe* p);

int pipe_isactive(struct pipe* p);

float pipe_tick(struct pipe* p);
