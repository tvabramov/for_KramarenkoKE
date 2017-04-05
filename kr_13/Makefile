CC = gcc
CFLAGS = -Wall -O0 -ggdb -I/home/students/tvabramov/my-ncurses/include
LDFLAGS = -L/home/students/tvabramov/my-ncurses/lib -lncurses -pthread

all: ./bin/smchat

./bin/smchat: ./src/smchat.o ./src/clinit.o
	$(CC) ./src/smchat.o ./src/clinit.o -o ./bin/smchat $(LDFLAGS)

./src/smchat.o: ./src/smchat.c
	$(CC) $(CFLAGS) -c ./src/smchat.c -o ./src/smchat.o 

./src/clinit.o: ./src/clinit.c  ./src/clinit.h
	$(CC) $(CFLAGS) -c ./src/clinit.c -o ./src/clinit.o

clean:
	rm ./bin/* ./src/*.o
