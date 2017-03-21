CC = gcc
CFLAGS = -Wall -O0
LDFLAGS = 

all: lab5

lab5: src.o my_daemon.o
	$(CC) $(CFLAGS) src.o my_daemon.o -o lab5 $(LDFLAGS)

src.o: src.c
	$(CC) $(CFLAGS) src.c -c

my_daemon.o: my_daemon.c my_daemon.h
	$(CC) $(CFLAGS) my_daemon.c -c

clean:
	rm lab5 src.o my_daemon.o my_daemon.pid
