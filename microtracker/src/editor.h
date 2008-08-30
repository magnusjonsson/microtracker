#include <ncurses.h>

struct editor {
  const char* filename;
  struct song* song;
  struct player* player;
  WINDOW* win;
  short order_pos;
  short pat_line;
  short pat_track;
};

void editor_init(struct editor* editor, const char* filename, struct song* song, struct player* player) {
  memset(editor,0,sizeof(*editor));
  editor->filename = filename;
  editor->song = song;
  editor->player = player;

  initscr();
  keypad(stdscr,TRUE);
  editor->win = stdscr;
  wrefresh(editor->win);
}

void editor_redraw(struct editor* editor) {
  werase(editor->win);

  int num_cols, num_rows;
  getmaxyx(editor->win,num_rows,num_cols);
  int middle_line = num_rows / 2;
  struct song* song = editor->song;
  for(int i = 0; song->order[i] != END_OF_ORDER; i++) {
    int screen_line = middle_line + i - editor->order_pos;
    wmove(editor->win, screen_line, 0);
    char buf[3];
    snprintf(buf,3,"%02x",song->order[i]);
    wprintw(editor->win, buf);
    if (i == editor->order_pos) {
      wprintw(editor->win, "*");
    }
  }

  int pat = song->order[editor->order_pos];
  for(int l = 0; l < PAT_LINES; l++) {
    int screen_line = middle_line + l - editor->pat_line;
    if (screen_line < 0 || screen_line >= num_rows)
      continue;
    for(int t = 0; t < PAT_TRACKS; t++) {
      wmove(editor->win, screen_line, 4 + t * 8);
      struct event event = song->patterns[pat][l][t];
      switch(event.cmd) {
      case CMD_NOP: wprintw(editor->win,"..."); break;
      case CMD_NOTE_OFF: wprintw(editor->win, "off"); break;
      case CMD_NOTE_ON:
        {
          char buffer[9];
          snprintf(buffer,9,"%i.%i",event.octave,event.degree);
          wprintw(editor->win, buffer);
          break;
        }
      default: wprintw(editor->win,"???"); break;
      }
    }
  }
  wmove(editor->win,0,4 + 4 * 8);

  struct player* player = editor->player;
  char buffer[20];
  snprintf(buffer,20,"%02x %02x %02x", player->order_pos, song->order[player->order_pos], player->pat_line);
  wprintw(editor->win,buffer);


  wmove(editor->win,middle_line,4 + editor->pat_track * 8);

  wrefresh(editor->win);
}

int editor_current_pattern(struct editor* editor) {
  return editor->song->order[editor->order_pos];
}

struct event* editor_current_event_ptr(struct editor* editor) {
  return &editor->song->patterns[editor_current_pattern(editor)][editor->pat_line][editor->pat_track];
}

int wrap(int line, int modulo) {
  while(line < 0)
    line += modulo;
  while(line >= modulo)
    line -= modulo;
  return line;
}

void editor_move_order_pos(struct editor* editor, int delta) {
  int order_length = song_order_length(editor->song);
  editor->order_pos += delta;
  while (editor->order_pos < 0) {
    editor->order_pos += order_length;
  }
  while (editor->order_pos >= order_length) {
    editor->order_pos -= order_length;
  }
}

void editor_move_pat_line(struct editor* editor, int delta) {
  editor->pat_line += delta;
  while(editor->pat_line >= PAT_LINES) {
    editor->pat_line -= PAT_LINES;
    editor_move_order_pos(editor,1);
  }
  while(editor->pat_line < 0) {
    editor->pat_line += PAT_LINES;
    editor_move_order_pos(editor,-1);
  }
}

void editor_enter_note_on(struct editor* editor, int octave, int diatonic) {
  struct event* event = editor_current_event_ptr(editor);
  int table[7] = { 0, 9, 17, 22, 31, 39, 48 };
  event->cmd = CMD_NOTE_ON;
  event->octave = octave;
  event->degree = table[diatonic];
  editor_move_pat_line(editor,1);
}

