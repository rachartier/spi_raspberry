CC=gcc
CFLAGS=-O3 -Wall -Wextra -Werror -c
LDFLAGS= -Llib -lspi -Wl,-rpath,lib/
OBJS=main.o

all: test_spi

.c.o:
	$(CC) $(CFLAGS) $<

test_spi: libspi cpyheader $(OBJS) 
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

cpyheader:
	cp lib/spi.h .

libspi:
	$(MAKE) -C lib/

.PHONY: clean
clean:
	$(MAKE) clean -C lib 
	rm -f libspi.so 
	rm -f spi.h
	rm -f $(OBJS)
	rm test_spi
