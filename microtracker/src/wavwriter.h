#include <stdio.h>

void fputint32(int v, FILE* f);
void fputint16(int v, FILE* f);

FILE* wavwriter_begin(const char* filename, int samplerate);
void wavwriter_end(FILE* f);
