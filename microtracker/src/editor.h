#include <ncurses.h>
#include "song.h"
#include "util.h"

#define MODE_EDO 0
#define MODE_JI 1
#define MODE_31_EDO 2

struct editor {
  const char* filename;
  struct song* song;
  struct player* player;
  WINDOW* win;
  struct songcursor cursor;
  short pat_track;
  int numer;
  int denom;
  int tuning_mode;
};

void editor_init(struct editor* editor,const char* filename,struct song* song,struct player* player);
void editor_run(struct editor* editor);
void editor_finalize(struct editor* editor);

void editor_redraw(struct editor* editor);
int editor_get_current_pattern(struct editor* editor);
struct event* editor_get_current_event_ptr(struct editor* editor);
void editor_move_order_pos(struct editor* editor,int delta);
void editor_move_pat_line(struct editor* editor,int delta);
void editor_enter_note_on(struct editor* editor,int octave,int diatonic);
void editor_enter_ji_note_on(struct editor* editor);
void editor_enter_note_off(struct editor* editor);
void editor_enter_nop(struct editor* editor);
void editor_insert_order(struct editor* editor);
void editor_delete_order(struct editor* editor);
void editor_increment_order(struct editor* editor,int delta);
void editor_transpose(struct editor* editor,int delta);
void editor_uniquify_pattern(struct editor* editor);
void editor_grab_note_degree(struct editor* editor);

