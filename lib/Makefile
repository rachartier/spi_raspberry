CC=gcc
CFLAGS=-O3 -Wall -Wextra -Werror -fPIC -c
LDFLAGS=-fPIC -shared
OBJS=spi.o

all: libspi.so
.c.o:
	$(CC) $(CFLAGS) $<

libspi.so: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

.PHONY: clean
clean: 
	rm -f $(OBJS)
	rm -f libspi.so
