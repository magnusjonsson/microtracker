#include <portaudio.h>
#include <stdio.h>

void listportaudioapis() {
  int api_count = 0;
  const PaHostApiInfo* api;
  while((api = Pa_GetHostApiInfo(api_count++)) != NULL) {
    printf("Host API: %s\n",api->name);
    printf("  type: %i\n", api->type);
    printf("  device count: %i\n",api->deviceCount);
    printf("  default input device: %i\n",api->defaultInputDevice);
    printf("  default output device: %i\n",api->defaultOutputDevice);
  }
}

void listportaudiodevices() {
  int device_count = Pa_GetDeviceCount();
  for(int i=0;i<device_count;i++) {
    PaDeviceInfo const* info = Pa_GetDeviceInfo(i);
    printf("Portaudio device %i:\n",i);
    printf("    Name: %s\n",info->name);
    const PaHostApiInfo* api = Pa_GetHostApiInfo(info->hostApi);
    printf("    Host API: %s\n",api->name);
    printf("    In/Out/Samplerate: %i/%i/%lf\n",info->maxInputChannels,info->maxOutputChannels,info->defaultSampleRate);
  }
}

int main() {
  Pa_Initialize();
  listportaudioapis();
  listportaudiodevices();
  Pa_Terminate();
  return 0;
}
