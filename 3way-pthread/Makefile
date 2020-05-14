

LDFLAGS = -pthread
CC = gcc
PROGRAM = main
SRCS = main.c
OBJS = $(SRCS:.c=.o)

default: all

all: $(PROGRAM)

$(PROGRAM): $(OBJS)
	$(CC) -O2 $(OBJS) -o $(PROGRAM) $(LDFLAGS)

clean:
	/bin/rm -f $(OBJS) $(PROGRAM)
