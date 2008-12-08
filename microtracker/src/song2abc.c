#include <stdlib.h>
#include <getopt.h>
#include "song.h"

struct voice {
  char is_on;
  char is_tie;
  int octave;
  int degree;
  int note_or_rest_start_pos;
  int current_pos;
};

void voice_init(struct voice* voice) {
  memset(voice, 0, sizeof(*voice));
}

void voice_nop(struct voice* voice) {
}

int gcd(int a, int b) {
  return b == 0 ? a : gcd(b, a%b);
}

void reduce_ratio(int* numer, int* denom) {
  while(1) {
    int g = gcd(*numer, *denom);
    if (g <= 1)
      return;
    *numer /= g;
    *denom /= g;
  }
}

const char* pitch_class_name(int degree) {
  switch(degree) {
  case 0: return "C";
  case 7: return "\\\\!D";
  case 8: return "D\\";
  case 9: return "D";
  case 14: return "Eb/";
  case 17: return "E\\";
  case 18: return "E";
  case 21: return "F\\";
  case 22: return "F";
  case 27: return "F#";
  case 31: return "G";
  case 34: return "G#\\\\!";
  case 36: return "Ab/";
  case 39: return "A\\";
  case 40: return "A";
  case 48: return "B\\";
  case 49: return "B";
  default:
    fprintf(stderr, "error: don't know the name for pitch degree %i\n", degree);
    exit(1);
  }
}

void emit_pitch(int octave, int degree) {
  printf("[%s%i]", pitch_class_name(degree), octave);
}

void emit_duration(int duration) {
  int numer = duration;
  int denom = 4;
  reduce_ratio(&numer, &denom);
  if (numer != 1)
    printf("%i", numer);
  if (denom != 1) {
    printf("/%i", denom);
  }
}

void emit_note(int octave, int degree, int duration) {
  emit_pitch(octave, degree);
  emit_duration(duration);
  printf("`");
}

void emit_rest(int duration) {
  printf("z");
  emit_duration(duration);
  printf("`");
}

int calculate_subduration(int start, int stop) {
  int maxduration = stop-start;
  int duration = 1;
  while (2*duration <= maxduration && start % (2*duration) == 0) {
    duration *= 2;
  }
  return duration;
}

void voice_stop_last_note_or_rest(struct voice* voice) {
  while(voice->note_or_rest_start_pos < voice->current_pos) {
    int subduration = calculate_subduration(voice->note_or_rest_start_pos, voice->current_pos);
    if (voice->is_on && voice->is_tie) {
      printf("-");
      voice->is_tie = 0;
    }
    if (voice->is_on)
      emit_note(voice->octave, voice->degree, subduration);
    else
      emit_rest(subduration);
    voice->note_or_rest_start_pos += subduration;
    voice->is_tie = 1;
  }
}

void voice_note_off(struct voice* voice) {
  if (voice->is_on) {
    voice_stop_last_note_or_rest(voice);
    voice->is_on = 0;
    voice->is_tie = 0;
  }
}

void voice_note_on(struct voice* voice, int octave, int degree) {
  voice_stop_last_note_or_rest(voice);
  voice->is_on = 1;
  voice->is_tie = 0;
  voice->octave = octave;
  voice->degree = degree;
  voice->note_or_rest_start_pos = voice->current_pos;
}

void voice_advance(struct voice* voice) {
  voice->current_pos++;
  if (voice->current_pos == 16) {
    voice_stop_last_note_or_rest(voice);
    voice->current_pos = 0;
    voice->note_or_rest_start_pos = 0;
    printf("|\n");
  }
}

void print_track(struct song const* song, int track) {
  printf("V:%i\n", track+1);

  struct songcursor cursor;
  songcursor_init(&cursor);

  struct voice voice;
  voice_init(&voice);

  do {
    struct event const* event = &song_line_readonly(song, &cursor)[track];

    switch(event->cmd) {
    case CMD_NOP:
      voice_nop(&voice);
      break;
    case CMD_NOTE_OFF:
      voice_note_off(&voice);
      break;
    case CMD_NOTE_ON:
      voice_note_on(&voice, event->octave, event->degree);
      break;
    }
    voice_advance(&voice);

    songcursor_advance(&cursor, song);
  } while(!songcursor_is_at_beginning_of_song(&cursor));

  voice_stop_last_note_or_rest(&voice);

  songcursor_finalize(&cursor);

  printf("%%\n");
}

struct options {
  const char* infile;
  const char* title;
  const char* composer;
};

int parse_options(int argc, char** argv, struct options* o) {
  while(1) {
    switch(getopt(argc, argv, "T:C:")) {
    case 'T':
      o->title = optarg;
      break;
    case 'C':
      o->composer = optarg;
      break;
    case -1:
      if (optind < argc)
	o->infile = argv[optind];
      return 0;
    default:
      fprintf(stderr, "Error: Bad command line argument\n");
      fflush(stderr);
      return 1;
    }
  }
}

int main(int argc, char** argv) {
  struct options options = {
    .infile = "untitled.song",
    .title = NULL,
    .composer = NULL,
  };
  if (parse_options(argc, argv, &options)) {
    return 1;
  }
  struct song song;
  song_init(&song);
  if (!song_load(&song, options.infile)) {
    printf("%%%%format sagittal.fmt\n"
	   "%%%%format sagittal-mixed.fmt\n"
	   "%%%%postscript sagmixed\n"
	   "%%%%microabc: procsag:1\n"
	   "%%%%continueall\n"
	   "%%%%maxshrink 1.0\n"
	   "%%%%microabc: equaltemp: 53\n"
	   "\n"
	   "X:1\n");
    if (options.title) {
      printf("T:%s\n", options.title);
    }
    if (options.composer) {
      printf("C:%s\n", options.composer);
    }
    printf("L:1/4\n"
	   "M:4/4\n"
	   "K:C\n"
	   "%%\n");
    for(int track = 0; track < PAT_TRACKS; track++) {
      print_track(&song, track);
    }
  }
  song_finalize(&song);
}
