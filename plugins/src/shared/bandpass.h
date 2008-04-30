#ifndef __Magnus_bandpass_h
#define __Magnus_bandpass_h

struct bandpasscoeffs {
  double gain;
  double fb1;
  double fb2;
};
struct bandpassstate {
  double w1,w2;
};

void bandpasscoeffs_identity(struct bandpasscoeffs* c);
void bandpasscoeffs_from_omega_and_damping(struct bandpasscoeffs* c, double omega, double damping);
void bandpasscoeffs_from_omega_and_q(struct bandpasscoeffs* c, double omega, double q);
void bandpassstate_init(struct bandpassstate* s);
double bandpass_tick(struct bandpassstate* s, const struct bandpasscoeffs* c, double in);

#endif
