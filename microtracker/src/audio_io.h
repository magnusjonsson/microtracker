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

int audio_io_init(struct audio_io* audio_io, struct player* player) {
  memset(audio_io,0,sizeof(*audio_io));
  audio_io->player = player;

  if (Pa_Initialize() != paNoError) {
    print_pa_error("Pa_Initialize");
    return 1;
  }
  
  PaHostApiInfo const* api_info =
    Pa_GetHostApiInfo(Pa_HostApiTypeIdToHostApiIndex(paALSA));
  if (!api_info) {
    fprintf(stderr,"Error: no alsa\n");
    return 1;
  }
  /*
  PaStreamParameters inputParameters={
    .device=api_info->defaultInputDevice,
    .channelCount=2,
    .sampleFormat=paInt16,
    .suggestedLatency=0.030,
    .hostApiSpecificStreamInfo=NULL,
  };
  */
  if (api_info->defaultOutputDevice == paNoDevice) {
    fprintf(stderr,"Error: no default alsa output device\n");
    return 1;
  }
  PaDeviceInfo const* out_info = Pa_GetDeviceInfo(api_info->defaultOutputDevice);
  PaStreamParameters outputParameters={
    .device=api_info->defaultOutputDevice,
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
    return 1;
  }
  
  if(Pa_StartStream(audio_io->stream) != paNoError) {
    print_pa_error("Pa_StartStream");
    return 1;
  }

  return 0;
}

void audio_io_finalize(struct audio_io* audio_io) {
  if (audio_io->stream) {
    if(Pa_StopStream(audio_io->stream) != paNoError) {
      print_pa_error("Pa_StopStream");
    }
    audio_io->stream = NULL;
    if(Pa_CloseStream(audio_io->stream) != paNoError) {
      print_pa_error("Pa_CloseStream");
    }
  }

  if (Pa_Terminate() != paNoError) {
    print_pa_error("Pa_Terminate");
  }
}
