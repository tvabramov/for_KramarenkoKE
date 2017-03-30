CC = gcc
CFLAGS = -Wall -O0 -ggdb
LDFLAGS = -pthread

all: udpserver udpclient

udpserver: ./src/udpserver.o
	$(CC) $(LDFLAGS) ./src/udpserver.o -o ./bin/udpserver

udpclient: ./src/udpclient.o
	$(CC) $(LDFLAGS) ./src/udpclient.o -o ./bin/udpclient

udpserver.o: ./src/udpserver.c ./src/common.h
	$(CC) $(CFLAGS) ./src/udpserver.c -c

udpclient.o: ./src/udpclient.c ./src/common.h
	$(CC) $(CFLAGS) ./src/udpclient.c -c

clean:
	rm ./bin/* ./src/*.o
