#include "onepole.h"
#include <assert.h>

void onepole_init(struct onepole* o, double coeff) {
  o->state = 0;
  o->coeff = coeff;
}

void onepole_finalize(struct onepole* o) {
  o->state = 0;
  o->coeff = 0;
}

void onepole_setcoeff(struct onepole* o, double coeff) {
  assert(0 <= coeff);
  assert(coeff <= 2);
  o->coeff=coeff;
}

double onepole_tick(struct onepole* o, double input) {
  double coeff = o->coeff;
  double state = o->state;
  state += (input-state) * coeff;
  o->state = state;
  return state;
}

#include <math.h>

double onepole_coeff_for_time_variance(double time_variance) {
  return 1/(0.5+sqrt(0.25+time_variance));
}

double onepole_coeff_for_time_spread(double time_spread) {
  return onepole_coeff_for_time_variance(time_spread*time_spread);
}

double onepole_coeff_for_omega(double omega) {
  //return onepole_coeff_for_time_variance(1/(omega*omega));
  //return 1.0-exp(-omega);

  // based on time delay:
  return omega/(1+omega);
}
