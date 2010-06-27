#include <ncurses.h>

#define MODE_EDO 0
#define MODE_JI 1

struct editor {
  const char* filename;
  struct song* song;
  struct player* player;
  WINDOW* win;
  struct songcursor cursor;
  short pat_track;
  int table[7];
  int numer;
  int denom;
  int tuning_mode;
};

void editor_init(struct editor* editor,const char* filename,struct song* song,struct player* player) {
  memset(editor,0,sizeof(*editor));
  songcursor_init(&editor->cursor);
  editor->filename = filename;
  editor->song = song;
  editor->player = player;

  initscr();
  keypad(stdscr,TRUE);
  editor->win = stdscr;
  wrefresh(editor->win);

  editor->table[0] = 0;
  editor->table[1] = 9;
  editor->table[2] = 17;
  editor->table[3] = 22;
  editor->table[4] = 31;
  editor->table[5] = 39;
  editor->table[6] = 48;
  editor->numer = 1;
  editor->denom = 1;
  editor->tuning_mode = MODE_JI;
}

void editor_redraw(struct editor* editor) {
  werase(editor->win);

  int num_cols,num_rows;
  getmaxyx(editor->win,num_rows,num_cols);
  int middle_line = num_rows / 2;
  struct song* song = editor->song;
  int order_pos = songcursor_order_pos(&editor->cursor);
  for(int i = 0; song->order[i] != END_OF_ORDER; i++) {
    int screen_line = middle_line + i - order_pos;
    wmove(editor->win,screen_line,0);
    char buf[3];
    snprintf(buf,3,"%02x",song->order[i]);
    wprintw(editor->win,buf);
    if (i == order_pos) {
      wprintw(editor->win,"*");
    }
  }
  int pat_line = songcursor_pattern_line(&editor->cursor);

  int pat = songcursor_pattern(&editor->cursor,editor->song);
  for(int l = 0; l < PAT_LINES; l++) {
    int screen_line = middle_line + l - pat_line;
    if (screen_line < 0 || screen_line >= num_rows)
      continue;
    for(int t = 0; t < PAT_TRACKS; t++) {
      wmove(editor->win,screen_line,4 + t * 8);
      struct event event = song->patterns[pat][l][t];
      switch(event.cmd) {
      case CMD_NOP: wprintw(editor->win,"..."); break;
      case CMD_NOTE_OFF: wprintw(editor->win,"off"); break;
      case CMD_NOTE_ON:
        {
          char buffer[9];
          snprintf(buffer,9,"%i.%i",event.octave,event.degree);
          wprintw(editor->win,buffer);
          break;
        }
      case CMD_JI_NOTE_ON:
	{
	  char buffer[9];
	  snprintf(buffer,9,"%i/%i",event.octave+1,event.degree+1);
          wprintw(editor->win,buffer);
	  break;
	}
      default: wprintw(editor->win,"???"); break;
      }
    }
  }
  wmove(editor->win,0,4 + 4 * 8);

  struct player* player = editor->player;
  struct songcursor const* player_cursor = &player->cursor;
  char buffer[20];
  snprintf(buffer,20,"%02x %02x %02x",songcursor_order_pos(player_cursor),
	   songcursor_pattern(player_cursor,song),songcursor_pattern_line(player_cursor));
  wprintw(editor->win,buffer);


  wmove(editor->win,middle_line,4 + editor->pat_track * 8);

  wrefresh(editor->win);
}

int editor_current_pattern(struct editor* editor) {
  return songcursor_pattern(&editor->cursor,editor->song);
}

struct event* editor_current_event_ptr(struct editor* editor) {
  struct event* line = song_line(editor->song,&editor->cursor);
  return &line[editor->pat_track];
}

void editor_move_order_pos(struct editor* editor,int delta) {
  songcursor_move_order_pos(&editor->cursor,editor->song,delta);
}

void editor_move_pat_line(struct editor* editor,int delta) {
  songcursor_move_pat_line(&editor->cursor,editor->song,delta);
}

void editor_enter_note_on(struct editor* editor,int octave,int diatonic) {
  struct event* event = editor_current_event_ptr(editor);
  event->cmd = CMD_NOTE_ON;
  event->octave = octave;
  event->degree = editor->table[diatonic];
  editor_move_pat_line(editor,1);
}

void editor_enter_ji_note_on(struct editor* editor) {
  struct event* event = editor_current_event_ptr(editor);
  event->cmd = CMD_JI_NOTE_ON;
  event->octave = editor->numer - 1;
  event->degree = editor->denom - 1;
  editor_move_pat_line(editor,1);
}


