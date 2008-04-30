struct fractionaldelay {
  float last_in;
  float delay;
};

void fractionaldelay_init(struct fractionaldelay* fd, float delay);
void fractionaldelay_finalize(struct fractionaldelay* fd);
float fractionaldelay_tick(struct fractionaldelay* fd, float in);
