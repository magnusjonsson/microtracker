#include "player.h"
#include <malloc.h>
#include <stdio.h>
#include <math.h>
#include <memory.h>

int player_init(struct player* player, struct song* song, struct synthdesc const* synthdesc, struct synthdesc const* effectdesc, int samplerate) {
  memset(player,0,sizeof(*player));
  songcursor_init(&player->cursor);
  if (pthread_mutex_init(&player->mutex,NULL))
    return 1;
  player->song = song;
  player->synthdesc = synthdesc;
  player->samples_per_tick = samplerate / 12;
  
  if (!player->synthdesc) {
    fprintf(stderr,"Couldn't load synth plugin dll\n");
    return 1;
  }

  if (synthdesc_instantiate(player->synthdesc,samplerate,&player->synthstate)) {
    fprintf(stderr,"Couldn't instantiate synth plugin\n");
    return 1;
  }

  player->effectdesc = effectdesc;

  if (player->effectdesc) {
    if (synthdesc_instantiate(player->effectdesc,samplerate,&player->effectstate)) {
      fprintf(stderr,"Couldn't instantiate reverb4 plugin\n");
      return 1;
    }
  }

  return 0;
}


void player_finalize(struct player* player) {
  songcursor_finalize(&player->cursor);
  pthread_mutex_lock(&player->mutex);
  synthdesc_deinstantiate(player->synthdesc,&player->synthstate);
  player->synthdesc = NULL;
  if (player->effectdesc) {
    synthdesc_deinstantiate(player->effectdesc,&player->effectstate);
    player->effectdesc = NULL;
  }
  pthread_mutex_destroy(&player->mutex);
}

void player_advance_cursor(struct player* player) {
  songcursor_advance(&player->cursor, player->song);
}


static double calc_ji_freq(int octave, int degree) {
  return (double)(octave+1)/(double)(degree+1);
}


static double calc_freq(int octave, int degree, int edo) {
  double multiplier = 0.0;
  if (edo == 53) {
    switch(degree) {
    case 0: multiplier = 1.0; break;
    case 7: multiplier = 35/32.0; break;
    case 8: multiplier = 10.0/9.0; break;
    case 9: multiplier = 9.0/8.0; break;
    case 14: multiplier = 6.0/5.0; break;
    case 17: multiplier = 5.0/4.0; break;
    case 18: multiplier = 80.0/63.0; break; // 10/9 * 8/7 = 80/63
    case 21: multiplier = 21.0/16.0; break;
    case 22: multiplier = 4.0/3.0; break;
    case 24: multiplier = 11.0/8.0; break;
    case 25: multiplier = 25.0/18.0; break; // 5/3 * 5/3 * 1/2
    case 26: multiplier = 45.0/32.0; break;
    case 29: multiplier = 35.0/24.0; break;
    case 31: multiplier = 3.0/2.0; break;
    case 34: multiplier = 25.0/16.0; break;
    case 36: multiplier = 8.0/5.0; break;
    case 37: multiplier = 13.0/8.0; break;
    case 39: multiplier = 5.0/3.0; break;
    case 40: multiplier = 27.0/16.0; break;
    case 43: multiplier = 7.0/4.0; break;
    case 44: multiplier = 16.0/9.0; break;
    case 45: multiplier = 9.0/5.0; break;
    case 48: multiplier = 15.0/8.0; break;
    case 49: multiplier = 40.0/21.0; break; // 5/3 * 8/7 = 40/21
    }
  }
  if (multiplier > 0)
    return pow(2.0, octave) * multiplier;
  else
    return pow(2,octave+(double)degree/edo);
}

void player_track_handle_event(struct player* player, int track, struct event event) {
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
      double freq = 8 * calc_freq(event.octave, event.degree, 53);
      player->synthdesc->noteon(player->synthstate,track,freq,0.5);
    }
    break;
  case CMD_JI_NOTE_ON:
    if (player->synthdesc && player->synthdesc->noteoff) {
      player->synthdesc->noteoff(player->synthstate,track);
    }
    if (player->synthdesc && player->synthdesc->noteon) {
      double A4 = 440.0;
      double freq = A4 * calc_ji_freq(event.octave, event.degree);
      player->synthdesc->noteon(player->synthstate,track,freq,0.5);
    }
    break;
  case CMD_31_EDO_NOTE_ON:
    if (player->synthdesc && player->synthdesc->noteoff) {
      player->synthdesc->noteoff(player->synthstate,track);
    }
    if (player->synthdesc && player->synthdesc->noteon) {
      double freq = 8 * calc_freq(event.octave, event.degree, 31);
      player->synthdesc->noteon(player->synthstate,track,freq,0.5);
    }
    break;
  }
}

void player_tick(struct player* player) {
  struct event track_events[PAT_TRACKS];

  song_get_line_events(player->song, &player->cursor, track_events);
  for(int i=0;i<PAT_TRACKS;i++) {
    player_track_handle_event(player,i,track_events[i]);
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

  float const* const ins[] = { outs[0], outs[1] };

  if (player->effectdesc && player->effectdesc->process) {
    player->effectdesc->process(player->effectstate, length, ins, outs);
  }
}

void player_handle_events(struct player* player) {
  if (!player->playing)
    return;
  while (player->distance_to_next_tick <= 0) {
    player_tick(player);
    player->distance_to_next_tick += player->samples_per_tick;
  }
}

// returns length of block generated
int player_generate_some_audio(struct player* player, float* out_left, float* out_right, int maxlength) {
  if (maxlength <= 0)
    return 0;
  
  pthread_mutex_lock(&player->mutex);

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

void player_play_from(struct player* player, struct songcursor const* cursor) {
  pthread_mutex_lock(&player->mutex);
  player->playing = 1;
  songcursor_copytofrom(&player->cursor, cursor);
  pthread_mutex_unlock(&player->mutex);
}

int player_is_at_beginning_of_song(struct player* player) {
  pthread_mutex_lock(&player->mutex);
  int answer = songcursor_is_at_beginning_of_song(&player->cursor) && player->distance_to_next_tick == 0;
  pthread_mutex_unlock(&player->mutex);
  return answer;
}

void player_begin_song_edit(struct player* player) {
  pthread_mutex_lock(&player->mutex);
}

void player_end_song_edit(struct player* player) {
  pthread_mutex_unlock(&player->mutex);
}
