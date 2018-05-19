CC=gcc
CFLAGS=-Wall -O2 -finline-functions


all:testioctl testkey

dtest:	test.o
	$(CC) -o dtest test.o

testioctl: testioctl.c
	$(CC) $(CFLAGS) -I../driver -o testioctl testioctl.c

testkey: testkey.c
	$(CC) $(CFLAGS) -I../driver -o testkey testkey.c

clean:
	rm -f dtest *.o testkey testioctl
