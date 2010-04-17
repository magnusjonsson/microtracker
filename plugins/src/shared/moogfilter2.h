struct moogfilter2 {
  double in;
  double y0;
  double y1;
  double y2;
  double y3;
};

void moogfilter2_init(struct moogfilter2* f);
double moogfilter2_tick(struct moogfilter2* f, double in, double omega, double reso);
