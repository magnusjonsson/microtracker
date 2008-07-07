#include <stdlib.h> // rand

void float_to_short_stereo(float const* left,
                           float const* right,
                           short* out,
                           int length)
{
  for(int i=0; i<length; i++ ) {
    double l = left[i];
    double r = right[i];
    
    if(l > 1.0) l = 1.0;
    if(l < -1.0) l = -1.0;
    if(r > 1.0) r = 1.0;
    if(r < -1.0) r = -1.0;
    double noise = rand()*(1.0/RAND_MAX) - rand()*(1.0/RAND_MAX);
    int sl = 32768+l*32767 + noise;
    int sr = 32768+r*32767 + noise;
    out[2*i+0] = (short)(sl-32768);
    out[2*i+1] = (short)(sr-32768);
  }
}
