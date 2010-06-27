OPT_FLAGS = -O
WARNING_FLAGS = -Wall
INCLUDES = -I ../plugins/src/
GCC_FLAGS = $(OPT_FLAGS) $(INCLUDES) $(WARNING_FLAGS) -std=c99

.PHONY : all

all : main dump song2abc

MAIN_SRCS = src/main.c src/synths.c src/song.c src/player.c src/util.c
DUMP_SRCS = src/dump.c src/synths.c src/wavwriter.c src/song.c src/player.c src/util.c
SONG2ABC_SRCS = src/song2abc.c src/synths.c src/song.c src/util.c

main : $(MAIN_SRCS:.c=.o)
	gcc $^ -o $@ -lncurses -lm -ljack -lpthread -ldl
#	gcc $^ -o $@ -lncurses -lm -lportaudio -lpthread -ldl

dump : $(DUMP_SRCS:.c=.o)
	gcc $^ -o $@ -lm -lpthread -ldl

song2abc : $(SONG2ABC_SRCS:.c=.o)
	gcc $^ -o $@ -lm -lpthread -ldl

%.o : %.c
	gcc $(GCC_FLAGS) -c $< -o $@ -MMD -MF $*.d -MP
clean :
	rm -f main dump song2abc $(MAIN_SRCS:.c=.o) $(DUMP_SRCS:.c=.o) $(SONG2ABC_SRCS:.c=.o) $(MAIN_SRCS:.c=.d) $(DUMP_SRCS:.c=.d) $(SONG2ABC_SRCS:.c=.d)

-include $(MAIN_SRCS:.c=.d)
-include $(DUMP_SRCS:.c=.d)
-include $(SONG2ABC_SRCS:.c=.d)
