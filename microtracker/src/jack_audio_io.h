#include <jack/jack.h>
#include <stdio.h> // fprintf
#include "f2s.h"

struct audio_io {
  jack_client_t* jack_client;
  jack_port_t* jack_port_left_out;
  jack_port_t* jack_port_right_out;
  struct player* player;
};

static int jack_process_callback(jack_nframes_t nframes, void* arg)
{
  struct audio_io* audio_io = (struct audio_io*)arg;
  float* buffer_left = jack_port_get_buffer(audio_io->jack_port_left_out, nframes);
  if (buffer_left == NULL) return 1;
  float* buffer_right = jack_port_get_buffer(audio_io->jack_port_right_out, nframes);
  if (buffer_right == NULL) return 1;

  if (audio_io->player != NULL) {
    player_generate_audio(audio_io->player, buffer_left, buffer_right, nframes);
  }
  return 0;
}

void audio_io_finalize(struct audio_io* audio_io);

int audio_io_init(struct audio_io* audio_io, struct player* player, int device) {
  memset(audio_io,0,sizeof(*audio_io));
  audio_io->player = player;

  jack_status_t jack_status = 0;
  audio_io->jack_client =
    jack_client_open("microtracker", JackNullOption, &jack_status);

  if (audio_io->jack_client == NULL) goto error;
			  
  if (jack_set_process_callback(audio_io->jack_client,
				jack_process_callback,
				audio_io)
      != 0) {
    goto error;
  }

  audio_io->jack_port_left_out =
    jack_port_register(audio_io->jack_client,
		       "left out",
		       JACK_DEFAULT_AUDIO_TYPE,
		       JackPortIsOutput | JackPortIsTerminal,
		       0);
  if (audio_io->jack_port_left_out == NULL) goto error;

  audio_io->jack_port_right_out =
    jack_port_register(audio_io->jack_client,
		       "right out",
		       JACK_DEFAULT_AUDIO_TYPE,
		       JackPortIsOutput | JackPortIsTerminal,
		       0);
  if (audio_io->jack_port_right_out == NULL) goto error;

  if (jack_activate(audio_io->jack_client) != 0) {
    goto error;
  }

  return 0;
 error:
  audio_io_finalize(audio_io);
  return 1;
}

int audio_io_get_sample_rate(struct audio_io* audio_io) {
  return jack_get_sample_rate(audio_io->jack_client);
}

void audio_io_set_player(struct audio_io* audio_io, struct player* player) {
  audio_io->player = player;
}

void audio_io_finalize(struct audio_io* audio_io) {
  if (audio_io->jack_client) {
    jack_deactivate(audio_io->jack_client);
    if(audio_io->jack_port_left_out != NULL) {
      jack_port_unregister(audio_io->jack_client,
			   audio_io->jack_port_left_out);
      audio_io->jack_port_left_out = NULL;
    }
    if (audio_io->jack_port_right_out != NULL) {
      jack_port_unregister(audio_io->jack_client,
			   audio_io->jack_port_right_out);
      audio_io->jack_port_right_out = NULL;
    }
    jack_client_close(audio_io->jack_client);
    audio_io->jack_client = NULL;
  }
}
