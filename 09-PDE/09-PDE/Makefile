# Common definitions
CC = mpicc

# Compiler flags, paths and libraries
CFLAGS = -std=c99 -pedantic -Wall -Wextra -O2 -ggdb
LFLAGS = $(CFLAGS)
LIBS   = -lm

OBJS = askparams.o

# Rule to create *.o from *.c
.c.o:
	$(CC) -c $(CFLAGS) $*.c

# Targets ...
all: partdiff-par

partdiff-par: partdiff-par.o $(OBJS) Makefile
	$(CC) $(LFLAGS) -o $@ $< $(OBJS) $(LIBS)


clean:
	$(RM) *.o *~

# partdiff-par.o: partdiff-par.c Makefile

partdiff-par.o: partdiff-par.c Makefile

askparams.o: askparams.c Makefile
