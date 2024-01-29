CC=$(CROSS_COMPILE)gcc
CFLAGS=-Wall -Werror

all: writer

writer:
	$(CC) $(CFLAGS) writer.c -o writer

clean:
	rm -f *.o writer
