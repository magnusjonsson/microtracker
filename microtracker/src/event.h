struct event {
#define CMD_NOP 0
#define CMD_NOTE_OFF 1
#define CMD_NOTE_ON 2
#define CMD_JI_NOTE_ON 3
  uint8_t cmd;
  uint8_t octave;
  uint8_t degree;
};
