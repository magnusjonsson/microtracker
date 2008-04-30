// Thiran Fractional Delay Allpass Filters
// http://ccrma.stanford.edu/~jos/Interpolation/Thiran_Allpass_Interpolators.html

// first order thiran filter
inline double thiran1_coeff(double delay) {
  int N = 1;
  double delta = delay - N;

  // this simplifies to the below
  //double a0 = 1 * (delta + 0)/(delta + 0) * (delta+1)/(delta+1);
  //double a1 = -1 * (delta + 0)/(delta + 1) * (delta+1)/(delta+2);

  //double a0 = 1;
  double a1 = - delta/(delta+2);
  return a1;
}

inline double thiran1_tick(double* state, double a1, double input) {  
  /*
           a_1 + a_0 * z^{-1}   F(z)
    H(z) = ------------------ = ----
           a_0 + a_1 * z^{-1}   G(z)

  */

  // run 1/G(z) filter
  double last_middle = *state;
  double middle = input - a1 * last_middle;
  *state = middle;
  // run F(z) filter
  double output = last_middle + middle * a1;
  return output;
}
