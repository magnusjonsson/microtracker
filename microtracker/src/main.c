#include <stdint.h>
#include <memory.h>
#include <unistd.h>
#include <getopt.h>

#include "song.h"
#include "player.h"
#include "audio_io.h"
#include "editor.h"

struct options {
  int output_device;
  const char* filename;
  const char* synth;
  const char* effect;
};

int parse_options(int argc, char** argv, struct options* o) {
  o->output_device = -1;
  o->filename = "untitled.song";
  o->synth = "simplesynth";
  o->effect = "reverb4";
  while(1) {
    switch(getopt(argc,argv,"O:s:e:")) {
    case 'O':
      o->output_device = atoi(optarg);
      break;
    case 's':
      o->synth = optarg;
      break;
    case 'e':
      o->effect = optarg;
      break;
    case -1:
      if(optind < argc)
	o->filename = argv[optind];
      return 0;
    default:
      fprintf(stderr,"Error: Bad command line argument\n");
      fflush(stderr);
      return 1;
    }
  }
}

int run(struct options const* options) {
  int error_code = 0;
  struct song song;
  struct player player;
  struct audio_io audio_io;
  struct editor editor;
  struct synthdesc const* synthdesc = finddesc(options->synth);
  struct synthdesc const* effectdesc = finddesc(options->effect);
  song_init(&song);
  song_load(&song,options->filename);
  if (!(error_code = player_init(&player,&song,synthdesc,effectdesc))) {
    if (!(error_code = audio_io_init(&audio_io,&player,options->output_device))) {
      editor_init(&editor,options->filename,&song,&player);
      editor_run(&editor);
      editor_finalize(&editor);
      audio_io_finalize(&audio_io);
    }
    player_finalize(&player);
  }
  song_finalize(&song);
  return error_code;
}

int main(int argc, char** argv) {
  struct options options;
  return parse_options(argc,argv,&options) || run(&options);
}
