OPT_FLAGS = -O3 -ffast-math
WARNING_FLAGS = -Wall
LIBS = -shared
GCC_FLAGS = $(OPT_FLAGS) $(WARNING_FLAGS) -std=gnu99 -Isrc/shared

all : organ.so reverb.so reverb2.so reverb3.so reverb4.so chorus.so simplesynth.so plucksynth.so drop.so 2drop.so add.so 2add.so swap.so resobass.so simplesynth2.so

organ.so : src/organ.o src/shared/pipe.o src/shared/onepole.o src/shared/delay.o src/shared/lagrange.o
reverb3.so : src/reverb3.o src/shared/bandpass.c
reverb4.so : src/reverb4.o src/shared/onepole.o
simplesynth.so : src/simplesynth.o src/shared/moogfilter.o src/shared/fasttanh.o
simplesynth2.so : src/simplesynth2.o src/shared/ms20filter.o src/shared/fasttanh.o
resobass.so : src/resobass.o src/shared/moogfilter.o src/shared/fasttanh.o

%.so : src/%.o
	gcc $(GCC_FLAGS) $(LIBS) $^ -o $@

%.o : %.c
	gcc $(GCC_FLAGS) -c $< -o $@ -MMD -MF $*.d -MP
%.S : %.c
	gcc $(GCC_FLAGS) -c $< -S -o $@

clean :
	rm -f *.so src/*.o src/*.d src/shared/*.o src/shared/*.d

-include src/*.d src/shared/*.d