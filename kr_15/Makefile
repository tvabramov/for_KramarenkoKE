CC = gcc
CFLAGS = -Wall -O0 -ggdb
LDFLAGS = -pthread

all: ./bin/tcpserver ./bin/tcpclient

./bin/tcpserver: ./src/tcpserver.c ./src/common.h
	$(CC) $(CFLAGS) $(LDFLAGS) ./src/tcpserver.c -o ./bin/tcpserver

./bin/tcpclient: ./src/tcpclient.c ./src/common.h
	$(CC) $(CFLAGS) $(LDFLAGS) ./src/tcpclient.c -o ./bin/tcpclient

clean: 
	rm ./bin/*

