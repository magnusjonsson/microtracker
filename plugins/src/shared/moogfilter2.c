#include "moogfilter2.h"
#include "fasttanh.h"
#include <memory.h>
#include <math.h>

void moogfilter2_init(struct moogfilter2* f) {
  memset(f, 0, sizeof(*f));
}

double moogfilter2_tick(struct moogfilter2* f, double IN, double omega, double reso) {
  if (omega > 1.999)
    omega = 1.999;
  else if (omega < 0)
    omega = 0;

  double b = omega*0.5;
  double ib = 1-omega;
  double k = reso*4;

  double in=f->in;
  double y0=f->y0;
  double y1=f->y1;
  double y2=f->y2;
  double y3=f->y3;


  // double Z = in+IN - k*(y3+Y3);
  //  double X = tanh(Z);

  double Y0perX = b;
  double Y1perX = b*Y0perX;
  double Y2perX = b*Y1perX;
  double Y3perX = b*Y2perX;
  double ZperX = -k*Y3perX;

  double Y0const =           ib*y0;
  double Y1const = b*Y0const+ib*y1;
  double Y2const = b*Y1const+ib*y2;
  double Y3const = b*Y2const+ib*y3;
  double Zconst = 0.5*(in+IN - k*(y3+Y3const));

  // X = f(Zconst + ZperX*X)

  // if f = id:
  // x = Zconst / (1-ZperX)

  double X = Zconst / (1-ZperX);

  for(int i = 0; i < 8; i++) {
    X = tanh(Zconst + ZperX*X);
    
  }

  double Y0 = b*X       + ib*y0;
  double Y1 = b*(y0+Y0) + ib*y1;
  double Y2 = b*(y1+Y1) + ib*y2;
  double Y3 = b*(y2+Y2) + ib*y3;

  f->y0=Y0;
  f->y1=Y1;
  f->y2=Y2;
  f->y3=Y3;
  f->in=IN;

  return Y3;
}
