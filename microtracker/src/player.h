#include "synthdesc.h"
#include "synths.h"
#include "song.h"
#include <pthread.h>

struct player {
  struct song* song;
  char playing;
  struct songcursor cursor;

  int distance_to_next_tick;
  int samples_per_tick;

  pthread_mutex_t mutex;

  struct synthdesc const* synthdesc;
  void* synthstate;
  struct synthdesc const* effectdesc;
  void* effectstate;
};

int player_init(struct player* player, struct song* song, struct synthdesc const* synthdesc, struct synthdesc const* effectdesc, int samplerate);
void player_finalize(struct player* player);
void player_advance_cursor(struct player* player);
void player_track_handle_event(struct player* player, int track, struct event event);
void player_tick(struct player* player);
void player_generate_audio_block(struct player* player, float* out_left, float* out_right, int length);
void player_handle_events(struct player* player);

// returns length of block generated
int player_generate_some_audio(struct player* player, float* out_left, float* out_right, int maxlength);
void player_generate_audio(struct player* player, float* out_left, float* out_right, int length);
void player_play(struct player* player);
void player_stop(struct player* player);
void player_play_from(struct player* player, struct songcursor const* cursor);
int player_is_at_beginning_of_song(struct player* player);

// any code block that modifies the player's song must be surrounded by
// calls to these.
void player_begin_song_edit(struct player* player);
void player_end_song_edit(struct player* player);
