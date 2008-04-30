OPT_FLAGS = -g
WARNING_FLAGS = -Wall
LIBS = -lncurses -lm -lportaudio -lpthread
INCLUDES = -I ../plugins/src/
GCC_FLAGS = $(OPT_FLAGS) $(INCLUDES) $(WARNING_FLAGS) -std=c99

SRCS = src/main.c src/synths.c src/wavwriter.c

main : $(SRCS:.c=.o)
	gcc $(GCC_FLAGS) $(LIBS) $^ -o $@

%.o : %.c
	gcc $(GCC_FLAGS) -c $< -o $@ -MMD -MF $*.d -MP
clean :
	rm -f main $(SRCS:.c=.d) $(SRCS:.c=.o)

-include $(SRCS:.c=.d)