void editor_insert_order(struct editor* editor) {
  struct song* song = editor->song;
  song_insert_order(song,songcursor_order_pos(&editor->cursor));
  editor_move_order_pos(editor,1);
}

void editor_delete_order(struct editor* editor) {
  struct song* song = editor->song;
  song_delete_order(song,songcursor_order_pos(&editor->cursor));
  songcursor_normalize(&editor->cursor,song);
}

void editor_increment_order(struct editor* editor,int delta) {
  song_increment_order(editor->song,songcursor_order_pos(&editor->cursor),delta);
}

void editor_transpose(struct editor* editor,int delta) {
  struct song* song = editor->song;
  struct event* event = editor_current_event_ptr(editor);
  int degree = event->degree + delta;
  int octave = event->octave;
  while (degree < 0) {
    degree += song->octave_divisions;
    octave -= 1;
  }
  while (degree >= song->octave_divisions) {
    degree -= song->octave_divisions;
    octave += 1;
  }
  if (octave < 0)
    octave = 0;
  if (octave > 7)
    octave = 7;
  event->octave = octave;
  event->degree = degree;
}

void editor_uniquify_pattern(struct editor* editor) {
  song_uniquify_pattern_at_order_pos(editor->song,songcursor_order_pos(&editor->cursor));
}

void editor_grab_note_degree(struct editor* editor) {
  struct event* event = editor_current_event_ptr(editor);
  if (event->cmd != CMD_NOTE_ON)
    return;
  int ch = getch();
  int diatonic = -1;
  switch(ch) {
  case '1': diatonic = 0; break;
  case '2': diatonic = 1; break;
  case '3': diatonic = 2; break;
  case '4': diatonic = 3; break;
  case '5': diatonic = 4; break;
  case '6': diatonic = 5; break;
  case '7': diatonic = 6; break;
  case 'q': diatonic = 0; break;
  case 'w': diatonic = 1; break;
  case 'e': diatonic = 2; break;
  case 'r': diatonic = 3; break;
  case 't': diatonic = 4; break;
  case 'y': diatonic = 5; break;
  case 'u': diatonic = 6; break;
  case 'a': diatonic = 0; break;
  case 's': diatonic = 1; break;
  case 'd': diatonic = 2; break;
  case 'f': diatonic = 3; break;
  case 'g': diatonic = 4; break;
  case 'h': diatonic = 5; break;
  case 'j': diatonic = 6; break;
  case 'z': diatonic = 0; break;
  case 'x': diatonic = 1; break;
  case 'c': diatonic = 2; break;
  case 'v': diatonic = 3; break;
  case 'b': diatonic = 4; break;
  case 'n': diatonic = 5; break;
  case 'm': diatonic = 6; break;
  }

  if (diatonic == -1)
    return;

  editor->table[diatonic] = event->degree;
}

