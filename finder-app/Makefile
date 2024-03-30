CFLAGS=-g -Wall

all: writer
default: writer

writer: writer.o
	$(CROSS_COMPILE)$(CC) $(CFLAGS) -o writer writer.o


writer.o : writer.c
	$(CROSS_COMPILE)$(CC) -c writer.c -o writer.o

.PHONY: clean

clean:
	rm -rf *.o
	rm -f writer
