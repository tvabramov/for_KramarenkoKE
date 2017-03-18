CC = gcc
CFLAGS = -Wall -O0 -ggdb -I/home/students/tvabramov/kr_11/my-ncurses/include
LDFLAGS = -L/home/students/tvabramov/kr_11/my-ncurses/lib -lncurses

all: mqchat

mqchat: src.c
	$(CC) $(CFLAGS) src.c -o mqchat $(LDFLAGS)

clean:
	rm mqchat
