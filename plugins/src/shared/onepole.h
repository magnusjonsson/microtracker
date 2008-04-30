struct onepole {
  double state;
  double coeff;
};
void onepole_init(struct onepole* o, double coeff);
void onepole_finalize(struct onepole* o);
void onepole_setcoeff(struct onepole* o, double coeff);
double onepole_tick(struct onepole* o, double input);
double onepole_coeff_for_time_variance(double time_variance);
double onepole_coeff_for_time_spread(double time_spread);
double onepole_coeff_for_omega(double omega);
