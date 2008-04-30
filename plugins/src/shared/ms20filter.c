#include "ms20filter.h"
#include "fasttanh.h"

void ms20filter_init(struct ms20filter* f) {
  f->state0 = 0;
  f->state1 = 0;
}

double ms20filter_tick(struct ms20filter* s, double lp_in, double bp_in, double omega, double resonance /* max 1 */) {
  //  if (resonance > 0.999)
  //    resonance = 0.999;
  if (omega > 1)
    omega = 1;
  double r = resonance * (2-omega); // 1+1/(1-omega)
  double s0 = s->state0;
  double s1 = s->state1;
  double feed0 = lp_in + bp_in - s1*r;
  double feed1 = s0 - bp_in + s1*r;
  s0 = s0 + omega*(fasttanh(feed0)-fasttanh(s0));
  s1 = s1 + omega*(fasttanh(feed1)-fasttanh(s1));
  s->state0 = s0;
  s->state1 = s1;
  return s1;
}
