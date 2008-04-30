#include "moogfilter.h"
#include "fasttanh.h"

void moogfilter_init(struct moogfilter* f) {
  f->p0 = 0;
  f->p1 = 0;
  f->p2 = 0;
  f->p3 = 0;
  f->p32 = 0;
  f->p33 = 0;
  f->p34 = 0;
}

double moogfilter_tick(struct moogfilter* f, double in, double omega, double reso) {
  if (omega > 1.0)
    omega = 1.0;
  else if (omega < 0)
    omega = 0;
  double b = omega;
  double k = reso*4;

  /*
  if (k > 3.999)
    k = 3.999;
  else if (k < -0.999)
    k = -0.999;
  */

  double p0=f->p0;
  double p1=f->p1;
  double p2=f->p2;
  double p3=f->p3;
  double p32=f->p32;
  double p33=f->p33;
  double p34=f->p34;
  
  // Coefficients optimized using differential evolution
  // to make feedback gain 4.0 correspond closely to the
  // border of instability, for all values of omega.
    double out = p3 * 0.360891 + p32 * 0.417290 + p33 * 0.177896 + p34 * 0.0439725;
  //double out = p3 * 0.360326 + p32 * 0.417718 + p33 * 0.177568 + p34 * 0.0445753;
  p34 = p33;
  p33 = p32;
  p32 = p3;

  p0 += (fasttanh(in - k*out) - fasttanh(p0))*b;
  p1 += (fasttanh(p0)-fasttanh(p1))*b;
  p2 += (fasttanh(p1)-fasttanh(p2))*b;
  p3 += (fasttanh(p2)-fasttanh(p3))*b;

  f->p0 = p0;
  f->p1 = p1;
  f->p2 = p2;
  f->p3 = p3;
  f->p32 = p32;
  f->p33 = p33;
  f->p34 = p34;

  return out;
}