int editor_handle_key(struct editor* editor) {
  int ch = getch();

  switch(editor->tuning_mode) {
  case MODE_JI: 
    switch(ch) {
    case 'q': editor->numer = 1; editor_enter_ji_note_on(editor); return 0;
    case 'w': editor->numer = 2; editor_enter_ji_note_on(editor); return 0;
    case 'e': editor->numer = 3; editor_enter_ji_note_on(editor); return 0;
    case 'r': editor->numer = 4; editor_enter_ji_note_on(editor); return 0;
    case 't': editor->numer = 5; editor_enter_ji_note_on(editor); return 0;
    case 'y': editor->numer = 6; editor_enter_ji_note_on(editor); return 0;
    case 'u': editor->numer = 7; editor_enter_ji_note_on(editor); return 0;
    case 'i': editor->numer = 8; editor_enter_ji_note_on(editor); return 0;
    case 'a': editor->denom = 1; editor_enter_ji_note_on(editor); return 0;
    case 's': editor->denom = 2; editor_enter_ji_note_on(editor); return 0;
    case 'd': editor->denom = 3; editor_enter_ji_note_on(editor); return 0;
    case 'f': editor->denom = 4; editor_enter_ji_note_on(editor); return 0;
    case 'g': editor->denom = 5; editor_enter_ji_note_on(editor); return 0;
    case 'h': editor->denom = 6; editor_enter_ji_note_on(editor); return 0;
    case 'j': editor->denom = 7; editor_enter_ji_note_on(editor); return 0;
    case 'k': editor->denom = 8; editor_enter_ji_note_on(editor); return 0;
    }
    break;
  case MODE_EDO:
    switch(ch) {
    case '1': editor_enter_note_on(editor,6,0); return 0;
    case '2': editor_enter_note_on(editor,6,1); return 0;
    case '3': editor_enter_note_on(editor,6,2); return 0;
    case '4': editor_enter_note_on(editor,6,3); return 0;
    case '5': editor_enter_note_on(editor,6,4); return 0;
    case '6': editor_enter_note_on(editor,6,5); return 0;
    case '7': editor_enter_note_on(editor,6,6); return 0;
    case 'q': editor_enter_note_on(editor,5,0); return 0;
    case 'w': editor_enter_note_on(editor,5,1); return 0;
    case 'e': editor_enter_note_on(editor,5,2); return 0;
    case 'r': editor_enter_note_on(editor,5,3); return 0;
    case 't': editor_enter_note_on(editor,5,4); return 0;
    case 'y': editor_enter_note_on(editor,5,5); return 0;
    case 'u': editor_enter_note_on(editor,5,6); return 0;
    case 'a': editor_enter_note_on(editor,4,0); return 0;
    case 's': editor_enter_note_on(editor,4,1); return 0;
    case 'd': editor_enter_note_on(editor,4,2); return 0;
    case 'f': editor_enter_note_on(editor,4,3); return 0;
    case 'g': editor_enter_note_on(editor,4,4); return 0;
    case 'h': editor_enter_note_on(editor,4,5); return 0;
    case 'j': editor_enter_note_on(editor,4,6); return 0;
    case 'z': editor_enter_note_on(editor,3,0); return 0;
    case 'x': editor_enter_note_on(editor,3,1); return 0;
    case 'c': editor_enter_note_on(editor,3,2); return 0;
    case 'v': editor_enter_note_on(editor,3,3); return 0;
    case 'b': editor_enter_note_on(editor,3,4); return 0;
    case 'n': editor_enter_note_on(editor,3,5); return 0;
    case 'm': editor_enter_note_on(editor,3,6); return 0;
    }
    break;
  }

  switch(ch) {
  case 27: return 1;
  case KEY_UP: editor_move_pat_line(editor,-1); break;
  case KEY_DOWN: editor_move_pat_line(editor,1); break;
  case KEY_PPAGE: editor_move_pat_line(editor,-16); break;
  case KEY_NPAGE: editor_move_pat_line(editor,16); break;
  case KEY_BTAB:
  case KEY_LEFT:
    editor->pat_track = wrap(editor->pat_track - 1,PAT_TRACKS);
    break;
  case '\t':
  case KEY_RIGHT:
    editor->pat_track = wrap(editor->pat_track + 1,PAT_TRACKS);
    break;
  case '`':
  case ' ':
    editor_current_event_ptr(editor)->cmd = CMD_NOTE_OFF;
    editor_move_pat_line(editor,1);
    break;
  case KEY_DC:
    editor_current_event_ptr(editor)->cmd = CMD_NOP;
    editor_move_pat_line(editor,1);
    break;
  case '{': editor_delete_order(editor);  break;
  case '}': editor_insert_order(editor);  break;
  case '[': editor_increment_order(editor,-1); break;
  case ']': editor_increment_order(editor,1); break;
  case '"': editor_uniquify_pattern(editor); break;
  case '8': editor_transpose(editor,-1); break;
  case '9': editor_transpose(editor,1); break;
  case '!': editor_grab_note_degree(editor); break;
  case '#': editor->tuning_mode = MODE_EDO; break;
  case '%': editor->tuning_mode = MODE_JI; break;
  default:
    if (ch == KEY_F(2)) {
      song_save(editor->song,editor->filename);
      break;
    }
    if (ch == KEY_F(3)) {
      song_load(editor->song,editor->filename);
      break;
    }
    if (ch == KEY_F(5)) {
      player_play(editor->player);
      break;
    }
    if (ch == KEY_F(6)) {
      struct songcursor pattern_start;
      songcursor_init(&pattern_start);
      songcursor_set_order_pos(&pattern_start,songcursor_order_pos(&editor->cursor));
      player_play_from(editor->player,&pattern_start);
      songcursor_finalize(&pattern_start);
      break;
    }
    if (ch == KEY_F(7)) {
      player_play_from(editor->player,&editor->cursor);
      break;
    }
    if (ch == KEY_F(8)) {
      player_stop(editor->player);
      break;
    }

    printf("Unhandled character: %i\n",ch);
    getch();
  }
  return 0;
}

void editor_run(struct editor* editor) {
  while(1) {
    editor_redraw(editor);
    if (editor_handle_key(editor) == 1)
      break;
  }
}

void editor_finalize(struct editor* editor) {
  endwin();
  songcursor_finalize(&editor->cursor);
}
