#include <stdio.h>

#define PAT_LINES 64
#define PAT_TRACKS 4

struct event {
#define CMD_NOP 0
#define CMD_NOTE_OFF 1
#define CMD_NOTE_ON 2
  uint8_t cmd;
  uint8_t octave;
  uint8_t degree;
};

typedef struct event pattern[PAT_LINES][PAT_TRACKS];

#define SONG_PATTERNS 255
#define MAX_ORDER_LENGTH 256
#define END_OF_ORDER 255

struct song {
  uint8_t octave_divisions;
  uint8_t order[256];
  pattern patterns[SONG_PATTERNS];
};

void song_init(struct song* song) {
  memset(song,0,sizeof(*song));
  for(int i=0;i<MAX_ORDER_LENGTH;i++)
    song->order[i] = END_OF_ORDER;
  song->order[0] = 0;
  song->octave_divisions = 53;
}

int song_save(struct song* song, const char* filename) {
  FILE* f = fopen(filename,"wb");
  if (!f)
    return 1;
  fwrite(song,1,sizeof(*song),f);
  fclose(f);
  return 0;
}

int song_load(struct song* song, const char* filename) {
  FILE* f = fopen(filename,"rb");
  if (!f)
    return 1;
  fread(song,1,sizeof(*song),f);
  fclose(f);
  return 0;
}

void song_finalize(struct song* song) {
}


int song_order_length(struct song* song) {
  int order_length = 0;
  while(order_length < MAX_ORDER_LENGTH && song->order[order_length] != END_OF_ORDER)
    order_length++;
  return order_length;
}

int song_pattern_line_is_empty(struct song* song, int pattern, int line) {
  for(int i=0;i<PAT_TRACKS;i++) {
    if (song->patterns[pattern][line][i].cmd != CMD_NOP)
      return 0;
  }
  return 1;
}

int song_pattern_is_empty(struct song* song, int pattern) {
  for(int i=0;i<PAT_LINES;i++) {
    if (!song_pattern_line_is_empty(song,pattern,i))
      return 0;
  }
  return 1;
}

int song_first_empty_pattern(struct song* song) {
  for(int i=0;i<SONG_PATTERNS;i++) {
    if (song_pattern_is_empty(song, i))
      return i;
  }
  return -1;
}

void song_copy_pattern(struct song* song, int from, int to) {
  memcpy(&song->patterns[to],&song->patterns[from],sizeof(pattern));
}
