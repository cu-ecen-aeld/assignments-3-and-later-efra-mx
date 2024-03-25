CC=$(CROSS_COMPILE)gcc
CFLAGS=-g -Wall

default: writer

writer: writer.o
	$(CC) $(CFLAGS) -o writer writer.o


writer.o : writer.c
	$(CC) -c writer.c -o writer.o

.PHONY: clean

clean:
	rm -rf *.o
	rm -f writer
