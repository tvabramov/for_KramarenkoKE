CC = gcc
CFLAGS = -Wall -O0 -ggdb -I/home/students/tvabramov/my-ncurses/include
LDFLAGS = -L/home/students/tvabramov/my-ncurses/lib -lncurses

all: lab11

lab11: src.c
	$(CC) $(CFLAGS) src.c -o lab11 $(LDFLAGS)

clean:
	rm lab11
