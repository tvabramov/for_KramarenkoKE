CC = gcc
CFLAGS = -Wall -O0 -I/home/students/tvabramov/my-ncurses/include
LDFLAGS = -L/home/students/tvabramov/my-ncurses/lib -lncurses

all: lab3

lab3: src.c
	$(CC) $(CFLAGS) src.c -o lab3 $(LDFLAGS)

clean:
	rm lab3
