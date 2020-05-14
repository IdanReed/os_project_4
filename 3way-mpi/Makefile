#LDFLAGS 	= -fopenmp
CC 			= mpicc
PROGRAM 	= main
SRCS 		= main.c
OBJS 		= $(SRCS:.c=.o)

default: all

all: $(PROGRAM)

$(PROGRAM): $(OBJS)
	$(CC) $(OBJS) -o $(PROGRAM) $(LDFLAGS)

clean:
	/bin/rm -f $(OBJS) $(PROGRAM)