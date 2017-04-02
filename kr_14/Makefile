CC = gcc
CFLAGS = -Wall -O0 -ggdb
LDFLAGS = -pthread

all: ./bin/udpserver ./bin/udpclient

./bin/udpserver: ./src/udpserver.c ./src/common.h
	$(CC) $(CFLAGS) $(LDFLAGS) ./src/udpserver.c -o ./bin/udpserver

./bin/udpclient: ./src/udpclient.c ./src/common.h
	$(CC) $(CFLAGS) $(LDFLAGS) ./src/udpclient.c -o ./bin/udpclient

clean: 
	rm ./bin/*

