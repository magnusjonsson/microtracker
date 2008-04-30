struct delay {
  float* buffer;
  int length;
  int pointer;
  int shouldfreebuffer;
};

int delay_memoryneeded(int length);

// memory is optional.
// if memory is null, then the delay will allocate and clear its own memory
// if memory is not null, the delay will use this memory. The memory must be
// cleared by the caller before calling delay_init.
void delay_init(struct delay* d, int length, void* memory);
void delay_clear(struct delay* d);
float delay_read(struct delay* d);
void delay_write(struct delay* d, float in);
float delay_tick(struct delay* d, float in);
void delay_finalize(struct delay* d);
