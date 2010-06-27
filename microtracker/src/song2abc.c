#include <stdlib.h>
#include <getopt.h>
#include <memory.h>
#include <stdio.h>
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

void emit_beam_separator() {
  printf(" ");
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

    if ((voice->note_or_rest_start_pos & 3) == 0) {
      emit_beam_separator();
    }
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
  }
}

void print_track(struct song* song, int track, const char* clef) {
  printf("V:%i", track+1);
  if (clef)
    printf(" clef=%s", clef);
  printf("\n");

  struct songcursor cursor;
  songcursor_init(&cursor);

  struct voice voice;
  voice_init(&voice);

  do {
    struct event const* event = &song_line(song, &cursor)[track];

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

    int pattern_line = songcursor_pattern_line(&cursor);
    if (pattern_line == 0) {
      printf("||\n");
    }
    else if ((pattern_line & 31) == 0) {
      printf("|\n");
    }
    else if ((pattern_line & 15) == 0) {
      printf("|");
    }
  } while(!songcursor_is_at_beginning_of_song(&cursor));

  voice_stop_last_note_or_rest(&voice);

  songcursor_finalize(&cursor);

  printf("%%\n");
}

struct options {
  const char* infile;
  const char* title;
  const char* composer;
  const char* voice_order;
  const char* notes;
  int num_clefs;
  const char* clefs[PAT_TRACKS];
};

int parse_options(int argc, char** argv, struct options* o) {
  while(1) {
    switch(getopt(argc, argv, "T:C:v:N:c:")) {
    case 'T':
      o->title = optarg;
      break;
    case 'C':
      o->composer = optarg;
      break;
    case 'v':
      o->voice_order = optarg;
      break;
    case 'N':
      o->notes = optarg;
      break;
    case 'c':
      if (o->num_clefs >= PAT_TRACKS) {
	fprintf(stderr, "Error: too many clefs\n");
	fflush(stderr);
	return 1;
      }
      o->clefs[o->num_clefs++] = optarg;
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
    .voice_order = NULL,
    .notes = NULL,
    .num_clefs = 0,
    .clefs = { NULL, NULL, NULL, NULL }
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
	   //	   "%%%%continueall\n"
	   "%%%%maxshrink 0.8\n"
	   "%%%%microabc: equaltemp: 53\n"
	   "%%%%measurefirst 0\n"
	   "%%%%barnumbers 1\n"
	   "\n"
	   "X:1\n");
    if (options.title) {
      printf("T:%s\n", options.title);
    }
    if (options.composer) {
      printf("C:%s\n", options.composer);
    }
    if (options.notes) {
      printf("C:%s\n", options.notes);
    }
    printf("L:1/4\n"
	   "M:4/4\n"
	   "K:C\n"
	   "%%\n");
    if (options.voice_order) {
      printf("%%%%score %s\n", options.voice_order);
    }
    for(int track = 0; track < PAT_TRACKS; track++) {
      print_track(&song, track, options.clefs[track]);
    }
  }
  song_finalize(&song);
}
