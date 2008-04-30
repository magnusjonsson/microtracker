#ifndef SYNTHDESC_H_INCLUDED
#define SYNTHDESC_H_INCLUDED

struct synthdesc {
  const char* name;
  int numinputs;
  int numoutputs;
  int (*size)(float samplerate); // amount of memory to allocate for init's first argument
  void (*init)(void* synth, float samplerate);
  void (*finalize)(void* synth); // does not free the memory for the synth
  void (*process)(void* synth, int length, float const*const* in, float*const* out);
  void (*noteon)(void* synth, int voice, float freq, float velocity);
  void (*noteoff)(void* synth, int voice);
  void (*pitchbend)(void* synth, float cents);
  void (*mod)(void* synth, float value);
  void (*vol)(void* synth, float value);
  struct paramdesc* params; // terminated by first param with NULL name
};

struct paramdesc {
  const char* name;
  const char* unit;
  double min,max;
  int isenum:1;
  float (*get)(void* synth);
  void (*set)(void* synth, float value);
};

#endif
