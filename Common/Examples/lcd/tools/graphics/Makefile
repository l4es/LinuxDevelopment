CC	=	gcc
CFLAGS	=	-Wall -O2 -finline-functions

all:		wave

wave:	wave.o
	$(CC) -o wave wave.o -lm

clean:
	rm -f wave *.o
