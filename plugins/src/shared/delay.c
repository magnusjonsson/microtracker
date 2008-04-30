#include "delay.h"

#include <stdlib.h>

int delay_memoryneeded(int length) {
  return length*sizeof(float);
}

void delay_clear(struct delay* d) {
  int i;
  int length=d->length;
  float* buffer=d->buffer;
  for(i=0;i<length;i++) {
    buffer[i]=0.0f;
  }
}

void delay_init(struct delay* d, int length, void* memory) {
  d->length = length;
  d->buffer = memory ? memory : calloc(delay_memoryneeded(length),1);
  d->pointer = 0;
  d->shouldfreebuffer = memory ? 0 : 1;
}

float delay_read(struct delay* d) {
  float* buffer=d->buffer;
  int pointer = d->pointer;
  float out = buffer[pointer];
  return out;
}

void delay_write(struct delay* d, float in) {
  float* buffer=d->buffer;
  int pointer = d->pointer;
  int length = d->length;
  buffer[pointer]=in;
  pointer++;
  if(pointer >= length) {
    pointer = 0;
  }
  d->pointer = pointer;
}

float delay_tick(struct delay* d, float in) {
  float* buffer=d->buffer;
  int pointer = d->pointer;
  int length = d->length;
  float out = buffer[pointer];
  buffer[pointer]=in;
  pointer++;
  if(pointer >= length) {
    pointer = 0;
  }
  d->pointer = pointer;
  return out;
}

void delay_finalize(struct delay* d) {
  d->length = 0;
  if (d->shouldfreebuffer) {
    free(d->buffer);
  }
  d->buffer = NULL;
}
