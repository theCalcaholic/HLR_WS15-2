# Common definitions
CC = gcc

# Compiler flags, paths and libraries
CFLAGS = -std=c99 -pedantic -Wall -Wextra -O2 -ggdb
LFLAGS = $(CFLAGS)
LIBS   = -lm

OBJS = askparams.o

# Rule to create *.o from *.c
.c.o:
	$(CC) -c $(CFLAGS) $*.c

# Targets ...
all: partdiff-seq partdiff-openmp partdiff-openmp-element partdiff-openmp-zeilen partdiff-openmp-spalten

partdiff-seq: partdiff-seq.o $(OBJS) Makefile
	$(CC) $(LFLAGS) -o $@ $< $(OBJS) $(LIBS)

partdiff-openmp-%: $@.o $(OBJS) Makefile
	$(CC) $(LFLAGS) -fopenmp -o $@ $< $(OBJS) $(LIBS)

clean:
	$(RM) *.o *~

partdiff-seq.o: partdiff-seq.c Makefile

askparams.o: askparams.c Makefile
