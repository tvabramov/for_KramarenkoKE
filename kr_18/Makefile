CC = gcc
CFLAGS = -Wall -O0 -ggdb
LDFLAGS = -lpcap

all: ./bin/snif

./bin/snif: ./src/snif.o ./src/clinit.o
	$(CC) ./src/snif.o ./src/clinit.o -o ./bin/snif $(LDFLAGS)

./src/snif.o: ./src/snif.c
	$(CC) $(CFLAGS) -c ./src/snif.c -o ./src/snif.o

./src/clinit.o: ./src/clinit.c ./src/clinit.h
	$(CC) $(CFLAGS) -c ./src/clinit.c -o ./src/clinit.o

clean:
	rm ./bin/* ./src/*.o

