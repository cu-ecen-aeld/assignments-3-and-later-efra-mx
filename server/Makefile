CC=$(CROSS_COMPILE)gcc
CFLAGS=-g -Wall -Werror -gdwarf -DDEBUG
LDFLAGS ?= -pthread -lrt

all: aesdsocket
default: aesdsocket

aesdsocket: aesdsocket.o linked-list.o
	$(CC) $(CFLAGS) -o aesdsocket aesdsocket.o linked-list.o $(LDFLAGS)

aesdsocket.o : aesdsocket.c
	$(CC) -g -c aesdsocket.c -o aesdsocket.o

linked-list.o : linked-list.c
	$(CC) -g -c linked-list.c -o linked-list.o

.PHONY: clean

clean:
	rm -rf *.o
	rm -f aesdsocket
