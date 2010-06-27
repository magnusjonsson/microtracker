#include <string.h>
#include <dlfcn.h>
#include <stdio.h>
#include <malloc.h>
#include "synths.h"
#include "synthdesc.h"

const struct synthdesc* finddesc(const char* name) {
  char buffer[1024];
#ifdef CYGWIN
  sprintf(buffer,"../plugins/%s.so",name);
#else
  snprintf(buffer,1024,"../plugins/%s.so",name);
#endif
  void* handle = dlopen(buffer,RTLD_LAZY /* | RTLD_LOCAL not supported on CYGWIN*/);
  if (!handle) {
    fprintf(stderr, "dlopen: %s\n", dlerror());
    return NULL;
  }
  struct synthdesc* synthdesc = dlsym(handle,"synthdesc");
  if (!synthdesc) {
    fprintf(stderr, "shared object %s is not a plugin\n",buffer);
    dlclose(handle);
    return NULL;
  }
  return synthdesc;
}

int synthdesc_instantiate(struct synthdesc const* synthdesc, double samplerate, void** state) {
  *state = synthdesc->size ? malloc(synthdesc->size(samplerate)) : NULL;
  if (!*state) {
    fprintf(stderr,"Failed to allocate memory for synth\n");
    return 1;
  }

  if (synthdesc->init)
    synthdesc->init(*state, samplerate);

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

