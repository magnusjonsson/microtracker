struct moogfilter {
  double p0;
  double p1;
  double p2;
  double p3;
  double p32;
  double p33;
  double p34;
};

void moogfilter_init(struct moogfilter* f);
double moogfilter_tick(struct moogfilter* f, double in, double omega, double reso);
