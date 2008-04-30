struct twopole {
  double state0L;
  double state0R;
  double state1L;
  double state1R;
  float theta;
  float radius;
  float re;
  float im;
};

void twopole_init(struct twopole* f, float theta, float radius);
void twopole_finalize(struct twopole* f);
void twopole_setradius(struct twopole* f, float radius);
void twopole_settheta(struct twopole* f, float theta);
float twopole_getradius(struct twopole* f);
float twopole_gettheta(struct twopole* f);
void twopole_tick(struct twopole* f, float* inoutleft, float* inoutright);
void twopole_process(struct twopole* f, int length, float const* inL, float const* inR, float* outL, float* outR);
void twopole_processadding(struct twopole* f, int length, float gain, float const* inL, float const* inR, float* outL, float* outR);
