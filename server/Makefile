CCC=$(CROSS_COMPILE)$(CC)
CFLAGS=-g -Wall -Werror -gdwarf -DDEBUG

all: aesdsocket
default: aesdsocket

aesdsocket: aesdsocket.o
	$(CCC) $(CFLAGS) -o aesdsocket aesdsocket.o


aesdsocket.o : aesdsocket.c
	$(CCC) -g -c aesdsocket.c -o aesdsocket.o

.PHONY: clean

clean:
	rm -rf *.o
	rm -f aesdsocket
