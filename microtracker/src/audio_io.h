#include <portaudio.h>
#include <stdio.h> // fprintf
#include "f2s.h"

struct audio_io {
  PaStream* stream;
  struct player* player;
  float buffer_left[4096];
  float buffer_right[4096];
};

static void print_pa_error(const char* context) {
  const PaHostErrorInfo *info = Pa_GetLastHostErrorInfo();
  fprintf(stderr,"%s: Portaudio error %li: %s\n",context,info->errorCode,info->errorText);
}

static int pa_callback(const void *inputBuffer, void *outputBuffer,
		       unsigned long framesPerBuffer,
		       const PaStreamCallbackTimeInfo* timeInfo,
		       PaStreamCallbackFlags statusFlags,
		       void *userData )
{
  struct audio_io* audio_io = (struct audio_io*)userData;
  //short const *in = (short const*)inputBuffer;
  short *out = (short*)outputBuffer;

  if(!audio_io->player) {
    for(int i=0;i<framesPerBuffer; i++) {
      out[2*i+0] = 0;
      out[2*i+1] = 0;
    }
    return 0;
  }

  player_generate_audio(audio_io->player, audio_io->buffer_left, audio_io->buffer_right, framesPerBuffer);

  float_to_short_stereo(audio_io->buffer_left,
                        audio_io->buffer_right,
                        out,
                        framesPerBuffer);
  return 0;
}

void audio_io_finalize(struct audio_io* audio_io);

int audio_io_init(struct audio_io* audio_io, struct player* player, int device) {
  memset(audio_io,0,sizeof(*audio_io));
  audio_io->player = player;

  if (Pa_Initialize() != paNoError) {
    print_pa_error("Pa_Initialize");
    return 1;
  }
  if (device < 0) {
    device = Pa_GetDefaultOutputDevice();
    if (device < 0) {
      print_pa_error("Pa_GetDefaultOutputDevice");
      goto error;
    }
  }
  
  PaDeviceInfo const* out_info = Pa_GetDeviceInfo(device);
  if (!out_info) {
    fprintf(stderr,"Error: Invalid output device number %i\n",device);
    goto error;
  }
  PaStreamParameters outputParameters={
    .device=device,
    .channelCount=2,
    .sampleFormat=paInt16,
    .suggestedLatency=out_info->defaultLowOutputLatency*1.5,
    .hostApiSpecificStreamInfo=NULL,
  };
  PaStreamFlags streamFlags = paNoFlag;

  if (Pa_OpenStream(&audio_io->stream,
                    NULL, // &inputParameters, // we don't need input
                    &outputParameters,
                    44100,
                    256,        /* frames per buffer, i.e. the number
                                   of sample frames that PortAudio will
                                   request from the callback */
                    streamFlags,
                    pa_callback, /* this is your callback function */
                    audio_io ) /*This is a pointer that will be passed to
                                 your callback*/
      != paNoError) {
    print_pa_error("Pa_OpenStream");
    goto error;
  }
  
  if(Pa_StartStream(audio_io->stream) != paNoError) {
    print_pa_error("Pa_StartStream");
    goto error;
  }

  return 0;
 error:
  audio_io_finalize(audio_io);
  return 1;
}

void audio_io_finalize(struct audio_io* audio_io) {
  if (audio_io->stream) {
    if(Pa_StopStream(audio_io->stream) != paNoError) {
      print_pa_error("Pa_StopStream");
    }
    if(Pa_CloseStream(audio_io->stream) != paNoError) {
      print_pa_error("Pa_CloseStream");
    }
    audio_io->stream = NULL;
  }

  if (Pa_Terminate() != paNoError) {
    print_pa_error("Pa_Terminate");
  }
}
