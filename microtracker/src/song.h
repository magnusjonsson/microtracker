#include <stdint.h>
#include <stdio.h>
#include <memory.h>

#define PAT_LINES 64
#define PAT_TRACKS 4

struct event {
#define CMD_NOP 0
#define CMD_NOTE_OFF 1
#define CMD_NOTE_ON 2
#define CMD_JI_NOTE_ON 3
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
  int len = fwrite(song,1,sizeof(*song),f);
  fclose(f);
  if (len != sizeof(*song)) {
    fprintf(stderr, "Error: Could not save song! The song may not be loadable.\n");
    return 1;
  }
  return 0;
}

int song_load(struct song* song, const char* filename) {
  FILE* f = fopen(filename,"rb");
  if (!f)
    return 1;
  int len = fread(song,1,sizeof(*song),f);
  fclose(f);
  if (len != sizeof(*song)) {
    fprintf(stderr, "Error: Could not read whole song! The song may not be loaded correctly. len = %i\n", len);
    return 1;
  }
  return 0;
}

void song_finalize(struct song* song) {
}


int song_order_length(struct song const* song) {
  int order_length = 0;
  while(order_length < MAX_ORDER_LENGTH && song->order[order_length] != END_OF_ORDER)
    order_length++;
  return order_length;
}

int song_pattern_line_is_empty(struct song const* song, int pattern, int line) {
  for(int i=0;i<PAT_TRACKS;i++) {
    if (song->patterns[pattern][line][i].cmd != CMD_NOP)
      return 0;
  }
  return 1;
}

int song_pattern_is_empty(struct song const* song, int pattern) {
  for(int i=0;i<PAT_LINES;i++) {
    if (!song_pattern_line_is_empty(song,pattern,i))
      return 0;
  }
  return 1;
}

int song_first_empty_pattern(struct song const* song) {
  for(int i=0;i<SONG_PATTERNS;i++) {
    if (song_pattern_is_empty(song, i))
      return i;
  }
  return -1;
}

void song_copy_pattern(struct song* song, int from, int to) {
  memcpy(&song->patterns[to],&song->patterns[from],sizeof(pattern));
}

void song_insert_order(struct song* song, int order_pos) {
  uint8_t* order = song->order;
  uint8_t prev = order[order_pos];
  for(int i=order_pos; i < MAX_ORDER_LENGTH;i++) {
    uint8_t curr = order[i];
    song->order[i] = prev;
    prev = curr;
  }
}

void song_delete_order(struct song* song, int order_pos) {
  uint8_t* order = song->order;
  uint8_t prev = END_OF_ORDER;
  for(int i=MAX_ORDER_LENGTH-1; i >= order_pos; i--) {
    uint8_t curr = order[i];
    order[i] = prev;
    prev = curr;
  }
}
void song_uniquify_pattern_at_order_pos(struct song* song, int order_pos) {
  uint8_t* order = song->order;
  uint8_t curr_pattern = order[order_pos];
  uint8_t free_pattern = song_first_empty_pattern(song);
  if (free_pattern < 0)
    return;
  song_copy_pattern(song,curr_pattern,free_pattern);
  order[order_pos] = free_pattern;
};

struct songcursor {
  short order_pos;
  short pat_line;
};

void songcursor_init(struct songcursor* cursor) {
  cursor->order_pos = 0;
  cursor->pat_line = 0;
}

void songcursor_finalize(struct songcursor* cursor) {
  songcursor_init(cursor);
}

void songcursor_move_order_pos(struct songcursor* cursor, struct song const* song, int delta) {
  int order_length = song_order_length(song);
  int order_pos = cursor->order_pos + delta;
  while (order_pos < 0) {
    order_pos += order_length;
  }
  while (order_pos >= order_length) {
    order_pos -= order_length;
  }
  cursor->order_pos = order_pos;
}

void songcursor_move_pat_line(struct songcursor* cursor, struct song const* song, int delta) {
  int line = cursor->pat_line + delta;
  while(line >= PAT_LINES) {
    line -= PAT_LINES;
    songcursor_move_order_pos(cursor,song,1);
  }
  while(line < 0) {
    line += PAT_LINES;
    songcursor_move_order_pos(cursor,song,-1);
  }
  cursor->pat_line = line;
}

int wrap(int line, int modulo) {
  while(line < 0)
    line += modulo;
  while(line >= modulo)
    line -= modulo;
  return line;
}

void song_increment_order(struct song* song, int order_pos, int delta) {
  uint8_t* order = song->order;
  order[order_pos] = wrap(order[order_pos] + delta, END_OF_ORDER);
}

void songcursor_normalize(struct songcursor* cursor, struct song const* song) {
  songcursor_move_order_pos(cursor, song, 0);
  songcursor_move_pat_line(cursor, song, 0);
}

void songcursor_advance_pattern(struct songcursor* cursor, struct song const* song) {
    if (cursor->order_pos == 255 || song->order[cursor->order_pos+1] == END_OF_ORDER) {
      cursor->order_pos = 0;
    }
    else {
      ++cursor->order_pos;
    }
}

void songcursor_advance(struct songcursor* cursor, struct song const* song) {
  if (cursor->pat_line == PAT_LINES - 1) {
    cursor->pat_line = 0;
    songcursor_advance_pattern(cursor, song);
  }
  else {
    cursor->pat_line++;
  }
}

void songcursor_copytofrom(struct songcursor* to, struct songcursor const* from) {
  *to = *from;
}

int songcursor_is_at_beginning_of_song(struct songcursor const* cursor) {
  return cursor->order_pos == 0 && cursor->pat_line == 0;
}

void songcursor_set_order_pos(struct songcursor* cursor, int order_pos) {
  cursor->order_pos = order_pos;
}

int songcursor_order_pos(struct songcursor const* cursor) {
  return cursor->order_pos;
}

int songcursor_pattern_line(struct songcursor const* cursor) {
  return cursor->pat_line;
}

int songcursor_pattern(struct songcursor const* cursor, struct song const* song) {
  return song->order[cursor->order_pos];
}

struct event* song_line(struct song* song, struct songcursor const* cursor) {
  return song->patterns[song->order[cursor->order_pos]][cursor->pat_line];
}

struct event const* song_line_readonly(struct song const* song, struct songcursor const* cursor) {
  return song->patterns[song->order[cursor->order_pos]][cursor->pat_line];
}
