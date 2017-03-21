CC = gcc
CFLAGS = -Wall -O0 -I/home/students/tvabramov/my-ncurses/include
LDFLAGS = -L/home/students/tvabramov/my-ncurses/lib -lncurses

all: lab2

lab2: src.c
	$(CC) $(CFLAGS) src.c -o lab2 $(LDFLAGS)

clean:
	rm lab2
