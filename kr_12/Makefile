CC = gcc
CFLAGS = -Wall -O0 -ggdb -I/home/students/tvabramov/kr_12/my-ncurses/include
LDFLAGS = -L/home/students/tvabramov/kr_12/my-ncurses/lib -lncurses

all: mqchat

mqchat: ./src/mqchat.o ./src/clinit.o
	$(CC) ./src/mqchat.o ./src/clinit.o -o mqchat $(LDFLAGS)

mqchat.o: ./src/mqchat.c
	$(CC) $(CFLAGS) -c ./src/mqchat.c -o ./src/mqchat.o 

clinit.o: ./src/clinit.c  ./src/clinit.h
	$(CC) $(CFLAGS) -c ./src/clinit.c -o ./src/clinit.o

clean:
	rm mqchat ./src/*.o
