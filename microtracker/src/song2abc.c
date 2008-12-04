#include "song.h"

int main(int argc, char** argv) {
  const char* infile = argc >= 2 ? argv[1] : "untitled.song";
  const char* outfile = argc >= 3 ? argv[2] : "output.abc";
  struct song song;
  song_init(&song);
  song_load(&song, infile);
  
  song_finalize(&song);
}
