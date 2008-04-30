struct ms20filter {
  double state0;
  double state1;
};

void ms20filter_init(struct ms20filter* f);
double ms20filter_tick(struct ms20filter* s, double lp_in, double hp_in, double omega, double resonance /* max 1 */);
