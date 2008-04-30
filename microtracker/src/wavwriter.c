#include "wavwriter.h"

void fputint32(int v, FILE* f) {
  int i;
  for(i=0;i<4;i++) {
    fputc(v&255,f);
    v >>= 8;
  }
}

void fputint16(int v, FILE* f) {
  int i;
  for(i=0;i<2;i++) {
    fputc(v&255,f);
    v >>= 8;
  }
}

FILE* wavwriter_begin(const char* filename, int samplerate) {
  FILE* f = fopen(filename,"wb");
  if (f) {
    //      header         |fmt_ chunk      #chn
    fputs("RIFF",f);
    fputint32(0,f); // total length of package to follow
    fputs("WAVE",f);
    // offs 12
    fputs("fmt ",f);
    fputint32(16,f); // length of format chunk
    fputint16(1,f); // always 1
    fputint16(2,f); // number of channels
    fputint32(samplerate,f);
    fputint32(samplerate*4,f); // bytes per second
    fputint16(4,f); // bytes per sample
    fputint16(16,f); // bits per sample
    // offs 36
    fputs("data",f);
    fputint32(0,f); // length of data to follow
  }
  return f;
}

void wavwriter_end(FILE* f) {
  int pos = ftell(f);
  fseek(f,40,SEEK_SET);
  fputint32(pos-44,f);
  fseek(f,4,SEEK_SET);
  fputint32(pos-8,f);
  fclose(f);
}
