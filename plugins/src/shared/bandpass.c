#include <math.h>
#include "bandpass.h"

void bandpasscoeffs_identity(struct bandpasscoeffs* c) {
  double alpha = 100000000;
  c->gain = alpha / (1 + alpha);
  c->fb1 = 0;
  c->fb2 = (1-alpha) / (1 + alpha);
}
void bandpasscoeffs_from_omega_and_q(struct bandpasscoeffs* c, double omega, double q) {
  if (q == 0) {
    bandpasscoeffs_identity(c);
  }
  else {
    double alpha = sin(omega)/q*0.5;
    
    double b0 = alpha;
    //  double b1 = 0;
    //  double b2 = -alpha;
    double a0 = 1 + alpha;
    double a1 = -2*cos(omega);
    double a2 = 1 - alpha;
    
    double tmp = 1 / a0;
    
    c->gain = b0 * tmp;
    c->fb1 = a1 * tmp;
    c->fb2 = a2 * tmp;
  }
}

void bandpassstate_init(struct bandpassstate* s) {
  s->w1 = 0;
  s->w2 = 0;
}

double bandpass_tick(struct bandpassstate* s, const struct bandpasscoeffs* c, double in) {
  double w1 = s->w1;
  double w2 = s->w2;
  double w0 = in * c->gain - w1 * c->fb1 - w2 * c->fb2;
  double out = w0 - w2;
  
  s->w2 = w1; s->w1 = w0;

  return out;
}


