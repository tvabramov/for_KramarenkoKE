CC = gcc
CFLAGS = -Wall -O0 -ggdb -I/home/students/tvabramov/my-libpcap/include
LDFLAGS = -L/home/students/tvabramov/my-libpcap/lib -lpcap

all: ./bin/snif

./bin/snif: ./src/snif.c
	$(CC) $(CFLAGS) ./src/snif.c -o ./bin/snif $(LDFLAGS)

#./bin/tcpclient: ./src/tcpclient.c ./src/common.h
#	$(CC) $(CFLAGS) $(LDFLAGS) ./src/tcpclient.c -o ./bin/tcpclient

#./bin/udpclient: ./src/udpclient.c ./src/common.h
#	$(CC) $(CFLAGS) $(LDFLAGS) ./src/udpclient.c -o ./bin/udpclient

clean:
	rm ./bin/*

