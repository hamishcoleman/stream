
PROGS+=stream_d
PROGS+=stream_omp

LDLIBS=-lm

all: $(PROGS)

stream_omp: stream_omp.c
	gcc -mcmodel=large -fopenmp $< -o $@
