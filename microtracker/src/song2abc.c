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

void emit_pitch(int octave, int degree) {
  printf("A");
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
  printf("Z");
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
    printf("|");
  }
}

void print_track(struct song const* song, int track) {
  printf("V%i:\n", track+1);

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

  printf("\n");
}

int main(int argc, char** argv) {
  const char* infile = argc >= 2 ? argv[1] : "untitled.song";
  struct song song;
  song_init(&song);
  if (!song_load(&song, infile)) {
    for(int track = 0; track < PAT_TRACKS; track++) {
      print_track(&song, track);
    }
  }
  song_finalize(&song);
}
