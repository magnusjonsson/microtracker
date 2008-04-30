#include "fractionaldelay.h"

void fractionaldelay_init(struct fractionaldelay* fd, float delay) {
  fd->last_in = 0;
  fd->delay = delay;
}

void fractionaldelay_finalize(struct fractionaldelay* fd) {
  fd->last_in = 0;
  fd->delay = 0;
}

inline
float fractionaldelay_tick(struct fractionaldelay* fd, float in) {
  float last_in = fd->last_in;
  float delay = fd->delay;
  fd->last_in = in;
  return in*(1-delay)+last_in*delay;
}

