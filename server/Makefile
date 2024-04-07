CCC=$(CROSS_COMPILE)$(CC)
CFLAGS=-g -Wall -Werror -gdwarf -DDEBUG -pthread

all: aesdsocket
default: aesdsocket

aesdsocket: aesdsocket.o linked-list.o
	$(CCC) $(CFLAGS) -o aesdsocket aesdsocket.o linked-list.o

aesdsocket.o : aesdsocket.c
	$(CCC) -g -c aesdsocket.c -o aesdsocket.o

linked-list.o : linked-list.c
	$(CCC) -g -c linked-list.c -o linked-list.o

.PHONY: clean

clean:
	rm -rf *.o
	rm -f aesdsocket