void editor_insert_order(struct editor* editor) {
  struct song* song = editor->song;
  int prev = song->order[editor->order_pos];
  for(int i=editor->order_pos; i < MAX_ORDER_LENGTH;i++) {
    int curr = song->order[i];
    song->order[i] = prev;
    prev = curr;
  }
  editor_move_order_pos(editor,1);
}

void editor_delete_order(struct editor* editor) {
  struct song* song = editor->song;
  int prev = END_OF_ORDER;
  for(int i=MAX_ORDER_LENGTH-1; i >= editor->order_pos; i--) {
    int curr = song->order[i];
    song->order[i] = prev;
    prev = curr;
  }
  if (song->order[editor->order_pos] == END_OF_ORDER) {
    --editor->order_pos;
  }
}

void editor_increment_order(struct editor* editor, int delta) {
  struct song* song = editor->song;
  song->order[editor->order_pos] =
    wrap(song->order[editor->order_pos] + delta, 255);
}

void editor_transpose(struct editor* editor, int delta) {
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
  int curr_pattern = editor->song->order[editor->order_pos];
  int free_pattern = song_first_empty_pattern(editor->song);
  if (free_pattern < 0)
    return;
  song_copy_pattern(editor->song,curr_pattern,free_pattern);
  editor->song->order[editor->order_pos] = free_pattern;
}

int editor_handle_key(struct editor* editor) {
  int ch = getch();
  switch(ch) {
  case 27: return 1;
  case KEY_UP: editor_move_pat_line(editor,-1); break;
  case KEY_DOWN: editor_move_pat_line(editor,1); break;
  case KEY_PPAGE: editor_move_pat_line(editor,-16); break;
  case KEY_NPAGE: editor_move_pat_line(editor,16); break;
  case KEY_BTAB:
  case KEY_LEFT:
    editor->pat_track = wrap(editor->pat_track - 1, PAT_TRACKS);
    break;
  case '\t':
  case KEY_RIGHT:
    editor->pat_track = wrap(editor->pat_track + 1, PAT_TRACKS);
    break;
  case '`':
  case '~':
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
  case '1': editor_enter_note_on(editor,6,0); break;
  case '2': editor_enter_note_on(editor,6,1); break;
  case '3': editor_enter_note_on(editor,6,2); break;
  case '4': editor_enter_note_on(editor,6,3); break;
  case '5': editor_enter_note_on(editor,6,4); break;
  case '6': editor_enter_note_on(editor,6,5); break;
  case '7': editor_enter_note_on(editor,6,6); break;
  case 'q': editor_enter_note_on(editor,5,0); break;
  case 'w': editor_enter_note_on(editor,5,1); break;
  case 'e': editor_enter_note_on(editor,5,2); break;
  case 'r': editor_enter_note_on(editor,5,3); break;
  case 't': editor_enter_note_on(editor,5,4); break;
  case 'y': editor_enter_note_on(editor,5,5); break;
  case 'u': editor_enter_note_on(editor,5,6); break;
  case 'a': editor_enter_note_on(editor,4,0); break;
  case 's': editor_enter_note_on(editor,4,1); break;
  case 'd': editor_enter_note_on(editor,4,2); break;
  case 'f': editor_enter_note_on(editor,4,3); break;
  case 'g': editor_enter_note_on(editor,4,4); break;
  case 'h': editor_enter_note_on(editor,4,5); break;
  case 'j': editor_enter_note_on(editor,4,6); break;
  case 'z': editor_enter_note_on(editor,3,0); break;
  case 'x': editor_enter_note_on(editor,3,1); break;
  case 'c': editor_enter_note_on(editor,3,2); break;
  case 'v': editor_enter_note_on(editor,3,3); break;
  case 'b': editor_enter_note_on(editor,3,4); break;
  case 'n': editor_enter_note_on(editor,3,5); break;
  case 'm': editor_enter_note_on(editor,3,6); break;
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
      player_play_from(editor->player,editor->order_pos,0);
      break;
    }
    if (ch == KEY_F(7)) {
      player_play_from(editor->player,editor->order_pos,editor->pat_line);
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
}
