#include <stdint.h>
#include <memory.h>

#include "song.h"
#include "player.h"
#include "audio_io.h"
#include "editor.h"

int main(int argc, char** argv) {
  const char* filename = argc >= 2 ? argv[1] : "untitled.song";
  struct song song;
  struct player player;
  struct audio_io audio_io;
  struct editor editor;
  song_init(&song);
  song_load(&song,filename);
  if (!player_init(&player,&song)) {
    if (!audio_io_init(&audio_io,&player)) {
      editor_init(&editor,filename,&song,&player);
      editor_run(&editor);
      editor_finalize(&editor);
      audio_io_finalize(&audio_io);
    }
    player_finalize(&player);
  }
  song_finalize(&song);
  return 0;
}
