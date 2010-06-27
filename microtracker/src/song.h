#include <stdint.h>

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

void song_init(struct song* song);
int song_save(struct song* song, const char* filename);
int song_load(struct song* song, const char* filename);
void song_finalize(struct song* song);
int song_order_length(struct song const* song);
int song_pattern_line_is_empty(struct song const* song, int pattern, int line);
int song_pattern_is_empty(struct song const* song, int pattern);
int song_first_empty_pattern(struct song const* song);
void song_copy_pattern(struct song* song, int from, int to);
void song_insert_order(struct song* song, int order_pos);
void song_delete_order(struct song* song, int order_pos);
void song_uniquify_pattern_at_order_pos(struct song* song, int order_pos);

struct songcursor {
  short order_pos;
  short pat_line;
};

void songcursor_init(struct songcursor* cursor);
void songcursor_finalize(struct songcursor* cursor);
void songcursor_move_order_pos(struct songcursor* cursor, struct song const* song, int delta);
void songcursor_move_pat_line(struct songcursor* cursor, struct song const* song, int delta);
void song_increment_order(struct song* song, int order_pos, int delta);
void songcursor_normalize(struct songcursor* cursor, struct song const* song);
void songcursor_advance_pattern(struct songcursor* cursor, struct song const* song);
void songcursor_advance(struct songcursor* cursor, struct song const* song);
void songcursor_copytofrom(struct songcursor* to, struct songcursor const* from);
int songcursor_is_at_beginning_of_song(struct songcursor const* cursor);
void songcursor_set_order_pos(struct songcursor* cursor, int order_pos);
int songcursor_order_pos(struct songcursor const* cursor);
int songcursor_pattern_line(struct songcursor const* cursor);
int songcursor_pattern(struct songcursor const* cursor, struct song const* song);

struct event* song_line(struct song* song, struct songcursor const* cursor);
struct event const* song_line_readonly(struct song const* song, struct songcursor const* cursor);
