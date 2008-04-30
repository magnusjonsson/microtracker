#include <math.h>
#include "synthdesc.h"
#include "synths.h"
#include <malloc.h>
#include <stdio.h>
#include <pthread.h>
#include "wavwriter.h"

struct player {
  struct song* song;
  char playing;
  short order_pos;
  short pat_line;
  //struct track tracks[PAT_TRACKS];
  
  int distance_to_next_tick;

  pthread_mutex_t mutex;

  struct synthdesc const* synthdesc;
  void* synthstate;
  struct synthdesc const* effectdesc;
  void* effectstate;
};

#define SAMPLES_PER_TICK 4410

int synthdesc_instantiate(struct synthdesc const* synthdesc, double samplerate, void** state) {
  *state = synthdesc->size ? malloc(synthdesc->size(44100)) : NULL;
  if (!*state) {
    fprintf(stderr,"Failed to allocate memory for synth\n");
    return 1;
  }

  if (synthdesc->init)
    synthdesc->init(*state, 44100);

  return 0;
}

void synthdesc_deinstantiate(struct synthdesc const* synthdesc, void** state) {
  if (synthdesc) {
    if (synthdesc->finalize) {
      synthdesc->finalize(*state);
      free(*state);
      *state = NULL;
    }
  }
}

int player_init(struct player* player, struct song* song) {
  memset(player,0,sizeof(*player));
  if (pthread_mutex_init(&player->mutex,NULL))
    return 1;
  player->song = song;
  player->synthdesc = finddesc("organ"); // TODO: reclaim synthdesc to make dll unload
  
  if (!player->synthdesc) {
    fprintf(stderr,"Couldn't load organ plugin dll\n");
    return 1;
  }

  if (synthdesc_instantiate(player->synthdesc,44100,&player->synthstate)) {
    fprintf(stderr,"Couldn't instantiate organ plugin\n");
    return 1;
  }

  player->effectdesc = finddesc("reverb4");

  if (!player->effectdesc) {
    fprintf(stderr,"Coudln't load reverb4 plugin dll\n");
    return 1;
  }

  if (synthdesc_instantiate(player->effectdesc,44100,&player->effectstate)) {
    fprintf(stderr,"Couldn't instantiate reverb4 plugin\n");
    return 1;
  }


  return 0;
}


void player_finalize(struct player* player) {
  pthread_mutex_lock(&player->mutex);
  synthdesc_deinstantiate(player->synthdesc,&player->synthstate);
  player->synthdesc = NULL;
  synthdesc_deinstantiate(player->effectdesc,&player->effectstate);
  player->effectdesc = NULL;
  pthread_mutex_destroy(&player->mutex);
}

void player_advance_cursor(struct player* player) {
  struct song* song = player->song;
  if (player->pat_line == PAT_LINES - 1) {
    player->pat_line = 0;
    if (player->order_pos == 255 || song->order[player->order_pos+1] == END_OF_ORDER) {
      player->order_pos = 0;
    }
    else {
      ++player->order_pos;
    }
  }
  else {
    player->pat_line++;
  }
}

double calc_freq(int octave, int degree, int edo) {
  // fun thing: round notes to 12-edo
  //degree = (degree * 12 + 27)/53;
  //return pow(2,octave+3+(double)degree/12.0);
  return pow(2,octave+3+(double)degree/edo);
}

void player_track_handle_event(struct player* player, int track, struct event event, float samplerate) {
  switch(event.cmd) {
  case CMD_NOP:
    break;
  case CMD_NOTE_OFF:
    if (player->synthdesc && player->synthdesc->noteoff) {
      player->synthdesc->noteoff(player->synthstate, track);
    }
    break;
  case CMD_NOTE_ON:
    if (player->synthdesc && player->synthdesc->noteoff) {
      player->synthdesc->noteoff(player->synthstate,track);
    }
    if (player->synthdesc && player->synthdesc->noteon) {
      double freq = calc_freq(event.octave, event.degree, player->song->octave_divisions);
      player->synthdesc->noteon(player->synthstate,track,freq,0.5);
    }
  }
}

void player_tick(struct player* player) {
  struct song* song = player->song;
  struct event* track_events = song->patterns[song->order[player->order_pos]][player->pat_line];
  for(int i=0;i<PAT_TRACKS;i++) {
    player_track_handle_event(player,i,track_events[i],44100);
  }
  player_advance_cursor(player);
}

void player_generate_audio_block(struct player* player, float* out_left, float* out_right, int length) {
  
  if (!player->synthdesc || !player->synthdesc->process) {
    for(int i=0;i<length;i++) {
      out_left[i] = 0;
      out_right[i] = 0;
    }
    return;
  }

  float* outs[] = { out_left, out_right };

  player->synthdesc->process(player->synthstate, length, NULL, outs);
  if (player->effectdesc && player->effectdesc->process) {
    //player->effectdesc->process(player->effectstate, length, outs, outs);
  }
}

void player_handle_events(struct player* player) {
  if (!player->playing)
    return;
  while (player->distance_to_next_tick <= 0) {
    player_tick(player);
    player->distance_to_next_tick += SAMPLES_PER_TICK;
  }
}

// returns length of block generated
int player_generate_some_audio(struct player* player, float* out_left, float* out_right, int maxlength) {
  pthread_mutex_lock(&player->mutex);

  if (maxlength <= 0)
    return 0;
  
  player_handle_events(player);

  int block_length = maxlength;
  if (player->playing) {
    if (block_length > player->distance_to_next_tick)
      block_length = player->distance_to_next_tick;
  }

  player_generate_audio_block(player,out_left,out_right,block_length);

  if (player->playing) {
    player->distance_to_next_tick -= block_length;
  }

  pthread_mutex_unlock(&player->mutex);

  return block_length;
}

void player_generate_audio(struct player* player, float* out_left, float* out_right, int length) {
  while(length > 0) {
    int block_length =
      player_generate_some_audio(player,out_left,out_right,length);
    length -= block_length;
    out_left += block_length;
    out_right += block_length;
  }
}

void player_play(struct player* player) {
  pthread_mutex_lock(&player->mutex);
  player->playing = 1;
  pthread_mutex_unlock(&player->mutex);
}

void player_stop(struct player* player) {
  pthread_mutex_lock(&player->mutex);
  player->playing = 0;
  if (player->synthdesc && player->synthdesc->noteoff) {
    for(int i=0;i<PAT_TRACKS;i++) {
      player->synthdesc->noteoff(player->synthstate,i);
    }    
  }
  pthread_mutex_unlock(&player->mutex);
}

void player_play_from(struct player* player, int order_pos, int pat_line) {
  pthread_mutex_lock(&player->mutex);
  player->playing = 1;
  player->order_pos = order_pos;
  player->pat_line = pat_line;
  pthread_mutex_unlock(&player->mutex);
}

int player_is_at_beginning_of_song(struct player* player) {
  pthread_mutex_lock(&player->mutex);
  int answer = player->order_pos == 0 && player->pat_line == 0 && player->distance_to_next_tick == 0;
  pthread_mutex_unlock(&player->mutex);
  return answer;
}
