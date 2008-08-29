OPT_FLAGS = -O
WARNING_FLAGS = -Wall
LIBS = -lncurses -lm -lportaudio -lpthread
INCLUDES = -I ../plugins/src/
GCC_FLAGS = $(OPT_FLAGS) $(INCLUDES) $(WARNING_FLAGS) -std=c99

.PHONY : all

all : main dump

MAIN_SRCS = src/main.c src/synths.c
DUMP_SRCS = src/dump.c src/synths.c src/wavwriter.c

main : $(MAIN_SRCS:.c=.o)
	gcc $^ -o $@ $(LIBS)

dump : $(DUMP_SRCS:.c=.o)
	gcc $^ -o $@ $(LIBS)

%.o : %.c
	gcc $(GCC_FLAGS) -c $< -o $@ -MMD -MF $*.d -MP
clean :
	rm -f main $(MAIN_SRCS:.c=.o) $(DUMP_SRCS:.c=.o) $(MAIN_SRCS:.c=.d) $(DUMP_SRCS:.c=.d)

-include $(MAIN_SRCS:.c=.d)
-include $(DUMP_SRCS:.c=.d)
