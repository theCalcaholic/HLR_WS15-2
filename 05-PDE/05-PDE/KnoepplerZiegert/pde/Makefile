# Common definitions
CC = gcc

# Compiler flags, paths and libraries
CFLAGS = -std=c99 -pedantic -Wall -Wextra -O0 -ggdb -pthread
LFLAGS = $(CFLAGS)
LIBS   = -lm

OBJS = askparams.o

# Rule to create *.o from *.c
.c.o:
	$(CC) -c $(CFLAGS) $*.c

# Targets ...
all: partdiff-seq partdiff-posix

partdiff-posix: partdiff-posix.o $(OBJS) Makefile
	$(CC) $(LFLAGS) -o $@ $< $(OBJS) $(LIBS)

partdiff-seq: partdiff-seq.o $(OBJS) Makefile
	$(CC) $(LFLAGS) -o $@ $< $(OBJS) $(LIBS)




clean:
	$(RM) *.o *~ partdiff-posix partdiff-seq

# partdiff-seq.o: partdiff-seq.c Makefile

partdiff-seq.o: partdiff-seq.c Makefile

partdiff-posix.o: partdiff-posix.c Makefile

askparams.o: askparams.c Makefile
