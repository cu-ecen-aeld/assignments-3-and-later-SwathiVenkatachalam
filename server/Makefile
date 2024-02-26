CC=$(CROSS_COMPILE)gcc
CFLAGS=-Wall -Werror -g

all: aesdsocket

aesdsocket:
	$(CC) $(CFLAGS) aesdsocket.c -o aesdsocket

clean:
	rm -f *.o aesdsocket
