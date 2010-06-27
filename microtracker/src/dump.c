#include <stdint.h>
#include <memory.h>

#define SAMPLERATE 48000

#include "song.h"
#include "player.h"
#include "wavwriter.h"
#include "f2s.h"

int main(int argc, char** argv) {
  const char* infile = argc >= 2 ? argv[1] : "untitled.song";
  const char* outfile = argc >= 3 ? argv[2] : "dump.wav";
  struct song song;
  struct player player;
  struct synthdesc const* synthdesc = finddesc("simplesynth");
  struct synthdesc const* effectdesc = finddesc("reverb2");
  song_init(&song);
  song_load(&song,infile);
  if (!player_init(&player,&song,synthdesc,effectdesc,SAMPLERATE)) {
    player_play(&player);
    FILE* f = wavwriter_begin(outfile,SAMPLERATE);
    if (!f) {
      fprintf(stderr,"Error opening file for writing\n");
    }
    else {
      float left[256];
      float right[256];
      short out[512];

      do {
        int length = player_generate_some_audio(&player,left,right,256);
        float_to_short_stereo(left,right,out,length);
        for(int i=0;i<length*2;i++) {
          fputint16(out[i],f);
        }
      } while(!player_is_at_beginning_of_song(&player));
      wavwriter_end(f);
    }
    player_finalize(&player);
  }
  song_finalize(&song);
  return 0;
}
