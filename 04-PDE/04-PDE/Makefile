# Common definitions
CC = gcc

# Compiler flags, paths and libraries
CFLAGS = -std=c99 -pedantic -Wall -Wextra -O2 -ggdb -fopenmp
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

#partdiff-openmp: partdiff-openmp-zeilen

partdiff-openmp-zeilen: partdiff-openmp-zeilen.o $(OBJS) Makefile
	$(CC) $(LFLAGS) -o $@ $< $(OBJS) $(LIBS)

clean:
	$(RM) *.o *~

# partdiff-seq.o: partdiff-seq.c Makefile

partdiff-seq.o: partdiff-seq.c Makefile

partdiff-openmp-zeilen.o: partdiff-openmp.c 
	$(CC) -c -D rows  $(CFLAGS) partdiff-openmp.c -o partdiff-openmp-zeilen.o

askparams.o: askparams.c Makefile
