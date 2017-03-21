CC = gcc
CFLAGS = -Wall -O0 -I/home/students/tvabramov/my-ncurses/include
LDFLAGS = -L/home/students/tvabramov/my-ncurses/lib -lncurses

all: lab6 test

lab6: src.c
	$(CC) $(CFLAGS) src.c -o lab6 $(LDFLAGS)

test: test.c
	$(CC) $(CFLAGS) test.c -o test
clean:
	rm lab6 test
