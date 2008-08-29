#include <string.h>
#include "synths.h"
#include <dlfcn.h>
#include <stdio.h>

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
