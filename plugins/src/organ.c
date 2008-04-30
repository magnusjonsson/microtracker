#include "shared/pipe.h"
#include "synthdesc.h"
#include <assert.h>
#include <malloc.h>
#include <memory.h>

struct voice {
  struct voice* next;
  int size;
  int key;
  struct pipe pipe;
};

struct organ {
  double samplerate;
  double reedfactor;
  double reflectionfactor;
  double airfactor;
  double dckillerstate;
  double dckillercoeff;

  struct voice* first_voice;
  void* memory_start;
  void* memory_stop;
};

void init(void* s, float samplerate) {
  struct organ* o = s;
  o->samplerate=samplerate;
  o->reedfactor = 0;
  o->reflectionfactor = 0.5;
  o->airfactor = 6;
  o->dckillerstate = 1.0e-9;
  o->dckillercoeff = 6.28*50/samplerate;
  int memory_size = (int) (samplerate * 1) * sizeof(float);
  o->memory_start = calloc(memory_size,1);
  o->memory_stop = o->memory_start + memory_size;
  o->first_voice = o->memory_stop;
}

void finalize(void* s) {
  struct organ* o = s;
  free(o->memory_start);
  memset(o, 0, sizeof(struct organ));
}

static struct voice* find_voice(struct organ* restrict o, int key) {
  struct voice** next_addr = &o->first_voice;
  while(1) {
    struct voice* next = *next_addr;
    if ((void*)next >= o->memory_stop)
      return NULL;
    if (next->key == key)
      return next;
    next_addr = &next->next;
  }
}

static struct voice* allocate_voice(struct organ* restrict o, float frequency) {
  int size = sizeof(struct voice) + pipe_memoryneeded(o->samplerate,frequency);
  size = (size + 7) & ~7; // round up to a multiple of 8
                          // so that doubles can be are aligned properly
  void* ptr = o->memory_start;
  struct voice** next_addr = &o->first_voice;
  while(1) {
    struct voice* next = *next_addr;
    if ((void*) next - ptr >= size) {
      struct voice* new_voice = (struct voice*) ptr;
      *next_addr = new_voice;
      new_voice->next = next;
      new_voice->size = size;
      return new_voice;
    }
    if ((void*)next >= o->memory_stop) {
      return NULL;
    }
    ptr = (void*)next + next->size;
    next_addr = &next->next;
  }
}
/*
void free_voice(struct organ* restrict o, struct voice* voice) {
  struct voice** next_addr = &o->first_voice;
  while(1) {
    struct voice* next = *next_addr;
    if ((void*)next >= o->memory_stop) {
      assert(0);
      return;
    }
    if (next == voice) {
      o->voice[voice->key]=NULL;
      *next_addr = next->next;
      return;
    }
    next_addr = &next->next;
  }
}
*/
static void noteon(void* s, int voice, float freq, float velocity) {
  struct organ* restrict o = s;
  assert(0 <= voice && voice < 128);
  assert(0 < freq);

  struct voice* v = find_voice(o,voice);

  if (v == NULL) {
    freq *= 0.979;
    v = allocate_voice(o,freq);
    if (v == NULL)
      // out of memory, so ignore key down event
      return;
    v->key = voice;
    pipe_init(&v->pipe,o->samplerate,freq,
              o->reedfactor,o->reflectionfactor,o->airfactor,
              (void*)v + sizeof(struct voice));
  }
  pipe_keydown(&v->pipe);
}

static void noteoff(void* s, int key) {
  struct organ* restrict o = s;
  assert(0 <= key);
  assert(key < 128);
  struct voice* v = find_voice(o,key);
  if (v) {
    pipe_keyup(&v->pipe);
    v->key = -1;
  }
}

static void process(void* s, int length, float const * const * in, float * const * out) {
  struct organ* o = s;
  float* outleft = out[0];
  float* outright = out[1];
  for (int i=0;i<length;i++) {
    double organout = 1.0e-5;
    struct voice** next_addr=&o->first_voice;
    while(1) {
      struct voice* next = *next_addr;
      if ((void*)next >= o->memory_stop)
        break;
      if (!pipe_isactive(&next->pipe)) {
        // remove voice from list
        *next_addr = next->next;
        memset(next,0,sizeof(struct voice));
      }        
      else {
        organout += pipe_tick(&next->pipe);
        next_addr = &next->next;
      }
    }
    organout -= o->dckillerstate;
    o->dckillerstate += organout * o->dckillercoeff;
    organout *= 0.10;
    outleft[i] = organout;
    outright[i] = organout;
  }
}

static int size(float samplerate) {
  return sizeof(struct organ);
}

struct synthdesc synthdesc = {
  .name = "organ",
  .numinputs = 0,
  .numoutputs = 2,
  .size = size,
  .init = init,
  .finalize = finalize,
  .process = process,
  .noteon = noteon,
  .noteoff = noteoff,
};
