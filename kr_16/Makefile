CC = gcc
CFLAGS = -Wall -O0 -ggdb
LDFLAGS = -pthread

all: ./bin/server ./bin/tcpclient ./bin/udpclient

./bin/server: ./src/server.c ./src/common.h
	$(CC) $(CFLAGS) $(LDFLAGS) ./src/server.c -o ./bin/server

./bin/tcpclient: ./src/tcpclient.c ./src/common.h
	$(CC) $(CFLAGS) $(LDFLAGS) ./src/tcpclient.c -o ./bin/tcpclient

./bin/udpclient: ./src/udpclient.c ./src/common.h
	$(CC) $(CFLAGS) $(LDFLAGS) ./src/udpclient.c -o ./bin/udpclient

clean:
	rm ./bin/*

